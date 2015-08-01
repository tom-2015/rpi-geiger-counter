#ifndef _LOCK_FREE_FIFO_QUEUE
#define _LOCK_FREE_FIFO_QUEUE

#include <stddef.h>
#include <pthread.h>

//provides a threadsafe FIFO buffer for pthreads

template <typename T>
class FifoQueue {
private:  
	struct Node {    
		Node( T val ) : value(val), next(NULL) { 
		}    
		T value;    
		Node* next;  
	};  
	
	Node * readptr;
	Node * peekptr;
	Node * writeptr;

	int cnt;
	pthread_mutex_t mutex_node_data;

public:  
	FifoQueue();
	~FifoQueue();

	//add item
	void write( const T& t );

	//Read without remove from queue
	bool peek (T & result);

	//read data and remove from queue
	bool read( T& result );

	//return elements in fifo
	int count ();
};

#endif