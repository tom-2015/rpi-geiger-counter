#include "log.h" 
#include <time.h>
#include <ctime>
#include <stdio.h>
#include <sstream>
#include "functions.h"

TLogLevel loggger_wantedLevel=(TLogLevel) (logERROR | logWARNING | logSTDOUT);
TLogLevel logger_messageLevel=logNone;

std::ofstream logger_os; //output file stream for logging data to
std::string logger_filename;
pthread_mutex_t logger_mutex; //mutex, to make things thread safe
unsigned long logger_max_file_size=LOG_DEFAULT_FILE_SIZE;

std::ofstream& Logger::LogStart(TLogLevel level){ 
	char buffer[64];
	
	if (logger_os.tellp() >= logger_max_file_size){
		logger_os.close();
		std::string renamed_file = logger_filename;
		renamed_file.append(".backup");
		
		if (file_exists(renamed_file.c_str())) remove (renamed_file.c_str());

		rename(logger_filename.c_str(), renamed_file.c_str());
		remove (logger_filename.c_str());
	}	

	if (!file_exists(logger_filename.c_str())){
		logger_os.close();
		logger_os.open(logger_filename.c_str(),  std::ios::out | std::ios::app);
		if (logger_os.is_open()){
			logger_messageLevel = loggger_wantedLevel;
		}else{
			logger_messageLevel = logNone; //prevent from sending data into the not opened stream
		}
	}

	time_t thetime = time(0);
	tm * ptm = std::localtime(& thetime);
	strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", ptm);// Format: 15.06.2009 20:20:00 

	logger_os << "-" << buffer << " " << ToString(level) << ": ";

	return logger_os;
}

std::string Logger::GetTime() {
	std::string res;

	char buffer[64];
	time_t thetime = time(0);
	tm * ptm = std::localtime(&thetime);
	strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", ptm);// Format: 15.06.2009 20:20:00 
	res.append(buffer);
	return res;
}

std::ofstream& Logger::GetStream(){
	return logger_os;
}

void Logger::Flush(){
	logger_os.flush();
    if (logSTDOUT & Logger::ReportingLevel()) std::cout.flush();
}

std::string Logger::ToString (TLogLevel level){
	std::string res;
	if (level & logERROR) res = "ERROR";
	if (level & logWARNING) res = "WARNING";
	if (level & logINFO) res = "INFO";
	if (level &logDEBUG) res = "DEBUG";
	return res;
}

void Logger::setReportingLevel(TLogLevel level){
	loggger_wantedLevel = level;
	if (logger_os.is_open()){
		logger_messageLevel = loggger_wantedLevel;
	}else{
		logger_messageLevel = logNone;
	}
}

bool Logger::open(const char * filename){
    pthread_mutex_init(&logger_mutex, NULL);
	logger_filename.assign(filename);
	logger_os.open(filename,  std::ios::out | std::ios::app);
	if (logger_os.is_open()){
		logger_messageLevel = loggger_wantedLevel;
		return true;
	}else{
		logger_messageLevel = logNone; //prevent from sending data into the not opened stream
		return false;
	}
}

void Logger::set_max_file_size(unsigned long size){
    logger_max_file_size=size;
    if (size ==0) logger_max_file_size = LOG_DEFAULT_FILE_SIZE;
}

unsigned long Logger::get_max_file_size(){
    return logger_max_file_size;
}

void Logger::close(){
	if (logger_os.is_open()){
		logger_os.flush();
		logger_os.close();
		logger_messageLevel = logNone;
	}
}

TLogLevel Logger::ReportingLevel(){
	return logger_messageLevel;
}

Logger::Logger(){

}

Logger::~Logger(){   

}
