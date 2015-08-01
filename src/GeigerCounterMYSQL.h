#ifndef _MYSQL_CLIENT
#define _MYSQL_CLIENT

#include <time.h>
#include <string>
#include "USBGeigerCounter.h"
#include "fifo.h"
#include "GeigerCounterApp.h"

class MYSQLclient: public GeigerCounterExtension {
private:
	std::string name;

	static void * client_thread_helper(void * param);
	void start_upload();
	void upload_buffer();
	bool keep_uploading;
	pthread_t client_thread;
	FifoQueue<GeigerCounterIntervalData> data_fifo;

	std::string mysql_host;  //host
	std::string mysql_user;  //user
	std::string mysql_pwd;   //password
	std::string mysql_db;    //database to use
	std::string mysql_table;
	std::string mysql_insert_query; //holds query string
	bool enabled;

public: 
	MYSQLclient (GeigerCounterApplication * app);
	~MYSQLclient();

	void enable();
	void disable();
	bool is_enabled(); //returns if enabled or not
	std::string get_status_json(); //returns a json object giving the status parameters of the extension
	void on_interval(const GeigerCounterIntervalData & data);
	void on_packet_received (const USBGeigerCounterPacket & data);
	void read_setting (const std::string & key, const std::string & value);
	const char * get_name();
};

#endif