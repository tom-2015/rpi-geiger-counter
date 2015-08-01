#ifndef _GEIGERCOUNTER_TEXT_FILE_EXTENSION
	#define _GEIGERCOUNTER_TEXT_FILE_EXTENSION
	
#include <time.h>
#include <string>
#include "USBGeigerCounter.h"
#include "fifo.h"
#include "GeigerCounterApp.h"

class TextFileExtension: public GeigerCounterExtension {
private:
	std::string name;

	static void * client_thread_helper(void * param);
	bool keep_saving;
	void start_saving();
	pthread_t client_thread;
	FifoQueue<GeigerCounterIntervalData> data_fifo;

	std::string dir; //where to save the text files
	std::string exec_cmd; //command to execute after saving files
	bool enabled;
	unsigned int file_index; //holds index for next file name, will count up until application is shut down

public: 
	TextFileExtension (GeigerCounterApplication * app);
	~TextFileExtension();

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