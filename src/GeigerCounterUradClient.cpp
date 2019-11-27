#include "GeigerCounterUradClient.h"
#include "httprequest.h"
#include <unistd.h>
#include <time.h>
#include <sstream>
#include "log.h"
#include "tinyxml2.h"
#include <iostream>
#include <string>
#include "functions.h"
using namespace std;

#define ID_TIME_SECONDS "01"			// compulsory: local time in seconds
#define ID_TEMPERATURE_CELSIUS "02"		// optional: temperature in degrees celsius
#define ID_PRESSURE_PASCALS "03"		// optional: barometric pressure in pascals
#define ID_HUMIDITY_RH "04"				// optional: humidity as relative humidity in percentage %
#define ID_LUMINOSITY_RL "05"			// optional: luminosity as relative luminosity in percentage ‰
#define ID_VOC_OHM "06"					// optional: volatile organic compounds in ohms
#define ID_CO2_PPM "07"					// optional: carbon dioxide in ppm
#define ID_CH2O_PPM "08"				// optional: formaldehyde in ppm
#define ID_PM25_UGCM "09"				// optional: particulate matter in micro grams per cubic meter
#define ID_BATTERY_VOLTS "0A"			// optional: device battery voltage in volts
#define ID_GEIGER_CPM "0B"				// optional: radiation measured on geiger tube in cpm
#define ID_INVERTERVOLTAGE_VOLTS "0C"	// optional: high voltage geiger tube inverter voltage in volts
#define ID_INVERTERDUTY_PM "0D"			// optional: high voltage geiger tube inverter duty in ‰
#define ID_VERSION_HW "0E"				// optional: hardware version
#define ID_VERSION_SW "0F"				// optional: software firmware version
#define ID_TUBE "10"					// optional: tube type ID

#define DEV_CLASS 0x13

//https://github.com/radhoo/uradmonitor_kit1/blob/master/code/uRADMonitor.cpp
//case GEIGER_TUBE_SBM20M:	return 0.013333; // CPM 9

UradClient::UradClient(GeigerCounterApplication * app) :GeigerCounterExtension(app), name("UradMonitorClient") {
	urad_upload_enabled = false;
	interval_packet_received = false;
	interval = 60;
	enabled = false;
	last_upload_time = 0;
	last_http_response_code = 0;
	cpm_correction = 1.0;

	posturl = "http://data.uradmonitor.com/api/v1/upload/exp/";
	timeout = 60;
	dev_id = DEV_CLASS << 24 + 1;
	hw = 106;
	sw = 123;
	tube_type = 5; //GEIGER_TUBE_SBM20M

	pthread_mutex_init(&this->last_interval_packet_mutex, NULL);
	pthread_mutex_init(&this->last_packet_mutex, NULL);

	name.assign("UradMonitorClient");
}

UradClient::~UradClient() {
	disable();
}

const char * UradClient::get_name() {
	return name.c_str();
}

void UradClient::on_interval(const GeigerCounterIntervalData & data) { //called when the measure interval is passed
	pthread_mutex_lock(&last_interval_packet_mutex);
	last_interval_packet = data;
	interval_packet_received = true;
	pthread_mutex_unlock(&last_interval_packet_mutex);
}

void UradClient::on_packet_received(const USBGeigerCounterPacket & data) { //called for every packet received from USB interface
	pthread_mutex_lock(&last_packet_mutex);
	last_packet = data;
	pthread_mutex_unlock(&last_packet_mutex);
}

void UradClient::enable() { //enable the http client
	if (urad_upload_enabled && !enabled) {
		keep_uploading = true;
		pthread_create(&upload_thread, NULL, (void* (*)(void*)) & UradClient::upload_thread_helper, this);
		enabled = true;
		LOG_DEBUG("HTTP Client enabled, uploading every " << interval << " seconds to " << posturl);
	}
}

void UradClient::disable() { //disable
	if (enabled) {
		keep_uploading = false;
		pthread_join(upload_thread, NULL);
		enabled = false;
	}
}

void UradClient::read_setting(const std::string & key, const std::string & value) { //set settings
	if (key.compare("urad_client_enabled") == 0) {
		urad_upload_enabled = value.compare("1") == 0;
	}
	else if (key.compare("urad_post_url") == 0) {
		posturl = value;
	}
	else if (key.compare("urad_post_proxy") == 0) {
		proxy_server_port = value;
	}
	else if (key.compare("urad_post_proxy_login") == 0) {
		proxy_username_password = value;
	}
	/*else if (key.compare("http_post_max_packets") == 0) {
		max_upload_packets = (int)strtol(value.c_str(), NULL, 10);
	}*/
	else if (key.compare("urad_send_int") == 0) {
		interval = (int)strtol(value.c_str(), NULL, 10);
		if (interval < 60) interval = 60;
	}
	else if (key.compare("urad_post_timeout") == 0) {
		timeout = (int)strtol(value.c_str(), NULL, 10);
	}
	else if (key.compare("urad_user_id") == 0) {
		user_id = value;
	}
	else if (key.compare("urad_user_key") == 0) {
		user_key = value;
	}
	else if (key.compare("urad_dev_id") == 0) {
		dev_id = (unsigned int)strtol(value.c_str(), NULL, 16);
	}
	else if (key.compare("urad_hw") == 0) {
		hw = value;
	}
	else if (key.compare("urad_sw") == 0) {
		sw = value;
	}
	else if (key.compare("urad_tube_type") == 0) {
		tube_type = (unsigned int)strtol(value.c_str(), NULL, 10);
	}
	else if (key.compare("urad_correction") == 0) { //correction factor for CPM
		cpm_correction = atof(value.c_str());
		if (cpm_correction <= 0) cpm_correction = 1.0;
	}
}

