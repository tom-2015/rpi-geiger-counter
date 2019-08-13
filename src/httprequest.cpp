#include "httprequest.h"
#include "tinyxml2.h"
#include <string>

using namespace std;

#ifdef HTTPRequestEnableXML
	using namespace tinyxml2;
#endif

HTTPRequest::HTTPRequest(){
	req_method = HTTPRequestGet;
	multi_handle=NULL;
	curl=NULL;
	formpost=NULL;
	headerlist=NULL;
	lastptr=NULL;
	#ifdef HTTPRequestEnableXML
		xml_doc=NULL;
	#endif
	reset();
}

HTTPRequest::~HTTPRequest(){
	reset();
}

void HTTPRequest::reset(){
	if (multi_handle!=NULL) curl_multi_cleanup(multi_handle);
	if (curl!=NULL) curl_easy_cleanup(curl);// always cleanup
	if (formpost!=NULL) curl_formfree(formpost);// then cleanup the formpost chain
	if (headerlist!=NULL) curl_slist_free_all (headerlist);// free slist 

	req_method = HTTPRequestGet;
	lastptr=NULL;
	multi_handle=NULL;
	curl=NULL;
	formpost=NULL;
	headerlist=NULL;
	received_data_length=0;
	received_data_buffer_size=0;
	curl_result=CURLE_OK;

	curl = curl_easy_init();//initialize the handles
	multi_handle = curl_multi_init();
	curl_multi_add_handle(multi_handle, curl);

	#ifdef HTTPRequestEnableXML
		if (xml_doc!=NULL){
			delete xml_doc;
			xml_doc = NULL;
		}
	#endif
}

void HTTPRequest::add_form_parameter(const char * name, const char * value){
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, name, CURLFORM_COPYCONTENTS, value, CURLFORM_END);
}

void HTTPRequest::add_form_parameter(const char * name, const void * data, size_t length){
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, name, CURLFORM_PTRCONTENTS, data,CURLFORM_BUFFERLENGTH, (long) length, CURLFORM_END);
}

void HTTPRequest::add_header (const char * header){
	headerlist = curl_slist_append(headerlist, header); // initalize custom header list (stating that Expect: 100-continue is notwanted
}

void HTTPRequest::set_proxy_server (const char * server_port){ //server:port
	curl_easy_setopt(curl, CURLOPT_PROXY, server_port);
}

void HTTPRequest::set_proxy_port(int port){
	curl_easy_setopt(curl, CURLOPT_PROXYPORT, (long) port);
}

void HTTPRequest::set_proxy_user_name(const char * name){
	curl_easy_setopt(curl, CURLOPT_PROXYUSERNAME, name);
}

void HTTPRequest::set_proxy_password(const char * pwd){
	curl_easy_setopt(curl, CURLOPT_PROXYPASSWORD, pwd);
}

void HTTPRequest::set_proxy_server_user_name_pwd (const char * user_pwd){ //username:password
	curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, user_pwd);
}

void HTTPRequest::set_http_user_name(const char * user){
	curl_easy_setopt(curl, CURLOPT_USERNAME , user);
}

void HTTPRequest::set_http_password(const char * pwd){
	curl_easy_setopt(curl, CURLOPT_PASSWORD, pwd);
}

void HTTPRequest::set_http_user_name_pwd(const char * user_pwd){
	curl_easy_setopt(curl, CURLOPT_USERPWD , user_pwd);
}

void HTTPRequest::verbose (int value){
	curl_easy_setopt(curl, CURLOPT_VERBOSE, value);
}

void HTTPRequest::set_request_type(HTTPRequestMethods reqmethod){
	req_method = reqmethod;
}

void HTTPRequest::set_http_authentication(HTTPAuthenticationConstants auth){
	curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)auth);
}

void HTTPRequest::set_https_no_peer_verification(){
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
}

void HTTPRequest::set_https_no_host_verification(){
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
}

bool HTTPRequest::send (const char * url){
	return send(url, true);
}

