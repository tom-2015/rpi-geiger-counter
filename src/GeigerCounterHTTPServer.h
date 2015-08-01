#ifndef _HTTP_SERVER_EXTENSION
#define _HTTP_SERVER_EXTENSION

#include "GeigerCounterApp.h"
#include "httpserver.h"
#include "USBGeigerCounter.h"
#include <string>
#include <pthread.h>
#include <vector>

class HTTPServerExtension : public GeigerCounterExtension, public http_server_listener {
	std::string name;
	int listen_port;
	bool enable_http_server;
	bool enabled;
	bool http_www_enabled;
	pthread_mutex_t mutex_last_packet, mutex_measured_data_list; //to protect data
	USBGeigerCounterPacket last_packet;
	std::string www_dir;
	std::vector<GeigerCounterIntervalData> measured_data_list; //holds the last measured data
	GeigerCounterApplication * app;

public:
	http_server * server;
	HTTPServerExtension (GeigerCounterApplication * app);
	~HTTPServerExtension();

	void on_interval(const GeigerCounterIntervalData & data);
	void on_packet_received (const USBGeigerCounterPacket & data);
	void enable();
	void disable();
	bool is_enabled(); //returns if enabled or not
	std::string get_status_json(); //returns a json object giving the status parameters of the extension

	void read_setting (const std::string & key, const std::string & value);
	const char * get_name();

	void http_event (http_server * server, http_server_event * e); //http connection received
};

#endif