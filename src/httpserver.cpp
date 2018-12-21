#include "httpserver.h"
#include <pthread.h>
#include <string>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <ctype.h>
#include <unistd.h>
#include "PracticalSocket.h"

using namespace std;

#define HTTP_BAD_REQUEST "HTTP/1.1 400 Bad Request\r\n\r\n<h1>400 Bad Request</h1>"
#define HTTP_200_OK "HTTP/1.1 200 OK\r\n"
#define HTTP_NOT_FOUND "HTTP/1.1 404 Not Found\r\n\r\n<h1>404 Page not found</h1>"

map<string, string> mime_types;

typedef enum {
	HTTP_STATE_GET_METHOD,
	HTTP_STATE_GET_URL,
	HTTP_STATE_GET_VERSION,
	HTTP_STATE_GET_HEADER_NAME,
	HTTP_STATE_GET_HEADER_VALUE,
	HTTP_STATE_BAD_REQUEST,
	HTTP_STATE_HEADERS_PROCESSED
} HTTP_SERVER_STATES;

void load_mime_types () {
	mime_types["avi"]="video/avi";
	mime_types["css"]="text/css";
	mime_types["exe"]="application/octet-stream";
	mime_types["gif"]="image/gif";
	mime_types["htm"]="text/html";
	mime_types["html"]="text/html";
	mime_types["jpg"]="image/jpeg";
	mime_types["jpeg"]="image/jpeg";
	mime_types["js"]="application/javascript";
	mime_types["json"]="application/json";
	mime_types["log"]="text/plain";
	mime_types["mid"]="audio/midi";
	mime_types["mov"]="video/quicktime";
	mime_types["mp3"]="audio/mpeg3";
	mime_types["mpeg"]="video/mpeg";
	mime_types["png"]="image/png";
	mime_types["txt"]="text/plain";
	mime_types["zip"]="application/x-compressed";
}

http_server::http_server(unsigned short port): serverSock(port){
	if (mime_types.size()==0){
		load_mime_types();
	}
	keep_listening=true;
	pthread_create(& this->listen_thread, NULL, (void* (*)(void*)) & http_server::thread_helper, this);
}

http_server::~http_server(){
	close();
}

void http_server::close(){
	this->keep_listening = false; //we stop listening
	pthread_join(this->listen_thread, NULL); //wait for the thread to exit
	this->serverSock.Close();
}

void * http_server::thread_helper (void * param){
	((http_server *) param)->listen();
}

void http_server::http_bad_request(){
	send_data(HTTP_BAD_REQUEST);
}

void http_server::http_not_found(){
	send_data(HTTP_NOT_FOUND);
}

void http_server::http_ok(bool flush_headers){
	send_data(HTTP_200_OK);
	if (flush_headers) send_data("\r\n");
}

string http_server::get_mime_type(string const & extension){
	string ext_to_lower;
	if (extension.length()>0){
		//convert to lower case
		char lower[2];
		lower[1] = 0;
		for (int i=0; i<extension.length();i++){
			lower[0] = (char)tolower((int)extension[i]);
			ext_to_lower.append(lower);
		}
		map<string,string>::iterator mime_type_iterator;
		mime_type_iterator = mime_types.find(ext_to_lower);
		if (mime_type_iterator!=mime_types.end()){
			return mime_types[ext_to_lower];
		}
	}
	return "";
}
    
void http_server::http_ok(bool flush_headers, int content_length){
	send_data(HTTP_200_OK);
	send_data("Content-Length: ");
	send_data(content_length);
	send_data("\r\n");
	if (flush_headers) send_data("\r\n");	
}

//send with content_length and additional headers (must end with \r\n)
void http_server::http_ok(bool flush_headers, int content_length, const char * headers){
	send_data(HTTP_200_OK);
	send_data("Content-Length: ");
	send_data(content_length);
	send_data("\r\n");
	send_data(headers); 
	if (flush_headers) send_data("\r\n");	
}

void http_server::send_data(int data){
	send_data(data, 10);
}

void http_server::send_data(int data, int radix){
	char str[33];
	str[0]=0;
	sprintf(str, "%d", data);
	this->sock->send(str,strlen(str));
}

void http_server::send_data(const char * data){
	this->sock->send(data, strlen(data));
}

