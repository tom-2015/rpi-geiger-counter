#ifndef _LOG_H
#define _LOG_H

#include <iostream>
#include <fstream>
#include <pthread.h>

// Log, version 0.1: a simple logging class

class Logger;

extern pthread_mutex_t logger_mutex;

//log levels
typedef enum  {
	logNone=0,
	logERROR=1, 
	logWARNING=2, 
	logINFO=4, 
	logDEBUG=8,
	logSTDOUT=16, //also send message to stdout
	logAll = 1+2+4+8+16
} TLogLevel;

#define LOG_DEFAULT_FILE_SIZE 1048576   //default, max 1MB log files

#define LOG_STREAM Logger::GetStream() //returns the ofstream for writing custom data to the log file without date/time stamps

//use the following macros to log message to the debug file and/or stdout
#define LOG_DEBUG(message) if (logDEBUG & Logger::ReportingLevel()){ \
                                pthread_mutex_lock(&logger_mutex); \
								Logger::LogStart(logDEBUG) << message << endl; \
								Logger::Flush(); \
                                pthread_mutex_unlock(&logger_mutex);\
								if (logSTDOUT & Logger::ReportingLevel()){ std::cout << Logger::GetTime() << " " << message << endl; } \
							}

#define LOG_ERROR(message) if (logERROR & Logger::ReportingLevel()){ \
                                pthread_mutex_lock(&logger_mutex); \
								Logger::LogStart(logERROR) << message << endl; \
								Logger::Flush(); \
                                pthread_mutex_unlock(&logger_mutex);\
								if (logSTDOUT & Logger::ReportingLevel()){ std::cout << Logger::GetTime() << " " << message << endl; } \
							}

#define LOG_WARNING(message) if (logWARNING & Logger::ReportingLevel()){ \
                                pthread_mutex_lock(&logger_mutex); \
								Logger::LogStart(logWARNING) << message << endl; \
								Logger::Flush(); \
                                pthread_mutex_unlock(&logger_mutex);\
								if (logSTDOUT & Logger::ReportingLevel()){ std::cout << Logger::GetTime() << " " << message << endl; } \
							}

#define LOG_INFO(message) if (logINFO & Logger::ReportingLevel()){ \
                                pthread_mutex_lock(&logger_mutex); \
								Logger::LogStart(logINFO) << message << endl; \
								Logger::Flush(); \
                                pthread_mutex_unlock(&logger_mutex);\
								if (logSTDOUT & Logger::ReportingLevel()){ std::cout << Logger::GetTime() << " " << message << endl; } \
							}

#define LOG(level, message) if (level & Logger::ReportingLevel()){ \
                                pthread_mutex_lock(&logger_mutex); \
								Logger::LogStart(level) << message << endl; \
								Logger::Flush(); \
                                pthread_mutex_unlock(&logger_mutex);\
								if (logSTDOUT & Logger::ReportingLevel()){ std::cout << Logger::GetTime() << " " << message << endl; } \
							}

class Logger{
	public:   
		Logger();   
		virtual ~Logger();   
		static std::ofstream& LogStart(TLogLevel level); //starts a new line and adds date/time
		static std::ofstream& GetStream();               //returns the file stream
		static void Flush();                             //flush the output buffer
		static bool open(const char * filename);         //open the log file
		static void close();                             //close the log file
		static TLogLevel ReportingLevel();               //returns the reporting level
		static void setReportingLevel(TLogLevel level);  //sets the reporting level
		static std::string ToString (TLogLevel level);   //converts the reporting level to a string (the text is used in the log file)
		static void set_max_file_size(unsigned long size);
		static unsigned long get_max_file_size();
		static std::string GetTime();
};

#endif