void * UradClient::upload_thread_helper(void * param) { //helper for multithreading
	((UradClient *)param)->upload();
}

void UradClient::upload() {
	HTTPRequest request;
	while (keep_uploading) {
		if (interval_packet_received) {
			time_t upload_time = time(0);
			ostringstream url;

			request.reset();
			request.set_request_type(HTTPRequestPost);

			if (proxy_server_port.length() > 0) request.set_proxy_server(proxy_server_port.c_str());
			if (proxy_username_password.length() > 0) request.set_proxy_server_user_name_pwd(proxy_username_password.c_str());

			/*if (username.length() > 0 && password.length() > 0) { //support for http authentication
				request.set_http_authentication(HTTPAuthenticateAny);
				request.set_http_password(password.c_str());
				request.set_http_user_name(username.c_str());
			}*/

			request.set_https_no_host_verification();
			request.set_https_no_peer_verification();

			char header[128];
			request.add_header("User-Agent: uRADMonitor/1.1");

			sprintf(header, "X-User-id:%s", user_id.c_str());
			request.add_header(header);

			sprintf(header, "X-User-hash:%s", user_key.c_str());
			request.add_header(header);

			sprintf(header, "X-Device-id:%08lX", dev_id);
			request.add_header(header);

			//request.add_header("Content-Length: 0");
			request.add_header("Content-Type: application/x-www-form-urlencoded");

			pthread_mutex_lock(&last_interval_packet_mutex);
			pthread_mutex_lock(&last_packet_mutex);

			unsigned int volt = calculate_tube_voltage((int)last_packet.adc_value, (int)last_packet.adc_calibration);
			unsigned int time = last_packet.time_on;
			unsigned int pwm = last_packet.duty_cycle;
			unsigned int cpm = last_interval_packet.cpm * cpm_correction;

			pthread_mutex_unlock(&last_interval_packet_mutex);
			pthread_mutex_unlock(&last_packet_mutex);

			url << posturl << ID_TIME_SECONDS << "/" << time << "/" << ID_VERSION_HW << "/" << hw << "/" << ID_VERSION_SW << "/" << sw << "/"
				<< ID_GEIGER_CPM << "/" << cpm << "/" ID_INVERTERVOLTAGE_VOLTS << "/" << volt << "/" << ID_INVERTERDUTY_PM << "/" << pwm
				<< "/" ID_TUBE "/" << tube_type;

			//https://www.uradmonitor.com/topic/post-api-for-kit1/
			if (request.send(url.str().c_str(), false)) {
				last_http_response_code = request.http_response_code();
				if (request.http_response_ok()) { //check response code (200 OK)
					char value[32] = { 0 }; // this has to be inited, or when parsing we could have extra junk screwing results
					char * serverAnswer = (char*)request.response_data();
					if (jsonKeyFind(serverAnswer, "setid", value, 32)) {
						dev_id = hex2int(value);
						app->change_setting("urad_dev_id", value);
						LOG_INFO("Urad: Device ID changed by server to: " << dev_id);
					}
					else if (jsonKeyFind(serverAnswer, "sendint", value, 32)) {
						interval = atoi(value);
						LOG_INFO("Urad: send interval changed by server to: " << interval);
					}
					else if (jsonKeyFind(serverAnswer, "error", value, 32)) {
						LOG_ERROR("Urad upload error: " << value);
					}
				}
				else {
					/*XMLDocument * doc = request.response_xml(); //get the response as XML document
					if (doc->ErrorID() == 0) {
						XMLElement* Root = doc->FirstChildElement("response"); //get elements
						if (Root != NULL) {
							XMLElement* ResultElement = Root->FirstChildElement("global");
							if (ResultElement != NULL) {
								const char * result = ResultElement->GetText();
								if (strcmp(result, "1") == 0) { //upload ok
									for (int i = 0;i < count;i++) packets.read(packet); //now remove the packets from the queue
									last_upload_time = time(0);
								}
								else { //server application error
									LOG_ERROR("Upload failed result: " << result << " response:\n" << request.response_data());
								}
							}
							else { //no result element found
								LOG_ERROR("Invalid XML, <result> not found:\n" << request.response_data());
							}
						}
						else { //no root element
							LOG_ERROR("No <respons> element:\n" << request.response_data());
						}
					}
					else {//xml error
						LOG_ERROR("Invalid XML returned:\n" << request.response_data());
					}
				}
				else {//upload error*/
					LOG_ERROR("Upload error Uradmonitor HTTP respons:\n" << request.http_response_code() << " data: " << request.response_data());
					if (request.http_response_code() == 0) LOG_ERROR("Please check your internet connection!");
				}
			}
			else {//curl error
				LOG_ERROR("cURL error:\n" << request.curl_error());
			}
		}

		int count_down = interval; //wait for next to be uploaded
		while (count_down > 0 && keep_uploading) {
			usleep(1000000); //sleep 1s
			count_down--;
		}
	}
	pthread_exit(NULL);
}

bool UradClient::is_enabled() {
	return enabled;
}

std::string UradClient::get_status_json() {
	ostringstream res;
	res << "{\"enabled\": " << (enabled ? 1 : 0) <<
		", \"last_response_code\": " << last_http_response_code <<
		", \"last_upload_ok\": " << last_upload_time << "}";
	return res.str();
}

int UradClient::get_http_response_code() {
	return last_http_response_code;
}

time_t UradClient::get_last_upload_time() {
	return last_upload_time;
}