bool HTTPRequest::send (const char * url, bool async){ //downloads the url
	still_running = false;
	switch (req_method){
		case HTTPRequestGet:
		case HTTPRequestPut:
		case HTTPRequestHead:
			curl_easy_setopt(curl, (CURLoption)req_method, (long) 1);
			break;
		case HTTPRequestPost:
			curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost); //pass the form parameter
			break;
	}
	if (headers != NULL && curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers) != CURLE_OK) return false;
	if (curl_easy_setopt(curl, CURLOPT_URL, url)!=CURLE_OK) return false;// what URL that receives this POST 
	if (headerlist!=NULL && curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist)!=CURLE_OK) return false;
	if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HTTPRequest::write_http_data)!=CURLE_OK) return false; //set the write function that receives the data
	if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, this)!=CURLE_OK) return false; //set a parameter (pointer to ostringstream) where the response data will be saved
	if (curl_multi_perform(multi_handle, &still_running)!=CURLM_OK) return false;
	if (async) return true;
	while (!completed());
	return curl_result == CURLE_OK;
}

bool HTTPRequest::completed (){
	struct timeval timeout;
	fd_set fdread,fdwrite,fdexcep;
	int maxfd = -1;

	//long curl_timeo = -1;

	FD_ZERO(&fdread);
	FD_ZERO(&fdwrite);
	FD_ZERO(&fdexcep);

	timeout.tv_sec = 0; //set a suitable default timeout 1ms
	timeout.tv_usec = 1000;

	/*curl_multi_timeout(multi_handle, &curl_timeo);
	if(curl_timeo >= 0) {
		timeout.tv_sec = curl_timeo / 1000;
		if(timeout.tv_sec > 1)
			timeout.tv_sec = 1;
		else
			timeout.tv_usec = (curl_timeo % 1000) * 1000;
	}*/

	// get file descriptors from the transfers
	curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

	if (select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout)!=-1){
		curl_multi_perform(multi_handle, &still_running);
	}else{
		still_running=0;
	}

	int msgs_in_queue;
	CURLMsg * msg = curl_multi_info_read(multi_handle,   & msgs_in_queue); 
	while (msgs_in_queue > 0 && msg!=NULL){
		if (msg->msg==CURLMSG_DONE){
			still_running=0;
			curl_result = msg->data.result;
		}
		msg = curl_multi_info_read(multi_handle, & msgs_in_queue);
	}

	return still_running==0;
}

bool HTTPRequest::http_response_ok(){
	return http_response_code()==200 && curl_result == CURLE_OK;
}

int HTTPRequest::http_response_code(){
	long http_code = 0;
	curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
	return (int)http_code;
}

void HTTPRequest::set_http_header(const char * header_value) {
	headers = curl_slist_append(headers, header_value);
}

CURLcode HTTPRequest::curl_response_code(){
	return curl_result;
}

unsigned char * HTTPRequest::response_data(){
	return received_data;
}

size_t HTTPRequest::response_length(){
	return received_data_length;
}

const char * HTTPRequest::curl_error(){
	return curl_easy_strerror(curl_result);
}

size_t HTTPRequest::write_http_data(char *data_ptr, size_t size, size_t nmemb, void * HTTPRequest_cls_ptr){
	HTTPRequest * client = (HTTPRequest *) HTTPRequest_cls_ptr;
	size_t n_bytes = size * nmemb;
	if (n_bytes > 0){
		size_t size_needed = client->received_data_length + n_bytes + 1; //min. buffer size needed
		if (size_needed > client->received_data_buffer_size){ //dynamically expand the buffer
			
			client->received_data_buffer_size = client->received_data_buffer_size * 2; //increase the buffer size
			if (client->received_data_buffer_size < size_needed) client->received_data_buffer_size = size_needed; //check if we have enough space by doubling the buffer size

			unsigned char * new_buffer = new unsigned char [client->received_data_buffer_size];
			
			if (client->received_data_length>0){
				memcpy((void*) new_buffer, (void*)client->received_data, client->received_data_length);
				delete client->received_data;
			}
			
			client->received_data = new_buffer;
		}
		memcpy((void*) & client->received_data[client->received_data_length],(void*) data_ptr, n_bytes);
		client->received_data_length += n_bytes; 
		client->received_data[client->received_data_length]=0; //terminate with a 0
	}
	return n_bytes;
}

#ifdef HTTPRequestEnableXML
XMLDocument * HTTPRequest::response_xml(){
	if (xml_doc==NULL){
		xml_doc = new XMLDocument();
		xml_doc->Parse((const char *) received_data);
	}
	return xml_doc;
}
#endif