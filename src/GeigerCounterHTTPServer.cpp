#include "GeigerCounterHTTPServer.h"

#include "log.h"
#include <sstream>
#include <iostream>
#include "functions.h"

using namespace std;

#define SERVER_BUFFER_SIZE 512

HTTPServerExtension::HTTPServerExtension (GeigerCounterApplication * app):GeigerCounterExtension(app), name("HTTPServer"){
	pthread_mutex_init(&mutex_last_packet, NULL);
	pthread_mutex_init(&mutex_measured_data_list,NULL);
	memset(& last_packet, 0, sizeof(last_packet));
	enable_http_server=false;
	enabled=false;
	server=NULL;
	listen_port=79;
	www_dir.assign("www");
	http_www_enabled=true;
	this->app = app;
	name.assign("httpserver");
}

HTTPServerExtension::~HTTPServerExtension(){
	if (server!=NULL){
		delete server;
		enabled=false;
	}
}

void HTTPServerExtension::on_interval(const GeigerCounterIntervalData & data){
	pthread_mutex_lock(&mutex_measured_data_list);
	int max_samples = 24 * 3600 / this->app->get_sampling_interval(); //nr of samples to give 24h chart
	measured_data_list.push_back(data);
	while (measured_data_list.size() > max_samples){
		measured_data_list.erase(measured_data_list.begin());
	}
	pthread_mutex_unlock(&mutex_measured_data_list);
}

const char * HTTPServerExtension::get_name(){
	return name.c_str();
}

void HTTPServerExtension::on_packet_received (const USBGeigerCounterPacket & data){
	pthread_mutex_lock(&mutex_last_packet);
	last_packet = data;
	pthread_mutex_unlock(&mutex_last_packet);
}

void HTTPServerExtension::enable(){
	if (enable_http_server && !enabled){
		LOG_DEBUG ("HTTP Server enabled on port " << listen_port);
		try {
			server = new http_server(listen_port);
			server->add_listener(this);
			enabled=true;
		}catch (SocketException e){
			LOG_ERROR("HTTP Server init error: " << e.what() << " Try running with sudo!");
			enabled=false;
		}
	}
}

void HTTPServerExtension::disable(){
	enabled=false;
	if (server!=NULL){
		server->close();
	}
}

void HTTPServerExtension::read_setting (const std::string & key, const std::string & value){
	if (key.compare("enable_http_server")==0){
		enable_http_server = value.compare("1")==0;
	}else if (key.compare("http_server_port")==0){
		listen_port = (unsigned short)strtol(value.c_str(), NULL, 10);
	}else if (key.compare("http_www_dir")==0){
		www_dir = value;
	}else if (key.compare("http_www_enabled")==0){
		http_www_enabled = value.compare("1")==0;
	}
}

