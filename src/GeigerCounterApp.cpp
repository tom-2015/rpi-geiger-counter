#include <sstream>
#include <string>
#include "GeigerCounterApp.h"
#include "GeigerCounterHTTPClient.h"
#include "GeigerCounterHTTPServer.h"
#include "GeigerCounterMYSQL.h"
#include "GeigerCounterText.h"
#include "GeigerCounterUradClient.h"
#include "functions.h"
#include "log.h"
#include <iostream>

using namespace std;

GeigerCounterApplication::GeigerCounterApplication (){
	previous_sampling_time=0;
	firstpacket = true;
}

GeigerCounterApplication::~GeigerCounterApplication (){
	//unload all extensions
	Logger::close();
	counter.disconnect();
	/*if (this->Extensions.size() > 3){
		cout << "ERROR > 3";
		cout.flush();
	}*/
	for (std::vector<GeigerCounterExtension*>::iterator it = this->Extensions.begin(); it != this->Extensions.end(); it++){
		delete (*it);
	}
}

void GeigerCounterApplication::add_extension (GeigerCounterExtension * ext){
	Extensions.push_back(ext);
}

//called by main when a packet is received
void GeigerCounterApplication::packet_received (const USBGeigerCounterPacket & packet){
	last_packet = packet;
	if (last_packet.type == USB_DATA){ //check if it is a binary data packet, USB_CMD packets are a response to USB commands (like saving adc calibration,...)
		if (firstpacket){ //first packet after reboot of program, adjust p_counter_value to that of the device
			p_counter_value = last_packet.counter;
			firstpacket=false;
		}

		if (last_packet.counter < p_counter_value){
			p_counter_value = 0; //counter has been reset, we must reset the counter value
		}

		interval_counts += last_packet.counter - p_counter_value;//add any hits that were measured
		p_counter_value = last_packet.counter;
		
		//pass it to the extensions
		for (std::vector<GeigerCounterExtension*>::iterator it = this->Extensions.begin(); it != this->Extensions.end(); it++){
			(*it)->on_packet_received(packet);
		}

		time_t current_time = time(0);
		if (current_time >= next_sampling_time){ //check if we reached the sampling time
			if (previous_sampling_time!=0){
				LOG_DEBUG ("Saving " << interval_counts << " hits. Next interval end: " << format_time(get_next_sampling_time()));
				GeigerCounterIntervalData data;
				data.counts = interval_counts;
				data.start_time = previous_sampling_time;
				data.end_time = next_sampling_time;
				data.last_packet = last_packet;
				data.cpm = 	(float)data.counts / (float)(data.end_time - data.start_time) * 60; //counts per min.
				data.radiation = counter.cpm2rad(data.cpm, calculate_tube_voltage((int)data.last_packet.adc_value, (int)data.last_packet.adc_calibration));
				
				for (std::vector<GeigerCounterExtension*>::iterator it = this->Extensions.begin(); it != this->Extensions.end(); it++){
					LOG_DEBUG ("Saving to " << (*it)->get_name());
					(*it)->on_interval(data);
				}
			}
			
			previous_sampling_time = next_sampling_time; //set the previous time
			next_sampling_time = get_next_sampling_time();

			interval_counts = 0; //reset nr of hits during interval
		}
	}
}

unsigned int GeigerCounterApplication::get_interval_counts (){
	return interval_counts;
}

unsigned int GeigerCounterApplication::get_total_counts (){
	return last_packet.counter;
}

void GeigerCounterApplication::change_setting(const char * name, const char * value) {
	ifstream settings;
	string line;
	string raw_line;
	ostringstream new_data;
	settings.open(config_file.c_str());
	bool value_changed = false;
	if (!settings.fail()) {
		while (getline(settings, raw_line)) {
			bool key_found = false;
			line.assign(raw_line);
			trim(line); //remove spaces
			istringstream is_line(line);
			if (line.length() > 0) {
				if (line[0] != '#' && line[0] != '/') {
					string key;
					if (getline(is_line, key, '=')) {
						if (key.compare(name) == 0) {
							new_data << key << "=" << value << endl;
							key_found = true;
							value_changed = true;
						}
					}
				}
			}
			if (!key_found) new_data << raw_line << endl;
		}
		if (!value_changed) new_data << name << "=" << value;

		settings.close();

		//write changed data to settings file
		ofstream new_settings;

		new_settings.open(config_file.c_str(), std::ofstream::out | std::ofstream::trunc);
		if (!new_settings.fail()) {
			new_settings << new_data.str();
			new_settings.close();
		}

	}
}

