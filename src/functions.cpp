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
#include <string.h>

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

unsigned int MIN(unsigned int a, unsigned int b) {
	if (a < b) return a;
	else return b;
}

/**
 * jsonKeyFind
 * finds a key and copies its value to the value output pointer
 */
bool jsonKeyFind(char *response, const char *key, char *value, unsigned int size) {
	char *s1 = strstr(response, key);
	unsigned int len = strlen(key);
	if (s1 && len) {
		char *s2 = strstr(s1 + len + 3, "\"");
		if (s2) {
			strncpy(value, s1 + len + 3, MIN(s2 - s1 - len - 3, size));
			return true;
		}
	}
	return false;
}

unsigned int hex2int(char *hex) {
	unsigned int val = 0;

	while (*hex) {
		// get current character then increment
		unsigned char byte = *hex++;
		// transform hex character to the 4bit equivalent number, using the ascii table indexes
		if (byte >= '0' && byte <= '9') byte = byte - '0';
		else if (byte >= 'a' && byte <= 'f') byte = byte - 'a' + 10;
		else if (byte >= 'A' && byte <= 'F') byte = byte - 'A' + 10;
		// shift 4 to make space for new digit, and add the 4 bits of the new digit
		val = (val << 4) | (byte & 0xF);
	}
	return val;
}