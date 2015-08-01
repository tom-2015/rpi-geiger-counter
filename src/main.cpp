#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <usb.h>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>
#include <unistd.h>
#include <cstdlib>
#include <time.h>
#include <ctime>
#include <termios.h>
#include <fcntl.h>

#include "GeigerCounterApp.h"
#include "functions.h"
#include "log.h"

using namespace std;

#define SAMPLING_INTERVAL_TIME 300        //this is the default sampling time, every 5 minutes we take a sample of how many counts and save that
#define DEFAULT_CONFIG_FILE "geiger.conf" //where default configuration is saved

GeigerCounterApplication app;

void exit_program(){
	//cout << "Exit because of user pressed key..." << endl;
	LOG_DEBUG("Exit program (you pressed a key).");
	app.counter.turn_remote_wdt_off();
	app.~GeigerCounterApplication();
	exit(0);
}

//this function starts polling
//the function only returns when read/write was not possible to the device (disconnected) or user pressed a key
void start_polling (){

	if (app.get_use_remote_watchdog()){
		if (!app.counter.turn_remote_wdt_on()){
			LOG_ERROR("Turning remote watchdog on failed." << strerror(errno));
			return;
		}
	}

	usleep (100000);

	if (!app.counter.turn_auto_usb_off()){
		LOG_ERROR("Turning automatic usb off failed." << strerror(errno));
		return;
	}
	
	USBGeigerCounterPacket packet;

	while (true){

		if(kbhit()){
			exit_program();
		}

		usleep(250000);

		//ask the geiger counter to send the data over the usb interface
		if (!app.counter.poll()){
			LOG_ERROR("Poll failed." << strerror(errno));
			return;
		}

		usleep(250000);

		//now read the received data
		if (!app.counter.read_packet(&packet)){
			LOG_ERROR("Read USB data failed! " << strerror(errno) << "Resetting USB device.");
			app.counter.reset_device();
			return;
		}

		//tell the application we received a packet from the USB interface, this class will then pass it to the other extensions like HTTPclient which will save a copy,...
		app.packet_received(packet);

	}
}

