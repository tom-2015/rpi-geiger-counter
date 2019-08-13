#include "GeigerCounterHTTPClient.h"
#include "httprequest.h"
#include <unistd.h>
#include <time.h>
#include <sstream>
#include "log.h"
#include "tinyxml2.h"
#include <iostream>
#include "tinyxml2.h"
#include "md5.h"
#include "functions.h"
#include "zlib.h"
using namespace std;
using namespace tinyxml2;


HTTPClient::HTTPClient(GeigerCounterApplication * app):GeigerCounterExtension(app), name("HTTPClient"){
	http_upload_enabled=false;
	interval = 300;
	max_upload_packets=1000; //allow 1000 packets per http request
	enabled=false;
	public_port=0;
	last_upload_time=0;
	last_http_response_code=0;
	name.assign("httpclient");
}

HTTPClient::~HTTPClient(){
	disable();
}

const char * HTTPClient::get_name(){
	return name.c_str();
}

void HTTPClient::on_interval(const GeigerCounterIntervalData & data){ //called when the measure interval is passed
	if (enabled) packets.write(data); //add data to the queue to be uploaded
}

void HTTPClient::on_packet_received (const USBGeigerCounterPacket & data){ //called for every packet received from USB interface

}

void HTTPClient::enable(){ //enable the http client
	if (http_upload_enabled && !enabled){
		keep_uploading=true;
		pthread_create(& upload_thread, NULL, (void* (*)(void*)) & HTTPClient::upload_thread_helper, this);
		enabled=true;
		LOG_DEBUG ("HTTP Client enabled, uploading every " << interval << " seconds to " << posturl);
	}
}

void HTTPClient::disable(){ //disable
	if (enabled){
		keep_uploading=false;
		pthread_join(upload_thread,NULL);
		enabled=false;
	}
}

void HTTPClient::read_setting (const std::string & key, const std::string & value){ //set settings
	if (key.compare("http_upload_enabled")==0){
		http_upload_enabled = value.compare("1")==0;
	}else if (key.compare("http_post_url")==0){
		posturl = value;
	}else if (key.compare("http_post_proxy")==0){
		proxy_server_port = value;
	}else if (key.compare("http_post_proxy_login")==0){
		proxy_username_password = value;
	}else if (key.compare("http_post_max_packets")==0){
		max_upload_packets = (int)strtol(value.c_str(), NULL, 10);
	}else if (key.compare("http_post_interval")==0){
		interval = (int)strtol(value.c_str(), NULL, 10);
		if (interval<100) interval=300;
	}else if (key.compare("http_post_timeout")==0){
		timeout = (int)strtol(value.c_str(), NULL, 10);
	}else if (key.compare("http_username")==0){
		username = value;
	}else if (key.compare("http_password")==0){
		password = value;
	}else if (key.compare("http_counter_password")==0){
		counter_password = value;
	}else if (key.compare("http_counter_id")==0){
		counter_id = value;
	}else if (key.compare("http_public_access_port")==0){
		public_port = (int) strtol(value.c_str(), NULL, 10);
	}else if (key.compare("http_compress") == 0) {
		compress = ((int)strtol(value.c_str(), NULL, 10)) > 0;
	}
}

void * HTTPClient::upload_thread_helper(void * param){ //helper for multithreading
	((HTTPClient *) param)->upload();
}

//returns the hash of the password and the token, token must be a number that is increased everytime.
//This is why UTC time is used, you must keep your raspberry time synced!
string get_counter_password_md5(time_t time, const string & password){
	unsigned char password_md5[33];
	ostringstream result;
	string result_string;

	MD5_CTX mdContext; //calc MD5 of password
	MD5Init (&mdContext);
	MD5Update (&mdContext, (unsigned char *) password.c_str() , password.length());
	MD5Final (&mdContext);
	MD5HexDump(&mdContext, password_md5);
	
	result << time << ":" << password_md5; //add time and password

	result_string = result.str();

	MD5Init(&mdContext);
	MD5Update(&mdContext, (unsigned char *) result_string.c_str(), result_string.length());
	MD5Final(&mdContext);
	MD5HexDump(&mdContext, password_md5);

	result_string.clear();
	result_string.assign((const char *) password_md5);
	return result_string;
}

