#include <fstream>
#include "thread.hpp"
#include "ts_queue.hpp"
#include "item.hpp"

#ifndef WRITER_HPP
#define WRITER_HPP

class Writer : public Thread {
public:
	// constructor
	Writer(int expected_lines, std::string output_file, TSQueue<Item*>* output_queue);

	// destructor
	~Writer();

	virtual void start() override;
private:
	// the expected lines to write,
	// the writer thread finished after output expected lines of item
	int expected_lines;

	std::ofstream ofs;
	TSQueue<Item*> *output_queue;

	// the method for pthread to create a writer thread
	static void* process(void* arg);
};

// Implementation start

Writer::Writer(int expected_lines, std::string output_file, TSQueue<Item*>* output_queue)
	: expected_lines(expected_lines), output_queue(output_queue) {
	ofs = std::ofstream(output_file);
}

Writer::~Writer() {
	ofs.close();
}

void Writer::start() {
	pthread_create(&t, 0, &Writer::process, (void*)this);
}

void* Writer::process(void* arg) {
	Writer *writer = static_cast<Writer *>(arg);
	while (writer->expected_lines--) {
		Item *it = writer->output_queue->dequeue();
		writer->ofs << *it;
		delete it;
	}
	return nullptr;
}

#endif // WRITER_HPP
