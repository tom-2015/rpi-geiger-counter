#ifndef _HTTP_SERVER_H
#define _HTTP_SERVER_H

#include <stdlib.h>
#include <list>
#include <map>
#include <string.h>
#include "PracticalSocket.h"
#include <pthread.h>

typedef enum {
	HTTP_EVENT_REQUEST
} HTTP_SERVER_EVENTS;

class http_server;

class http_server_event {
public:
	HTTP_SERVER_EVENTS type;
	string * request_method;
	string * request_url;
	string * http_version;
	
	map<string, string> * request_headers;

	http_server_event(HTTP_SERVER_EVENTS type);
	~http_server_event();
};

class http_server_listener {
public:
	virtual void http_event (http_server * server, http_server_event * e)=0;
};

//very basic http server class
class http_server {
private:
	static void * thread_helper (void * param);		 //helper for multithreading
	void listen ();
	void send_event (http_server_event * e); //sends event to all listeners

	pthread_t listen_thread;		
	bool keep_listening;				

	TCPServerSocket serverSock;     // Server Socket object
	std::list<http_server_listener*> listeners; //all event listeners
	TCPSocket * sock; //connection socket object
public:
	void http_bad_request(); //sends back bad request
	void http_ok(bool flush_headers); //send OK header and if flush_headers is set to true, add \r\n\r\n, ready to send html text
	void http_ok(bool flush_headers, int content_length);
	void http_ok(bool flush_headers, int content_length, const char * headers);
	void http_not_found();

	void send_data(int data);
	void send_data(int data, int radix);
	void send_data(const char * data); //send null terminated string back
	void send_data(const char * data, unsigned int size);
	void send_data(const string & data);
	void send_data(const string * data);

	std::string get_mime_type(std::string const & extension);

	void add_listener (http_server_listener * listener); //adds listener to the internal listener array

	http_server(unsigned short port);
	~http_server();
	
	void close();


};


#endif