bool execute_command (int argc, char ** argv, const char * config_file){
	USBGeigerCounter counter;
	ifstream settings;

	settings.open(config_file);
	string line;

	if (!settings.fail()){
		while (getline(settings, line)){
			trim(line); //remove spaces
			istringstream is_line(line);
			if (line.length() > 0){
				if (line[0]!='#' && line[0]!='/'){
					string key;
					if(getline(is_line, key, '=') ){
						string value;
						if(getline(is_line, value)){
							trim(key);
							trim(value);
							if (key.compare("log_file_max_size")==0){
								Logger::set_max_file_size((int)strtol(value.c_str(), NULL, 10));
							}else if (key.compare("vid")==0){
								counter.set_vid((int)strtol(value.c_str(), NULL, 10));
							}else if (key.compare("pid")==0){
								counter.set_pid((int)strtol(value.c_str(), NULL, 10));
							}else if (key.compare("debug")==0){ //enable debug output
								Logger::setReportingLevel ((TLogLevel) strtol(value.c_str(), NULL, 10));
							}
						}
					}
				}
			}
		}
	}else{
		LOG_ERROR("Unable to open configuration file: " << config_file);
	}

	switch (counter.connect()){
		case HID_ERROR_NONE:
			cout << "Connected to device " << counter.hid.get_dev_path() << endl;
			break;
		case HID_ERROR_CANT_CREATE_UDEV:
			cout << "Can't create udev object, is libudev installed? run command: sudo apt-get install libudev-dev." << endl;
			return true;
			break;
		case HID_ERROR_UNABLE_TO_FIND_PARENT_USB_DEVICE:
			cout << "Can't find parent USB device." << endl;
			return true;
			break;
		case HID_ERROR_CANT_OPEN_FILE:
			cout << "Access denied, run this program with sudo!" << endl;
			return true;
			break;
		case HID_ERROR_DEVICE_NOT_FOUND:
			cout << "Device not found." << endl;
			return true;
			break;
	}


	if (strcmp(argv[1], "-r")==0){
		USBGeigerCounterPacket p;
		bool continues=false;
		bool cls = false;
		if (argc>=3 && strcmp(argv[2], "-c")==0) continues = true;
		if (argc>=4 && strcmp(argv[3], "-c")==0) continues = true;
		if (argc>=3 && strcmp(argv[2], "-cls")==0) cls = true;
		if (argc>=4 && strcmp(argv[3], "-cls")==0) cls = true;
		
		counter.turn_auto_usb_off();
		do {
			if (counter.poll()){
				usleep(100000);
				int timeout=0;
				while ((!counter.read_packet(& p) || p.type != USB_DATA) && timeout < 20){ //read until received a packet that is not a CMD packet
					counter.poll();
					usleep(100000);
					timeout++;
				} 
				if (timeout < 20){
					if (cls) cout << "\033[2J\033[0;0f";
					cout << "Counts:          " << p.counter << endl <<
							"Time counting:   " << p.time_counting << "s" << endl <<
							"Pulse width:     " << (p.last_pulsewidth * 2 / 3) << "us" << endl <<
							"Threshold:       " << (p.threshold * 2 / 3) << "us" << endl <<
							"Tube voltage:    " << calculate_tube_voltage ((int)p.adc_value, (int)p.adc_calibration) << "V" << endl <<
							"ADC cal value:   " << p.adc_calibration << endl <<
							"DCDC duty cycle: " << p.duty_cycle << endl <<
							"Watchdog events: " << p.wdt << endl <<
							"Errors:          " << p.error << endl;
				}else{
					cout << "Read failed." << endl;
				}
			}else{
				cout << "Poll failed." << endl;
			}
			if (kbhit()) continues = false; //stop polling
			if (continues) usleep(800000);
			if (continues) cout << endl;
		}while (continues);
	}else if (strcmp(argv[1], "-adc")==0){
		if (argc<3){
			cout << "Missing value argument." << endl;
			return true;
		}
		int value = (int) strtol(argv[2], NULL, 10);
		counter.set_adc_calibration (value);
		cout << "ADC calibration value set to " << value << endl;
	}else if (strcmp(argv[1], "-thres")==0){
		if (argc<3){
			cout << "Missing value argument." << endl;
			return true;
		}
		int value = (int) strtol(argv[2], NULL, 10);
		counter.set_threshold(value);
		cout << "Threshold set to " << value << "us" << endl;
	}else if (strcmp(argv[1], "-save")==0){
		if (counter.save_settings()) cout << "Settings saved in EEPROM." << endl;
		else cout << "Failed to save settings." << endl;
	}else if (strcmp(argv[1], "-default")==0){
		counter.set_default_settings();
		cout << "Settings set to default." << endl;
	}else if (strcmp(argv[1], "-reset")==0){
		counter.reset_chip();
		cout << "Reset PIC." << endl;
	}else if (strcmp(argv[1], "-clear")==0){
		counter.reset_counter();
		cout << "Counter value reset" << endl;
	}else if (strcmp(argv[1], "-pwm")==0){
		if (argc<3){
			cout << "Missing value argument." << endl;
			return true;
		}
		int value =  (int) strtol(argv[2], NULL, 10);
		counter.disable_auto_pwm();
		counter.set_pwm(value);
		cout << "PWM pulse width set to " << value << endl;
	}else if (strcmp(argv[1], "-autopwm")==0){
		if (argc >= 3 && argv[2][0]=='0'){
			counter.disable_auto_pwm();
			cout << "Disabled." << endl;
		}else{
			counter.enable_auto_pwm();
			cout << "Enabled." << endl;
		}
	}else if (strcmp(argv[1], "-sw")==0){
		while (!counter.turn_auto_usb_off());
		unsigned char buffer[64];
		while (counter.read_packet((USBGeigerCounterPacket*) & buffer[0])); //clear the fifo
		counter.get_sw_version();
		usleep(100000);
		while (!counter.read_packet((USBGeigerCounterPacket*) & buffer[0])){ //read packet
			counter.get_sw_version();
			usleep(100000);
		}
		buffer[63]=0;
		cout << & buffer[3] << endl;
	}else{
		cout << "Command line arguments: " << endl <<
			    "   -C <file>      Uses <file> as configuration file, default is geiger.conf, must be first option!" << endl <<
			    "	-r   		   Reads the counter values and prints it on the screen." << endl <<
				"   -r -c          Continues read and output values every second until a key is pressed." << endl <<
				"   -r -c -cls     Clear screen after each output." << endl <<
				"	-adc [value]   Sets the ADC calibration value (for measuring the voltage)." << endl << 
				"   -thres [value] Sets the min pulse width for detection in µs." << endl <<
				"   -save		   Save the threshold and ADC value to EEPROM." << endl <<
				"   -default       Reset stored settings to defaults." << endl <<
				"   -reset		   Resets the counter chip." << endl <<
				"   -clear         Resets the counter value." << endl <<
				"   -pwm [value]   Sets the PWM duty cycle (adjusts tube voltage)." << endl <<
				"   -autopwm [1,0] Enables/disables automatic adjustment of the pwm to maintain at least 400V." << endl <<
				"   -sw            Reads the firmware version of the counter." << endl;
	}

	return true;
}