void http_server::send_data(const char * data, unsigned int size){
	this->sock->send(data, size);
}

void http_server::send_data(const string & data){
	this->sock->send(data.c_str(), data.length());
}

void http_server::send_data(const string * data){
	this->sock->send(data->c_str(), data->length());
}

void http_server::listen (){
	this->serverSock.SetNonBlocking();

	while (this->keep_listening){ //we keep listening until we need to unload the class (keep_listening becomes false)
		try {
			sock = this->serverSock.accept();
			if (sock !=NULL){

				char buffer[1024]; // receive data

				int recvMsgSize;
				HTTP_SERVER_STATES state=HTTP_STATE_GET_METHOD;

				string request_method;
				string request_url;
				string http_version;
				string header_name;
				string header_value;
				map<string, string> request_headers; //contains request headers

				while ((recvMsgSize = sock->recv(buffer, 1024)) > 0) { // Zero means  end of transmission
					for (int i=0;i<recvMsgSize;i++){
						char data = buffer[i];

						if (data!='\n'){ //we do not process \n characters
							switch (state){
								case HTTP_STATE_GET_METHOD:
									if (data==' '){
										state=HTTP_STATE_GET_URL;
									}else if (data=='\r' || data=='\n'){
										state=HTTP_STATE_BAD_REQUEST;
									}else{
										request_method.push_back(data);
									}
									break;
								case HTTP_STATE_GET_URL:
									if (data==' '){
										state=HTTP_STATE_GET_VERSION;
									}else if (data=='\r' || data=='\n'){
										state=HTTP_STATE_BAD_REQUEST;
									}else{
										request_url.push_back(data);
									}
									break;
								case HTTP_STATE_GET_VERSION:
									if (data=='\r'){
										header_name.clear();
										state=HTTP_STATE_GET_HEADER_NAME;
									}else if (data=='\r' || data=='\n'){
										state=HTTP_STATE_BAD_REQUEST;
									}else{
										http_version.push_back(data);
									}
									break;
								case HTTP_STATE_GET_HEADER_NAME:
									if (data==':' || data=='\r'){

										header_value.clear();
										if (header_name.length()==0 && data=='\r') state=HTTP_STATE_HEADERS_PROCESSED; //all processed
										else state=HTTP_STATE_GET_HEADER_VALUE;
									}else{
										header_name.push_back(data);
									}
									break;
								case HTTP_STATE_GET_HEADER_VALUE:
									if (data=='\r'){

										request_headers.insert(std::pair<string,string>(header_name,header_value));
										header_name.clear();
										state = HTTP_STATE_GET_HEADER_NAME; //get next header name
									}else{
										header_value.push_back(data);
									}
									break;
								case HTTP_STATE_HEADERS_PROCESSED:
									break;
								case HTTP_STATE_BAD_REQUEST:

									break;
							}
						}
						if (state==HTTP_STATE_HEADERS_PROCESSED || state==HTTP_STATE_BAD_REQUEST) break;
					}
					if (state==HTTP_STATE_HEADERS_PROCESSED || state==HTTP_STATE_BAD_REQUEST) break;

				}
				

				if (state==HTTP_STATE_HEADERS_PROCESSED){
					http_server_event e(HTTP_EVENT_REQUEST);

					e.request_method = & request_method;
					e.request_url = & request_url;
					e.http_version = & http_version;
					e.request_headers = & request_headers;
					
					send_event (& e);
				}

				if (state==HTTP_STATE_BAD_REQUEST){
					http_bad_request(); //send bad request error
				}
				
				delete sock;
			}else{
				usleep(1000); //no connections
			}
		}catch (SocketException e) {
			cout << "socket exception: " << e.what() << endl;
		}
	}
	pthread_exit(NULL); //exit
	return;
}

void http_server::send_event (http_server_event * e){
	for (std::list<http_server_listener*>::iterator it = this->listeners.begin(); it != this->listeners.end(); it++) (*it)->http_event(this, e);
}

void http_server::add_listener (http_server_listener * listener){
	this->listeners.push_back(listener);
}

http_server_event::http_server_event(HTTP_SERVER_EVENTS type){
	this->type = type;
}

http_server_event::~http_server_event(){

}
