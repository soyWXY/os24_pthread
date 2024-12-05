#include <assert.h>
#include <stdlib.h>
#include <vector>
#include "ts_queue.hpp"
#include "item.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "producer.hpp"
#include "consumer_controller.hpp"
#include "debug.hpp"

#define READER_QUEUE_SIZE 200
#define WORKER_QUEUE_SIZE 200
#define WRITER_QUEUE_SIZE 4000
#define CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE 20
#define CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE 80
#define CONSUMER_CONTROLLER_CHECK_PERIOD 1000000
#define PRODUCER_SIZE 4

int main(int argc, char** argv) {
	assert(argc == 4);

	int n = atoi(argv[1]);
	std::string input_file_name(argv[2]);
	std::string output_file_name(argv[3]);

	// initialization
	TSQueue<Item *> input_q(READER_QUEUE_SIZE);
	TSQueue<Item *> worker_q(WORKER_QUEUE_SIZE);
	TSQueue<Item *> output_q(WRITER_QUEUE_SIZE);
	Transformer xformer;

	Reader reader(n, input_file_name, &input_q);
	std::vector<Producer> all_producer;
	for (int i = 0; i < PRODUCER_SIZE; ++i)
		all_producer.emplace_back(&input_q, &worker_q, &xformer);
	ConsumerController ctrler(&worker_q, &output_q, &xformer,
		  					  CONSUMER_CONTROLLER_CHECK_PERIOD,
		  					  CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE,
		  					  CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE);
	Writer writer(n, output_file_name, &output_q);

	// execution
	ctrler.start();
	reader.start();
	for (Producer& producer: all_producer)
		producer.start();
	writer.start();

	// teardown
	reader.join();
	writer.join();
	for (Producer& producer: all_producer) {
		producer.cancel();
		producer.detach();  // doesn't matter if it's joined or detached
	}
	ctrler.cancel();
	ctrler.join();  // must be join

	DEBUG("exiting main\n");
	return 0;
}
