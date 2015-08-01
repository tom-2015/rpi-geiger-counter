#include "fifo.h"
#include "GeigerCounterApp.h"
//#include <iostream>

using namespace std;

template<typename T>
FifoQueue<T>::FifoQueue(){   
	cnt=0;
	peekptr = readptr = writeptr = NULL; // new Node( T() );           // add dummy separator 
	pthread_mutex_init(& this->mutex_node_data, NULL);
}

template<typename T>
FifoQueue<T>::~FifoQueue() {    
	while( readptr != NULL ) {   // release the list      
		Node* tmp = readptr;      
		readptr = tmp->next;      
		delete tmp;    
	}  
}

//add item
template<typename T>
void FifoQueue<T>::write( const T& t ) {  
	pthread_mutex_lock(& mutex_node_data);
	if (writeptr == NULL){ //empty list
		writeptr = new Node(t);
	}else{
		writeptr->next = new Node(t); //add to the back
		writeptr = writeptr->next;
	}
	if (peekptr==NULL) peekptr = writeptr;
	if (readptr==NULL) readptr = writeptr;
	
	cnt++;

	pthread_mutex_unlock(& mutex_node_data);
}

//Read without remove from queue
template<typename T>
bool FifoQueue<T>::peek (T & result){
	pthread_mutex_lock(& mutex_node_data);
	if( peekptr != NULL ) {         // if queue is nonempty
		result = peekptr->value;    
		peekptr = peekptr->next; 
		pthread_mutex_unlock(& mutex_node_data);
		return true;   
	}
	pthread_mutex_unlock(& mutex_node_data);
	return false; 
}

//read data and remove from queue
template<typename T>
bool FifoQueue<T>::read( T& result ) {    
	pthread_mutex_lock(& mutex_node_data);
	if( readptr != NULL ) {         // if queue is nonempty      
		result = readptr->value;    //read the value
		Node * tmp = readptr;       //store the item for deleting

		readptr = readptr->next;    //advance the ptr
		peekptr = readptr;			//adnance the peek data ptr
		if (readptr==NULL) writeptr=NULL; //read to the end of the list, reset the write ptr

		delete tmp; //delete the item we just read
		cnt--;
		pthread_mutex_unlock(& mutex_node_data);
		return true;     
	}

	pthread_mutex_lock(& mutex_node_data);
	return false;
}

//return elements in fifo
template<typename T>
int FifoQueue<T>::count (){
	pthread_mutex_lock(& mutex_node_data);
	int res = cnt;
	pthread_mutex_unlock(& mutex_node_data);
	return res;
}

//generate a template for the GeigerCounterIntervalData (data packet for each interval)
template class FifoQueue<GeigerCounterIntervalData>;
