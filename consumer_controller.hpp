#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <thread>
#include <chrono>
#include "debug.hpp"
#include "consumer.hpp"
#include "ts_queue.hpp"
#include "item.hpp"
#include "transformer.hpp"

#ifndef CONSUMER_CONTROLLER
#define CONSUMER_CONTROLLER

class ConsumerController : public Thread {
public:
	// constructor
	ConsumerController(
		TSQueue<Item*>* worker_queue,
		TSQueue<Item*>* writer_queue,
		Transformer* transformer,
		int check_period,
		int low_threshold,
		int high_threshold
	);

	// destructor
	~ConsumerController();

	virtual void start();

private:
	std::vector<Consumer*> consumers;

	TSQueue<Item*>* worker_queue;
	TSQueue<Item*>* writer_queue;

	Transformer* transformer;

	// Check to scale down or scale up every check period in microseconds.
	int check_period;
	// When the number of items in the worker queue is lower than low_threshold,
	// the number of consumers scaled down by 1.
	int low_threshold;
	// When the number of items in the worker queue is higher than high_threshold,
	// the number of consumers scaled up by 1.
	int high_threshold;

	static void* process(void* arg);
};

// Implementation start

ConsumerController::ConsumerController(
	TSQueue<Item*>* worker_queue,
	TSQueue<Item*>* writer_queue,
	Transformer* transformer,
	int check_period,
	int low_threshold,
	int high_threshold
) : worker_queue(worker_queue),
	writer_queue(writer_queue),
	transformer(transformer),
	check_period(check_period),
	low_threshold(low_threshold),
	high_threshold(high_threshold) {
}

ConsumerController::~ConsumerController() {
	// ensure program really ends and all consumers
	// should be blocked at dequeueing worker queue
	ASSERT(worker_queue->get_size() == 0);

	// controller thread must be joined beforehood,
	// or `consumers` can be accessed by both controller
	// and controller thread
	for (Consumer *c: consumers) {
		c->detach();
		c->cancel();
	}

	// free consumer pointer and end consumer thread
	// operation on consumer is PROHIBITED after this
	for (Consumer *unused: consumers)
		worker_queue->enqueue(nullptr);
}

void ConsumerController::start() {
	pthread_create(&t, 0, &ConsumerController::process, (void*)this);
}

void* ConsumerController::process(void* arg) {
	ConsumerController *ctrler = static_cast<ConsumerController *>(arg);
	while (true) {
		std::chrono::microseconds period(ctrler->check_period);
		std::this_thread::sleep_for(period);

		TSQueue<Item*> *worker_q = ctrler->worker_queue;
		int ratio = worker_q->get_size() * 100 / worker_q->capacity();
		if (ratio > ctrler->high_threshold) {
			ctrler->consumers.emplace_back(new Consumer(
					ctrler->worker_queue,
					ctrler->writer_queue,
					ctrler->transformer));
			ctrler->consumers.back()->start();
			std::cout << "Scaling up consumers, total # = "
					  << ctrler->consumers.size() << std::endl;
		} else if (ratio < ctrler->low_threshold &&
				   ctrler->consumers.size() > 1) {
			Consumer *c = ctrler->consumers.back();
			c->detach();
			c->cancel();
			ctrler->consumers.pop_back();
			std::cout << "Scaling down consumers, total # = "
					  << ctrler->consumers.size() << std::endl;
		}
	}
}

#endif // CONSUMER_CONTROLLER_HPP
