#include <pthread.h>
#include <stdio.h>
#include <atomic>
#include "thread.hpp"
#include "ts_queue.hpp"
#include "item.hpp"
#include "transformer.hpp"

#ifndef CONSUMER_HPP
#define CONSUMER_HPP

class Consumer : public Thread {
public:
	// constructor
	Consumer(TSQueue<Item*>* worker_queue, TSQueue<Item*>* output_queue, Transformer* transformer);

	// destructor
	~Consumer();

	virtual void start() override;

	virtual int cancel() override;
private:
	TSQueue<Item*>* worker_queue;
	TSQueue<Item*>* output_queue;

	Transformer* transformer;

	std::atomic<bool> is_cancel;

	// the method for pthread to create a consumer thread
	static void* process(void* arg);
};

Consumer::Consumer(TSQueue<Item*>* worker_queue, TSQueue<Item*>* output_queue, Transformer* transformer)
	: worker_queue(worker_queue), output_queue(output_queue), transformer(transformer) {
	is_cancel = false;
}

Consumer::~Consumer() {}

void Consumer::start() {
	pthread_create(&t, 0, &Consumer::process, (void*)this);
}

int Consumer::cancel() {
	is_cancel = true;
	int ret = pthread_cancel(t);
	return ret;
}

void* Consumer::process(void* arg) {
	Consumer* consumer = (Consumer*)arg;

	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);

	while (!consumer->is_cancel) {
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);

		Item *it = consumer->worker_queue->dequeue();
		if (!it) {
			continue;
		}
		it->val = consumer->transformer->consumer_transform(it->opcode, it->val);
		consumer->output_queue->enqueue(it);

		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
	}

	delete consumer;

	return nullptr;
}

#endif // CONSUMER_HPP
