#include "GeigerCounterText.h"
#include <pthread.h>
#include <ostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include "log.h"
#include "functions.h"

using namespace std;

TextFileExtension::TextFileExtension(GeigerCounterApplication * app):GeigerCounterExtension(app), name("TextFileExtension"){
	enabled=false;
	keep_saving=false;
	file_index=0;
	name.assign("textfile");
}

TextFileExtension::~TextFileExtension(){
	disable();
}

void TextFileExtension::enable(){
	if (!keep_saving){ //check already enabled
		if (enabled){
			LOG_DEBUG ("Saving samples as text in " << dir);
			keep_saving = true;
			pthread_create( & client_thread, NULL, (void* (*)(void*)) & TextFileExtension::client_thread_helper, this);
		}
	}
}

void TextFileExtension::disable(){
	if (keep_saving){
		keep_saving =false;
		pthread_join(client_thread,NULL);
	}
}

const char * TextFileExtension::get_name(){
	return name.c_str();
}

//process all the mysql settings lines
void TextFileExtension::read_setting (const std::string & key, const std::string & value){
	if (key.compare("text_dir")==0){
		dir = value;
	}else if (key.compare("text_exec_cmd")==0){
		exec_cmd = value;
	}else if (key.compare("text_enabled")==0){
		enabled = value.compare("1")==0;
	}
}

void TextFileExtension::on_interval(const GeigerCounterIntervalData & data){
	if (enabled) data_fifo.write(data); //5min sample stored (async buffer)
}

void TextFileExtension::on_packet_received (const USBGeigerCounterPacket & data){
//USB data received, do nothing
}

void * TextFileExtension::client_thread_helper(void * param){
	((TextFileExtension *) param)->start_saving();
}

void TextFileExtension::start_saving(){
	while (keep_saving){
		if (data_fifo.count()>0){
			
			char str_file_index[32];
			sprintf(str_file_index, "%u", file_index);
			string file_name = dir;
			
			if (dir.length()>0 && dir.at(dir.length()-1)!='/') dir.append("/");
			file_name.append("gc");
			file_name.append("_");
			file_name.append(str_file_index);
			file_name.append(".txt");

			while (file_exists(file_name.c_str())){
				file_index++;

				sprintf(str_file_index, "%u", file_index);
				file_name = dir;
				if (dir.length()>0 && dir.at(dir.length()-1)!='/') dir.append("/");
				file_name.append("gc");
				file_name.append("_");
				file_name.append(str_file_index);
				file_name.append(".txt");
			}
			
			GeigerCounterIntervalData data;
			data_fifo.peek(data);

			try {
				ofstream ofs(file_name.c_str(), std::ofstream::out);
				ofs << "start=" << data.start_time << endl
					<< "stop=" << data.end_time << endl 
					<< "counts=" << data.counts;
				ofs.close();
				file_index++;
				data_fifo.read(data); //remove from fifo
				if (exec_cmd.length()>0){
					string full_cmd = exec_cmd + " \"" + file_name + "\"";
					system(full_cmd.c_str());
				}
			}catch (std::ofstream::failure e){
				LOG_DEBUG("Error opening file " << file_name << ".");
			}

		}
		usleep(100000);
	}
	pthread_exit(NULL);
	return;
}

bool TextFileExtension::is_enabled(){
	return enabled;
} 

std::string TextFileExtension::get_status_json(){
	ostringstream res;
	res << "{\"enabled\": " << (enabled ? 1 : 0) << "}";
	return res.str();
}