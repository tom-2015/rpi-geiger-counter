#ifndef _HTTP_REQUEST
#define _HTTP_REQUEST

#define HTTPRequestEnableXML 1

#ifdef HTTPRequestEnableXML
 #include "tinyxml2.h" 
#endif
#include <curl/curl.h>

typedef enum {
	HTTPAuthenticateBasic = CURLAUTH_BASIC , //basic authentication
	HTTPAuthenticateDigest = CURLAUTH_DIGEST, //digest authentication
	HTTPAuthenticateDigestIE = CURLAUTH_DIGEST_IE, //IE version of digest
	HTTPAuthenticateAny = CURLAUTH_ANY, //choose the safest way
	HTTPAuthenticateAnySafe = CURLAUTH_ANYSAFE //any without basic
} HTTPAuthenticationConstants;

typedef enum {
	HTTPRequestGet = CURLOPT_HTTPGET,
	HTTPRequestPost = CURLOPT_HTTPPOST,
	HTTPRequestHead = CURLOPT_NOBODY,
	HTTPRequestPut = CURLOPT_PUT
} HTTPRequestMethods;

class HTTPRequest {
	HTTPRequestMethods req_method;
	CURL *curl;
	CURLM *multi_handle;
	struct curl_httppost *formpost;
	struct curl_httppost *lastptr;
	struct curl_slist *headerlist;
	unsigned char * received_data;
	size_t received_data_length, received_data_buffer_size;
	static size_t write_http_data(char *data_ptr, size_t size, size_t nmemb, void * http_client_cls_ptr); //here the http download response is received and written to the stream
	int still_running;
	CURLcode curl_result;

	struct curl_slist * headers = NULL;

	#ifdef HTTPRequestEnableXML
		tinyxml2::XMLDocument * xml_doc;
	#endif

public:
	HTTPRequest ();
	~HTTPRequest();
	void reset();

	void add_form_parameter(const char * name, const char * value);
	void add_form_parameter(const char * name, const void * data, size_t length);
	void add_header (const char * header);
	void set_proxy_server (const char * server); //server:port
	void set_proxy_port(int port);
	void set_proxy_user_name(const char * name);
	void set_proxy_password(const char * pwd);
	void set_proxy_server_user_name_pwd (const char * user_pwd); //username:password
	void set_http_user_name(const char * user);
	void set_http_password(const char * pwd);
	void set_http_user_name_pwd(const char * user_pwd); //username:password
	void set_http_authentication(HTTPAuthenticationConstants auth);
	void set_https_no_host_verification();
	void set_https_no_peer_verification();
	void set_http_header(const char * header_value);

	void set_request_type(HTTPRequestMethods reqmethod);

	void verbose (int value); //print debug code on cout
		
	bool send (const char * url);  //start downloading the url
	bool send (const char * url, bool async); //downloads with optional async parameter
	bool completed ();		 //returns true if the operation has completed
	bool http_response_ok(); //checks curl_repsonse_code = CURLE_OK and http_response_code == 200
	CURLcode curl_response_code(); //returns the curl response code, ok =  CURLE_OK
	int http_response_code();      //returns the http response code  ok = 200

	const char * curl_error();     //in case curl_response_code != CURLE_OK get an error description string here

	unsigned char * response_data(); //returns a pointer to the received http data buffer
	size_t response_length(); //returns the received data size
	#ifdef HTTPRequestEnableXML
		tinyxml2::XMLDocument * response_xml();
	#endif
};
#endif