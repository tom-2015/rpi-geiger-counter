#ifndef URAD_CLIENT_EXTENSION
#define URAD_CLIENT_EXTENSION

#include "GeigerCounterApp.h"
#include "fifo.h"
#include <pthread.h>
#include <string>

class UradClient : public GeigerCounterExtension {
	std::string user_id, user_key, sw, hw, posturl, proxy_server_port, proxy_username_password, name;
	int send_interval;
	int timeout;
	int interval;
	time_t last_upload_time;
	int last_http_response_code;
	unsigned int dev_id;
	unsigned int tube_type;
	float cpm_correction;

	bool urad_upload_enabled;
	bool enabled;
	bool keep_uploading;

	bool interval_packet_received;
	pthread_t upload_thread;
	GeigerCounterIntervalData last_interval_packet;
	USBGeigerCounterPacket last_packet;
	pthread_mutex_t last_interval_packet_mutex;
	pthread_mutex_t last_packet_mutex;

	static void * upload_thread_helper(void * param);

public:
	UradClient(GeigerCounterApplication * app);
	~UradClient();

	void on_interval(const GeigerCounterIntervalData & data); //called when the measure interval is passed
	void on_packet_received(const USBGeigerCounterPacket & data); //called for every packet received from USB interface
	void enable();
	void disable();
	bool is_enabled(); //returns if enabled or not
	std::string get_status_json(); //returns a json object giving the status parameters of the extension

	const char * get_name();
	void read_setting(const std::string & key, const std::string & value);
	void upload();
	//bool get_http_client_enabled(); //returns if enabled
	int get_http_response_code(); //returns last http upload response code
	time_t get_last_upload_time(); //returns last time data was succesfully uploaded
};

#endif