//this function opens the file and reads all the settings
void GeigerCounterApplication::load_settings(const char * file){
	ifstream settings;

	config_file.assign(file);
	settings.open(file);
	string line;

	if (!settings.fail()){
		while (getline(settings, line)){
			trim(line); //remove spaces
			istringstream is_line(line);
			if (line.length() > 0){
				if (line[0]!='#' && line[0]!='/'){
					if (line[0]=='[' && line[line.length()-1]==']'){ //sections are used for loading the extension classes (if defined)
						if (line.compare("[mysql]")==0){
							add_extension(new MYSQLclient(this));
						}else if (line.compare("[http_client]")==0){
							add_extension(new HTTPClient(this));
						}else if (line.compare("[http_server]")==0){
							add_extension(new HTTPServerExtension(this));
						}else if (line.compare("[text]")==0){
							add_extension(new TextFileExtension(this));
						}else if (line.compare("[urad_monitor_client]") == 0) {
							add_extension(new UradClient(this));
						}
					}else{
						string key;
						if(getline(is_line, key, '=') ){
							string value;
							if(getline(is_line, value)){
								trim(key);
								trim(value);
								if (key.compare("interval")==0){ //upload interval
									int tmp_interval = (int)strtol(value.c_str(), NULL, 10);
									if (tmp_interval > 10) interval = tmp_interval;
								}else if (key.compare("use_remote_watchdog")==0){ //if we should use the USB connection 'remote' watchdog
									use_remote_watchdog = value.compare("1")==0;
								}else if (key.compare("log_file")==0){ //log file name
									log_file = value;
									if (log_file.length() > 0){
										Logger::open(log_file.c_str());
									}
								}else if (key.compare("log_file_max_size")==0){
									Logger::set_max_file_size((int)strtol(value.c_str(), NULL, 10));
								}else if (key.compare("vid")==0){
									this->counter.set_vid((int)strtol(value.c_str(), NULL, 10));
								}else if (key.compare("pid")==0){
									this->counter.set_pid((int)strtol(value.c_str(), NULL, 10));
								}else if (key.compare("debug")==0){ //enable debug output
									Logger::setReportingLevel ((TLogLevel) strtol(value.c_str(), NULL, 10));
								}
								for (std::vector<GeigerCounterExtension*>::iterator it = this->Extensions.begin(); it != this->Extensions.end(); it++) (*it)->read_setting(key, value); //pass the parameter to the extendsions, they process adjust their settings if needed
							}
						}
					}
				}
			}
		}
	}else{
		LOG_ERROR("Unable to open configuration file: " << file);
	}

	next_sampling_time = get_next_sampling_time();

	for (std::vector<GeigerCounterExtension*>::iterator it = this->Extensions.begin(); it != this->Extensions.end(); it++) (*it)->enable();

}

void GeigerCounterApplication::set_sampling_interval(int new_interval){
	interval = new_interval;
}

int GeigerCounterApplication::get_sampling_interval(){
	return interval;
}

bool GeigerCounterApplication::get_use_remote_watchdog(){
	return use_remote_watchdog;
}

GeigerCounterExtension * GeigerCounterApplication::get_extension(int index){
	return Extensions[index];
}

int GeigerCounterApplication::get_extension_count(){
	return Extensions.size();
}

//returns the next time to take a sample (= nr of counts every 5 min)
//the function will make sure this happens every x:00,x:05,x:10,x:15,... minutes
time_t GeigerCounterApplication::get_next_sampling_time (){
	time_t res = time(0) + interval;
	return res - (res % interval);
}

GeigerCounterExtension::GeigerCounterExtension(GeigerCounterApplication * app){
	this->app = app;
}

GeigerCounterExtension::~GeigerCounterExtension(){

}