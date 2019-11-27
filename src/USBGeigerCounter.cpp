#include "USBGeigerCounter.h"
#include "hid.h"
#include <stdio.h>
#include <iostream>

#define GC_VENDOR_ID 0x04D8 
#define GC_PRODUCT_ID 0x000C
#define GC_NR_RETRY 10

#define GC_CMD_AUTO_SEND_USB_OFF "auto usb 0"
#define GC_CMD_AUTO_SEND_USB_ON "auto usb 1"
#define GC_CMD_CLEAR_COUNTER "clear"
#define GC_CMD_GET "get"
#define GC_CMD_RESET "reset"
#define GC_CMD_REMOTE_WDT_OFF "rem watchdog 0"
#define GC_CMD_REMOTE_WDT_ON "rem watchdog 1"
#define GC_CMD_WDT_RESET "wdt"
#define GC_CMD_ADC_CALIBRATE "cal %d"
#define GC_CMD_SAVE "save"
#define GC_CMD_DEFAULT_SETTINGS "default"
#define GC_CMD_SET_THRESHOLD "threshold %d"
#define GC_CMD_SET_PWM "pwm %d"
#define GC_CMD_ENABLE_AUTO_ADJUST_PWM "auto pwm 1"
#define GC_CMD_DISABLE_AUTO_ADJUST_PWM "auto pwm 0"
#define GC_CMD_GET_SOFTWARE_VERSION "sw ver"

#define GC_GEIGER_FLAG_AUTO_PWM 1       //set if the automatic pwm adjust is on
#define GC_GEIGER_FLAG_WDT 2		    //set if the watchdog timer on USB connection is on, the microcontroller expects to receive an USB command at least every 4 seconds or it will automatically reset!
#define GC_GEIGER_FLAG_AUTO_USB 4       //set if packets are automatically send on usb interface each 100ms



USBGeigerCounter::USBGeigerCounter(){
	vid = GC_VENDOR_ID;
	pid = GC_PRODUCT_ID;
}


USBGeigerCounter::~USBGeigerCounter(){
	hid.close_device();
}

HID_ERROR_MESSAGES USBGeigerCounter::connect(){
	return hid.open_device(vid,pid);
}

void USBGeigerCounter::disconnect(){
	hid.close_device ();
}

bool USBGeigerCounter::read_packet(USBGeigerCounterPacket * packet){
	if (hid.read_data_retry((void* )packet,sizeof(USBGeigerCounterPacket), GC_NR_RETRY)){
		last_response_time = time(0);
		return true;
	}
	return false;
}

bool USBGeigerCounter::reset_device(){
	return hid.reset_device ();
}

bool USBGeigerCounter::poll(){
	return hid.send_data_retry(GC_CMD_GET,GC_NR_RETRY);
}

bool USBGeigerCounter::reset_counter(){
	return hid.send_data(GC_CMD_CLEAR_COUNTER);
}

bool USBGeigerCounter::reset_chip(){
	return hid.send_data(GC_CMD_RESET);
}

bool USBGeigerCounter::turn_auto_usb_off(){
	return hid.send_data(GC_CMD_AUTO_SEND_USB_OFF);
}

bool USBGeigerCounter::turn_auto_usb_on(){
	return hid.send_data(GC_CMD_AUTO_SEND_USB_ON);
}

bool USBGeigerCounter::turn_remote_wdt_on(){
	return hid.send_data(GC_CMD_REMOTE_WDT_ON);
}

bool USBGeigerCounter::turn_remote_wdt_off(){
	return hid.send_data(GC_CMD_REMOTE_WDT_OFF);
}

time_t USBGeigerCounter::get_last_response_time(){
	return last_response_time;
}

bool USBGeigerCounter::reset_wdt(){
	return hid.send_data(GC_CMD_WDT_RESET);
}

bool USBGeigerCounter::set_adc_calibration(int val){
	char buffer[32];
	sprintf(buffer,GC_CMD_ADC_CALIBRATE , val);
	return hid.send_data(buffer);
}

bool USBGeigerCounter::save_settings(){
	return hid.send_data(GC_CMD_SAVE);
}

bool USBGeigerCounter::set_default_settings(){
	return hid.send_data(GC_CMD_DEFAULT_SETTINGS);
}

bool USBGeigerCounter::set_threshold(int us){
	char buffer[32];
	sprintf(buffer,GC_CMD_SET_THRESHOLD , us * 3/2);
	return hid.send_data(buffer);
}

bool USBGeigerCounter::set_pwm(int val){
	char buffer[32];
	sprintf(buffer,GC_CMD_SET_PWM , val );
	return hid.send_data(buffer);
}

bool USBGeigerCounter::enable_auto_pwm(){
	return hid.send_data(GC_CMD_ENABLE_AUTO_ADJUST_PWM);
}

bool USBGeigerCounter::disable_auto_pwm(){
	return hid.send_data(GC_CMD_DISABLE_AUTO_ADJUST_PWM);
}

bool USBGeigerCounter::get_sw_version(){
	return hid.send_data(GC_CMD_GET_SOFTWARE_VERSION);
}


int USBGeigerCounter::get_vid(){
	return vid;
}

int USBGeigerCounter::get_pid(){
	return pid;
}

void USBGeigerCounter::set_vid(int val){
	vid = val;
}

void USBGeigerCounter::set_pid(int val){
	pid = val;
}


float USBGeigerCounter::cpm2rad(float cpm, float volt){
	float x = 0.5 * (volt - (float)RAD_WORKING_VOLTAGE) / 10000.0;
	
	float cps_mrh = (float)RAD_SENSETIVITY * (x + 1.0) / (1.0-x); //adjusted counts per second per millirontgen per hour (adjusted to the current tube voltage
	float usvh_pcm = 1.0 / (cps_mrh / 10.0 * 60.0); // microsievert per hour per count per minute
	
	return usvh_pcm * cpm;
}