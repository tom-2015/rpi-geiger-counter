#ifndef _GEIGER_COUNTER_APP_H
#define _GEIGER_COUNTER_APP_H
#include "USBGeigerCounter.h"
#include "log.h"

#include <string>
#include <vector>

typedef struct {
	int counts;
	time_t start_time;
	time_t end_time;
	float radiation; //radiation level in µS/h
	int cpm; //counts per min.
	USBGeigerCounterPacket last_packet;
}GeigerCounterIntervalData;

class GeigerCounterApplication;

//extension for the main application
class GeigerCounterExtension {
protected:
	GeigerCounterApplication * app;
public:
	GeigerCounterExtension(GeigerCounterApplication * app);
	~GeigerCounterExtension();
	virtual void on_interval(const GeigerCounterIntervalData & data)=0; //called when the sampling interval is passed (SAMPLING_INTERVAL_TIME =300s)
	virtual void on_packet_received (const USBGeigerCounterPacket & data)=0; //called for every packet received from USB interface
	virtual	void enable()=0; //called to enable
	virtual void disable()=0; //called to disable
	virtual bool is_enabled()=0; //returns if enabled or not
	virtual std::string get_status_json()=0; //returns a json object giving the status parameters of the extension
	virtual void read_setting (const std::string & key, const std::string & value)=0; //called at startup to read settings
	virtual const char * get_name()=0; //must return a unique name
};

//main application class
class GeigerCounterApplication {
private:
	int interval;
	bool use_remote_watchdog;
	std::string log_file; //location of the log file
	std::string config_file; //location of the config file
	std::vector<GeigerCounterExtension*> Extensions; //all extensions loaded

	time_t next_sampling_time, previous_sampling_time;
	int p_counter_value;
	int interval_counts;
	USBGeigerCounterPacket last_packet; //last_packet is last received respons from geiger counter and http_packet is the same but only used by http server for sending status
	bool firstpacket; //true if this is the first packet we received after a reboot of the program


public:
	USBGeigerCounter counter; //object to the geiger counter interface class
	
	GeigerCounterApplication ();
	~GeigerCounterApplication();

	void packet_received (const USBGeigerCounterPacket & packet);
	void load_settings(const char * file);
	void change_setting(const char * name, const char * value); //edits config file to change a value
	void add_extension (GeigerCounterExtension * ext);
	GeigerCounterExtension * get_extension(int index);
	int get_extension_count();
	void set_sampling_interval(int new_interval);
	int  get_sampling_interval(); //sample interval
	bool get_use_remote_watchdog();
	
	unsigned int get_interval_counts ();
	unsigned int get_total_counts();
	
	time_t get_next_sampling_time();
};

#endif