void HTTPServerExtension::http_event (http_server * server, http_server_event * e){
	if (e->type==HTTP_EVENT_REQUEST){
		string url;
		string * url_ptr;
		int pos = e->request_url->find("?"); //remove parameters
		if (pos!= string::npos) {
			url = e->request_url->substr(0, pos);
			url_ptr = & url;
		}else{
			url_ptr = e->request_url;
		}

		ostringstream content;
		if (url_ptr->compare("/json")==0){
			pthread_mutex_lock (& mutex_last_packet); //lock the http packet or wait until it is unlocked see https://computing.llnl.gov/tutorials/pthreads/
			
			content << "{\"type\": " << (int)last_packet.type << ", " <<
					  "\"version\": " << (int)last_packet.version << ", " << 
					  "\"flags\": " << last_packet.flags << ", " <<
					  "\"counter\": " << last_packet.counter << ", " <<
					  "\"time_counting\": " << last_packet.time_counting << ", " << 
					  "\"time_on\": " << last_packet.time_on << ", " <<
					  "\"adc_calibration\": " << last_packet.adc_calibration << ", " <<
					  "\"threshold\": " << last_packet.threshold << ", "<<
					  "\"last_pulsewidth\": " << last_packet.last_pulsewidth << ", " <<
					  "\"adc_value\": " << last_packet.adc_value << ", " << 
					  "\"error\": " << last_packet.error << ", " << 
					  "\"watchdog_restarts\": " << last_packet.wdt << ", " <<
					  "\"duty_cycle\": " << last_packet.duty_cycle << ", " <<
					  "\"last_pulse_time\": " << last_packet.last_pulse_time << ", " <<
					  "\"max_duty_cycle\": " << last_packet.max_pwm_value << ", " <<
					  "\"extensions\": {";

			for (int i=0; i<app->get_extension_count();i++){
				if (i!=0) content << ", ";
				content << "\"" << app->get_extension (i)->get_name() << "\": " << app->get_extension(i)->get_status_json();
			}
			content << "}}";

			pthread_mutex_unlock (& mutex_last_packet); //unlock the http packet

			server->http_ok(true, content.str().length(), "cache-control: no-cache\r\nContent-Type: application/json\r\n");
			server->send_data(content.str());
		}else if (url_ptr->compare("/chart.json")==0){
			pthread_mutex_lock(&mutex_measured_data_list);
			time_t p_sample_time=0;
			time_t first_sample_time;
			if (measured_data_list.size() > 0) first_sample_time = measured_data_list[0].end_time;

			content << "{\"xlabels\":[";
			for (int i=0;i<measured_data_list.size();i++){
				time_t sample_time = measured_data_list[i].end_time;
				if ((i%4)==0 || i==measured_data_list.size()-1){
					if (i!=0) content << ",";
					struct tm * localsampletime = localtime(&sample_time);
					content << "[" << ((sample_time-first_sample_time) / 60) << ", \"" << localsampletime->tm_hour << ":" << localsampletime->tm_min << "\"]";
					p_sample_time = sample_time;
				}
			}
			content << "],\"charts\":[{\"label\":\"hits\", \"data\": [";
			for (int i=0;i<measured_data_list.size();i++){
				time_t sample_time = measured_data_list[i].end_time;
				if (i!=0) content << ",";
				content << "[" << ((sample_time-first_sample_time) / 60) << "," << measured_data_list[i].counts << "]";
			}
			content << "]}], \"tooltipdata\":[";
			for (int i=0;i<measured_data_list.size();i++){
				time_t sample_time = measured_data_list[i].end_time;
				if (i!=0) content << ",";
				content << "[" << ((sample_time-first_sample_time ) / 60) << "," << measured_data_list[i].counts << "]";
			}
			content << "]}";
			server->http_ok(true, content.str().length(), "cache-control: no-cache\r\nContent-Type: application/json\r\n");
			server->send_data(content.str());
			pthread_mutex_unlock(&mutex_measured_data_list);
		}else if (http_www_enabled && www_dir.length()>1 ){
			if (url_ptr->c_str()[0]!='/'){
				server->http_bad_request();
				return;
			}

			if (url_ptr->compare("/")==0){
				url_ptr->clear();
				url_ptr->assign("/index.htm");
			}

			if (url_ptr->find("/../")!= string::npos){ //not supported to prevent going out of the www directory
				server->http_bad_request();
				return;
			}

			unsigned char buffer[SERVER_BUFFER_SIZE];
			string path = www_dir;
			path.append(url_ptr->c_str());

			FILE * f = fopen(path.c_str(), "r");
			if (f!=NULL){
				fseek (f , 0 , SEEK_END);

				string mime_type = server->get_mime_type(get_file_extension(*url_ptr));
				string content_type;

				if (mime_type.length()>0){
					content_type = "Content-Type: " + mime_type + "\r\n";
				}
				
				server->http_ok(true, (int)ftell(f), content_type.c_str());

				rewind (f);
				
				size_t read = fread(buffer, 1, SERVER_BUFFER_SIZE, f);
				while (read!=0){
					server->send_data((char*)buffer, read);
					read = fread(buffer, 1, SERVER_BUFFER_SIZE, f);
				}
				fclose(f);
			}else{
				server->http_not_found();
			}
		}else{
			server->http_not_found();
		}
	}
}

bool HTTPServerExtension::is_enabled(){
	return enabled;
} 

std::string HTTPServerExtension::get_status_json(){
	ostringstream res;
	res << "{\"enabled\": " << (enabled ? 1 : 0) << "}";
	return res.str();
}