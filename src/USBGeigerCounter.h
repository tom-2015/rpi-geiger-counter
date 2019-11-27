#ifndef USB_GEIGER_COUNTER_H
#define USB_GEIGER_COUNTER_H



#include "hid.h"
#include <ctime>

#define RAD_SENSETIVITY 11 //cps / (mR/h) 1 mR/h = 10µSv/h @ working voltage the counts per second with radiation of 1 m rontgen per hour
#define RAD_WORKING_VOLTAGE 400 //in datasheet nominal working voltage
#define RAD_PLATEAU_SLOPE 15 // % per 100V


typedef enum {
	USB_DATA=68,
	USB_CMD=79
}USBGeigerCounterTypes;

typedef struct {
	unsigned char  type;			 //packet type 68 (=D) DATA PACKET, 79 CMD RESPONS (=O)
	unsigned char  version;			 //packet version, 1
	unsigned short flags;			 //flags
	unsigned int   counter; 		 //geiger counter value (hits since last reset)
	unsigned int   time_counting; 	 //time since last counter reset
	unsigned int   time_on; 	     //time since boot of chip
	unsigned int   adc_calibration;  //stores calibaration value of voltage ADC (* 1000 000 000)  
	unsigned short threshold; 		 //threshold time in 2/3µs
	unsigned short last_pulsewidth;  //last pulse width
	unsigned short adc_value;	     //adc_value on geiger tube (ADC value 0-1023), U = adc_value * 4096 / 1024 / 1000 / 4700 * 1004700 * adc_calibration / 1000000
	unsigned short error;			 //number of pulse errors (very long pulses)
	unsigned short wdt;				 //number of watchdog timer resets
	unsigned short duty_cycle;		 //duty cycle value
	unsigned int   last_pulse_time;  //time the last pulse was detected
	unsigned short max_pwm_value;    //holds the max duty cycle value
	unsigned char padding[26];		 //reserved bytes to fill up the packet to make 64bytes
} USBGeigerCounterPacket;

#define test

class USBGeigerCounter {
private:
	time_t last_response_time;
	int pid, vid;
public:
	raw_hid_device hid;
	
	USBGeigerCounter ();
	~USBGeigerCounter();

	HID_ERROR_MESSAGES connect(); //connects to the device, returns HID_ERROR_MESSAGES
	void disconnect(); //disconnects

	float cpm2rad(float cpm, float volt); //converts counts per min to µSv/h
	bool read_packet(USBGeigerCounterPacket * packet); //read a packet which will be stored in packet
	bool poll(); //sends poll command
	bool reset_counter(); //resets the counter value
	bool reset_chip(); //resets the chip (sends a command to reset the entire pic + counter value)
	bool reset_device(); //resets the usb protocol (not the counter/PIC chip)
	bool turn_auto_usb_off();
	bool turn_auto_usb_on();
	bool turn_remote_wdt_on();
	bool turn_remote_wdt_off();
	bool reset_wdt();
	bool set_adc_calibration(int val);
	bool save_settings();
	bool set_default_settings();
	bool set_threshold(int us);
	bool set_pwm(int val);
	bool enable_auto_pwm();
	bool disable_auto_pwm();
	bool get_sw_version();
	int get_vid();
	int get_pid();
	void set_vid(int val);
	void set_pid(int val);
	time_t get_last_response_time();
};

#endif