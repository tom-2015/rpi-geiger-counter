#ifndef _FUNCTIONS_H
#define _FUNCTIONS_H

#include <string>
#include <time.h>

#define DEFAULT_DATE_TIME_FORMAT "%Y.%m.%d %H:%M:%S"

void trim(std::string& s);			   //removes spaces at the beginning and the end of a string
std::string format_time (time_t time); //uses a default format
std::string format_time (time_t time, const char * format); //formats time to a std string
std::string format_gm_time(time_t time); //formats time as gm time
std::string format_gm_time (time_t time, const char * format);
int kbhit(void);
bool file_exists(const char *name); //checks if a file exists
int calculate_tube_voltage (int adc_value, int adc_calibration); //returns the voltage on the geiger counter tube, given the adc value and calibration number
std::string str_replace(std::string src, std::string const& target, std::string const& repl);
std::string get_file_extension(std::string const & path);

#endif