void HTTPClient::upload(){
	HTTPRequest request;
	while(keep_uploading){
		GeigerCounterIntervalData packet;
		int count = 0;
		time_t upload_time = time(0);
		ostringstream data;
		bool zlib_init_failed = false;
		unsigned char * zlib_out = NULL;

		data << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<request>";
		data << "<meter id=\"" << counter_id << "\" version=\"" << APP_VERSION << "\" token=\"" << upload_time << "\" password=\"" << get_counter_password_md5(upload_time, counter_password) << "\">";

		//save all the counter packets
		while (count < max_upload_packets && packets.peek(packet)){
			data << "<sample type=\"1\">" << //type is 1, radioactivity
						"<start format=\"TS\">" << packet.start_time << "</start>" << //start and end time
						"<end format=\"TS\">" << packet.end_time << "</end>" <<
						"<value unit=\"counts\">" << packet.counts << "</value>" << //number of counts
					"</sample>";
			count++;
		}
   
		//save counter status
		if (count > 0){
			data << "<status>" <<
						"<last_response_time>" << app->counter.get_last_response_time() << "</last_response_time>" <<
						"<counter>" << (int) packet.last_packet.counter << "</counter>" <<
						"<time_counting>" << packet.last_packet.time_counting << "</time_counting>" <<
						"<pulse>" << packet.last_packet.last_pulsewidth << "</pulse>" <<
						"<error>" << packet.last_packet.error << "</error>" << 
						"<wdt>" << packet.last_packet.wdt  << "</wdt>" << 
						"<pwm max=\"" << packet.last_packet.max_pwm_value << "\">" << packet.last_packet.duty_cycle << "</pwm>" <<
						"<port>" << public_port << "</port>" << 
						"<version>" << ((int) packet.last_packet.version) << "</version>" << 
						"<threshold>" << packet.last_packet.threshold << "</threshold>" << 
						"<tube_voltage>" << calculate_tube_voltage ((int)packet.last_packet.adc_value, (int)packet.last_packet.adc_calibration) << "</tube_voltage>" << 
						"<adc cal=\"" << packet.last_packet.adc_calibration << "\">" << packet.last_packet.adc_value << "</adc>" <<
					"</status>" <<
					"<upload_interval>" << interval << "</upload_interval>" <<
					"<sample_interval>" << this->app->get_sampling_interval() << "</sample_interval>";
		}

		data << "</meter></request>";

		//if packets received, upload the data using http request
		if (count > 0) {
			string post_data = data.str();
			request.reset();
			request.set_request_type(HTTPRequestPost);

			if (proxy_server_port.length() > 0) request.set_proxy_server(proxy_server_port.c_str());
			if (proxy_username_password.length() > 0) request.set_proxy_server_user_name_pwd(proxy_username_password.c_str());

			if (username.length() > 0 && password.length() > 0) { //support for http authentication
				request.set_http_authentication(HTTPAuthenticateAny);
				request.set_http_password(password.c_str());
				request.set_http_user_name(username.c_str());
			}

			request.set_https_no_host_verification();
			request.set_https_no_peer_verification();

			if (compress) {
				request.set_http_header("Content-Encoding: gzip");
				z_stream strm;
				
				/* allocate deflate state */
				strm.zalloc = Z_NULL;
				strm.zfree = Z_NULL;
				strm.opaque = Z_NULL;

				if (deflateInit(&strm, Z_DEFAULT_COMPRESSION) == Z_OK) {
					zlib_out = new unsigned char [post_data.length()];
					strm.avail_in = post_data.length();
					strm.next_in = (unsigned char *) post_data.c_str();

					strm.avail_out = post_data.length();
					strm.next_out = zlib_out;

					if (deflate(&strm, Z_FINISH) != Z_STREAM_ERROR) {    /* no bad return value */
						/*ofstream myFile; for debugging you can write the gz data to a file...
						myFile.open("/dev/shm/zlib_data.bin", ios::out | ios::binary);
						myFile.write((const char *) zlib_out, post_data.length() - strm.avail_out);
						myFile.close();*/
						request.add_form_parameter("data", zlib_out, post_data.length() - strm.avail_out);
					} else {
						LOG_ERROR("Failed compress http form data, turn of compression!");
					}

				}else{
					zlib_init_failed = true;
					LOG_ERROR("Failed to init zlib, turn of compression!");
				}
				deflateEnd(&strm);
			}
			else
			{
				request.add_form_parameter("data", post_data.c_str(), post_data.length());
			}
			
			if (request.send (posturl.c_str(), false) && !zlib_init_failed){
				last_http_response_code = request.http_response_code();
				if (request.http_response_ok()){ //check response code (200 OK)
					XMLDocument * doc = request.response_xml(); //get the response as XML document
					if (doc->ErrorID()==0){
						XMLElement* Root = doc->FirstChildElement("response"); //get elements
						if (Root!=NULL){
							XMLElement* ResultElement = Root->FirstChildElement("global");
							if (ResultElement!=NULL){
								const char * result = ResultElement->GetText();
								if (strcmp(result, "1")==0){ //upload ok
									for (int i=0;i<count;i++) packets.read(packet); //now remove the packets from the queue
									last_upload_time = time(0);
								}else{ //server application error
									LOG_ERROR ("Upload failed result: " << result << " response:\n" << request.response_data());
								}
							}else{ //no result element found
								LOG_ERROR ("Invalid XML, <result> not found:\n" << request.response_data());
							}
						}else{ //no root element
							LOG_ERROR ( "No <respons> element:\n" << request.response_data());
						}
					}else{//xml error
						LOG_ERROR ("Invalid XML returned:\n" << request.response_data());
					}
				}else{//upload error
					LOG_ERROR ("Upload error HTTP respons:\n" << request.http_response_code() << "data: " << request.response_data());
					if (request.http_response_code()==0) LOG_ERROR("Please check your internet connection!");
				}
			}else{//curl error
				LOG_ERROR ("cURL error:\n" << request.curl_error());
			}
		}

		if (zlib_out != NULL) delete zlib_out;

		int count_down = interval; //wait for next to be uploaded
		while (count_down > 0 && keep_uploading){
			usleep(1000000); //sleep 1s
			count_down--;
		}
	}
	pthread_exit(NULL);
}

bool HTTPClient::is_enabled(){
	return enabled;
} 

std::string HTTPClient::get_status_json(){
	ostringstream res;
	res << "{\"enabled\": " << (enabled ? 1 : 0) << 
		", \"last_response_code\": " << last_http_response_code << 
		", \"last_upload_ok\": " << last_upload_time << "}";
	return res.str();
}

int HTTPClient::get_http_response_code(){
	return last_http_response_code;
}

time_t HTTPClient::get_last_upload_time(){
	return last_upload_time;
}