#include <string>

#ifndef RAW_HID_H
	#define RAW_HID_H

	typedef enum {
		HID_ERROR_NONE =0,
		HID_ERROR_CANT_CREATE_UDEV=1,
		HID_ERROR_UNABLE_TO_FIND_PARENT_USB_DEVICE=2,
		HID_ERROR_CANT_OPEN_FILE=3,
		HID_ERROR_DEVICE_NOT_FOUND=4
	}HID_ERROR_MESSAGES;

	class raw_hid_device {
		protected:
			int fd;
			int vid;
			int pid;
			std::string dev_path;
			std::string manufacturer;
			std::string serial;
			std::string product;

			void reset(); //reset all fields

		public:
			raw_hid_device ();
			~raw_hid_device();
			HID_ERROR_MESSAGES open_device (int vid, int pid); //tries to open the device with vid and pid, returns 0 when success or one of the error constants if not
			void close_device(); //closes the connection to the device
			bool reset_device(); //resets the usb connection

			bool send_data (const char * cmd, int len); //sends data to the device (cmd= data with length len) returns true on success
			bool send_data (const char * cmd); //sends zero ended string
			bool send_data_retry(const char * cmd, int len, int nr_retry);//sends the command, if failed we will retry nr_retry times
			bool send_data_retry(const char * cmd, int nr_retry); //sends a zero ended string, if failed will retry nr_retry times

			bool read_data (void * buff, int maxlen); //reads data and puts in buff with a maxlen read length returns true if data was written to the buffer
			bool read_data_retry(void * buff, int maxlen, int nr_retry); //same as read_data but will retry nr_retry times in case of a failure

			int get_vid();
			int get_pid();
			
			std::string get_dev_path();
			std::string get_manufacturer();
			std::string get_serial();
			std::string get_product();

	};

#endif