int main(int argc, char **argv){

	string config_file(DEFAULT_CONFIG_FILE);

	//check if we need to change the default config file location!
	if (argc>2){
		if (strcmp(argv[1], "-C")==0){
			config_file.assign(argv[2]);
			argc-=2;
			argv+=2;
		}
	}

	if (argc>1){
		if (execute_command(argc, argv, config_file.c_str())) return 0; //execute commands
	}

	cout << "Automatic USB Geiger counter logger V" << APP_VERSION << "." << endl;
	cout << "Loading configuration file " << config_file  << "." << endl;
	
	app.set_sampling_interval(SAMPLING_INTERVAL_TIME);
	app.load_settings(config_file.c_str());
	
	LOG_DEBUG ("Connecting to USB Device...");
	
	cout << "Hit any key to exit this program." << endl;

	while (true){
		HID_ERROR_MESSAGES connect_result = app.counter.connect();
		if (connect_result==HID_ERROR_NONE){
			cout << "Connected to device:" << endl;
			cout << " " << app.counter.hid.get_product() << endl;
			cout << "  -path: " << app.counter.hid.get_dev_path() << endl;
			cout << "  -vid: " << app.counter.hid.get_vid() << endl;
			cout << "  -pid: " << app.counter.hid.get_pid() << endl;
			cout << "start polling of data..." << endl;

			LOG_DEBUG("Connected to device, path: " << app.counter.hid.get_dev_path() << ", vid: " << app.counter.hid.get_vid() << ", pid: " << app.counter.hid.get_pid());
			LOG_DEBUG("Start polling of data.");

			start_polling();
			app.counter.disconnect(); //polling stopped for some reason
		}else{
			switch(connect_result){
				case HID_ERROR_CANT_CREATE_UDEV:
					LOG_ERROR("Could not connect to device: " << connect_result << " libudev not found.");
					break;
				case HID_ERROR_UNABLE_TO_FIND_PARENT_USB_DEVICE:
					LOG_ERROR("Could not connect to device: " << connect_result << " can't find device.");
					break;
				case HID_ERROR_CANT_OPEN_FILE:
					LOG_ERROR("Could not connect to device: " << connect_result << " can't open file, try running with sudo!");
					break;
				case HID_ERROR_DEVICE_NOT_FOUND:
					LOG_ERROR("Could not connect to device: " << connect_result << " no device found, check /dev.");
					break;
				default:
					LOG_ERROR("Unknown Connection error.");
					break;
			}
		}

		LOG_ERROR("Connection lost, reconnecting in 15 seconds.");
		std::flush(cout);
		usleep(15000000); //wait for 15 seconds
	}

	cout << endl;
	return 0;
}
