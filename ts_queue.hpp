#include <pthread.h>
#include <vector>
#include "debug.hpp"

#ifndef TS_QUEUE_HPP
#define TS_QUEUE_HPP

#define DEFAULT_BUFFER_SIZE 200

template <class T>
class TSQueue {
public:
	// constructor
	TSQueue();

	explicit TSQueue(int max_buffer_size);

	// destructor
	~TSQueue();

	// add an element to the end of the queue
	void enqueue(T item);

	// remove and return the first element of the queue
	T dequeue();

	// return the number of elements in the queue
	int get_size();

	int capacity() const;
private:
	// the maximum buffer size
	int buffer_size;
	// the buffer containing values of the queue
	std::vector<T> buffer;
	// the current size of the buffer
	int size;
	// the index of first item in the queue
	int head;
	// the index of last item in the queue
	int tail;

	// pthread mutex lock
	pthread_mutex_t mutex;
	// pthread conditional variable
	pthread_cond_t cond_enqueue, cond_dequeue;
};

// Implementation start

template <class T>
TSQueue<T>::TSQueue() : TSQueue(DEFAULT_BUFFER_SIZE) {
}

template <class T>
TSQueue<T>::TSQueue(int buffer_size)
	  : buffer_size(buffer_size), buffer(buffer_size),
		size(0), head(0), tail(0) {
	ASSERT(buffer_size >= 0);
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond_enqueue, NULL);
	pthread_cond_init(&cond_dequeue, NULL);
}

template <class T>
TSQueue<T>::~TSQueue() {
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond_enqueue);
	pthread_cond_destroy(&cond_dequeue);
}

template <class T>
void TSQueue<T>::enqueue(T item) {
	pthread_mutex_lock(&mutex);
	while (size == buffer_size) {
		pthread_cond_wait(&cond_enqueue, &mutex);
	}

	buffer[tail] = item;
	tail = ++tail % buffer_size;
	size += 1;

	ASSERT(size <= buffer_size);
	ASSERT(size == (tail >= head) ? tail - head : tail - head + buffer_size);

	pthread_cond_signal(&cond_dequeue);
	pthread_mutex_unlock(&mutex);
}

template <class T>
T TSQueue<T>::dequeue() {
	pthread_mutex_lock(&mutex);
	while (size == 0) {
		pthread_cond_wait(&cond_dequeue, &mutex);
	}

	T ret = buffer[head];
	head = ++head % buffer_size;
	size -= 1;

	ASSERT(size >= 0);
	ASSERT(size == (tail >= head) ? tail - head : tail - head + buffer_size);

	pthread_cond_signal(&cond_enqueue);
	pthread_mutex_unlock(&mutex);

	return ret;
}

template <class T>
int TSQueue<T>::get_size() {
	pthread_mutex_lock(&mutex);
	int ret = size;
	pthread_mutex_unlock(&mutex);
	return ret;
}

template <class T>
int TSQueue<T>::capacity() const {
	return buffer_size;
}

#endif // TS_QUEUE_HPP
