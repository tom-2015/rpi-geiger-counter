#include "functions.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <termios.h>
#include <fcntl.h>
#include <sys/stat.h>

using namespace std;

bool file_exists(const char *name){
  struct stat   buffer;
  return (stat (name, &buffer) == 0);
}


void trim(string& s) {
    while(s.compare(0,1," ")==0 || s.compare(0,1,"\t")==0)
        s.erase(s.begin()); // remove leading whitespaces
    while(s.size()>0 && (s.compare(s.size()-1,1," ")==0 || s.compare(s.size()-1,1,"\t")==0))
        s.erase(s.end()-1); // remove trailing whitespaces
}

string format_time(time_t time){
	return format_time(time, DEFAULT_DATE_TIME_FORMAT);
}

string format_time (time_t time, const char * format){
	std::string res;
	char buffer[32];

	tm * ptm = std::localtime(& time);
	strftime(buffer, 32, format, ptm);// Format: 15.06.2009 20:20:00 
	res.append(buffer);
	return res;
}

string format_gm_time(time_t time){
	return format_time(time, DEFAULT_DATE_TIME_FORMAT);
}

string format_gm_time (time_t time, const char * format){
	std::string res;
	char buffer[32];

	tm * ptm = std::gmtime(& time);
	strftime(buffer, 32, format, ptm);// Format: 15.06.2009 20:20:00 
	res.append(buffer);
	return res;
}

int kbhit(void){
  struct termios oldt, newt;
  int ch;
  int oldf;
 
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
 
  ch = getchar();
 
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
 
  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }
 
  return 0;
}

int calculate_tube_voltage (int adc_value, int adc_calibration){
	return (4096ul * (unsigned long)adc_value / 1024ul * 1004700ul / 1000ul / 4700ul  * (unsigned long)adc_calibration / 1000000ul);
}

//replaces target to repl in src
string str_replace( string src, string const& target, string const& repl){
    if (target.length() == 0) {
        return src;
    }

    if (src.length() == 0) {
        return src;
    }

    size_t idx = 0;

    for (;;) {
        idx = src.find( target, idx);
        if (idx == string::npos)  break;

        src.replace( idx, target.length(), repl);
        idx += repl.length();
    }

    return src;
}

string get_file_extension(string const & path){
	string extension;
	int pos= path.find_last_of(".");

	if (pos!=string::npos){
		extension = path.substr(pos+1);
	}

	return extension;
}