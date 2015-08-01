#ifndef HTTP_CLIENT_EXTENSION
#define HTTP_CLIENT_EXTENSION

#include "GeigerCounterApp.h"
#include "fifo.h"
#include <pthread.h>
#include <string>

class HTTPClient: public GeigerCounterExtension {
	std::string name;
	std::string password, username, posturl, filename, proxy_server_port, proxy_username_password, counter_id, counter_password;
	int max_upload_packets; //max nr of samples uploaded in 1 request
	int timeout;
	int interval;
	int public_port; //public access port for /json
	time_t last_upload_time;
	int last_http_response_code;
	
	bool http_upload_enabled;
	bool enabled;
	bool keep_uploading;

	pthread_t upload_thread;
	FifoQueue<GeigerCounterIntervalData> packets;
	static void * upload_thread_helper(void * param);

public:
	HTTPClient(GeigerCounterApplication * app);
	~HTTPClient();

	void on_interval(const GeigerCounterIntervalData & data); //called when the measure interval is passed
	void on_packet_received (const USBGeigerCounterPacket & data); //called for every packet received from USB interface
	void enable();
	void disable();
	bool is_enabled(); //returns if enabled or not
	std::string get_status_json(); //returns a json object giving the status parameters of the extension

	const char * get_name();
	void read_setting (const std::string & key, const std::string & value);
	void upload();
	bool get_http_client_enabled(); //returns if enabled
	int get_http_response_code(); //returns last http upload response code
	time_t get_last_upload_time(); //returns last time data was succesfully uploaded
};

#endif