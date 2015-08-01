#include "hid.h"
//some usefull links about this:

//https://www.kernel.org/doc/Documentation/hid/hidraw.txt
//http://lxr.hpcs.cs.tsukuba.ac.jp/linux/samples/hidraw/hid-example.c
//http://www.signal11.us/oss/udev/
//alternative way
//http://ldd6410-2-6-28.googlecode.com/svn/trunk/linux-2.6.28-samsung/drivers/hid/hidraw.c
//http://askubuntu.com/questions/645/how-do-you-reset-a-usb-device-from-the-command-line


/*
 * Hidraw Userspace Example
 *
 * Copyright (c) 2010 Alan Ott <alan@signal11.us>
 * Copyright (c) 2010 Signal 11 Software
 *
 * The code may be used by anyone for any purpose,
 * and can serve as a starting point for developing
 * applications using hidraw.
 */

/* Linux */
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

/*
 * Ugly hack to work around failing compilation on systems that don't
 * yet populate new version of hidraw.h to userspace.
 *
 * If you need this, please have your distro update the kernel headers.
 */
#ifndef HIDIOCSFEATURE
	#define HIDIOCSFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
	#define HIDIOCGFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)
#endif

/* Unix */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libudev.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>

/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>
#include <unistd.h>
#include <cstdlib>
#include <time.h>
#include <usb.h>





using namespace std;

raw_hid_device::raw_hid_device (){
	this->fd=-1;
}

raw_hid_device::~raw_hid_device(){
	close_device();
}

void raw_hid_device::reset(){
	close_device();
	this->dev_path.clear();
	this->manufacturer.clear();
	this->serial.clear();
	this->product.clear();
	this->pid=0;
	this->vid=0;
	this->fd=-1;
}

bool raw_hid_device::reset_device(){
	bool res = ioctl(fd, USBDEVFS_RESET, 0)==0;
	close_device();
	return res;
}

void raw_hid_device::close_device(){
	if (this->fd>=0) close(this->fd);
}

//returns the device path of a device given a vendor id and product id
HID_ERROR_MESSAGES raw_hid_device::open_device (int vid,int pid){
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev;
	
	reset(); //reset fields
	
	/* Create the udev object */
	udev = udev_new();
	if (!udev) {
		return HID_ERROR_CANT_CREATE_UDEV;
	}
	
	/* Create a list of the devices in the 'hidraw' subsystem. */
	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "hidraw");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);
	/* For each item enumerated, print out its information.
	   udev_list_entry_foreach is a macro which expands to
	   a loop. The loop will be executed for each member in
	   devices, setting dev_list_entry to a list entry
	   which contains the device's path in /sys. */
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path;
		const char *dev_node_path;
		/* Get the filename of the /sys entry for the device
		   and create a udev_device object (dev) representing it */
		path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);

		/* usb_device_get_devnode() returns the path to the device node
		   itself in /dev. */
		dev_node_path=udev_device_get_devnode(dev);
		//printf("Device Node Path: %s\n", dev_node_path);

		/* The device pointed to by dev contains information about
		   the hidraw device. In order to get information about the
		   USB device, get the parent device with the
		   subsystem/devtype pair of "usb"/"usb_device". This will
		   be several levels up the tree, but the function will find
		   it.*/
		dev = udev_device_get_parent_with_subsystem_devtype(dev,"usb", "usb_device");
		if (!dev) {
			return HID_ERROR_UNABLE_TO_FIND_PARENT_USB_DEVICE;
		}
	
		/* From here, we can call get_sysattr_value() for each file
		   in the device's /sys entry. The strings passed into these
		   functions (idProduct, idVendor, serial, etc.) correspond
		   directly to the files in the directory which represents
		   the USB device. Note that USB strings are Unicode, UCS2
		   encoded, but the strings returned from
		   udev_device_get_sysattr_value() are UTF-8 encoded. */
		string vendor = udev_device_get_sysattr_value(dev,"idVendor");
		string product = udev_device_get_sysattr_value(dev, "idProduct");

		int vendor_id_int =(int)strtol(vendor.c_str(), NULL, 16 ) ;
		int product_id_int = (int) strtol(product.c_str(), NULL, 16 ); 

		

		/*printf("  VID/PID: %s %s\n", vendor.c_str() , product.c_str());
		printf("  %s\n  %s\n", udev_device_get_sysattr_value(dev,"manufacturer"), udev_device_get_sysattr_value(dev,"product"));
		printf("  serial: %s\n", udev_device_get_sysattr_value(dev, "serial"));*/

		if (vendor_id_int == vid && product_id_int == pid){
			this->dev_path.assign(dev_node_path);
			if (udev_device_get_sysattr_value(dev,"manufacturer")!=NULL) this->manufacturer.assign(udev_device_get_sysattr_value(dev,"manufacturer"));
			if (udev_device_get_sysattr_value(dev, "serial")!=NULL) this->serial.assign(udev_device_get_sysattr_value(dev, "serial"));
			if (udev_device_get_sysattr_value(dev,"product")!=NULL) this->product.assign(udev_device_get_sysattr_value(dev,"product"));

			this->vid = vendor_id_int;
			this->pid = product_id_int;

			udev_device_unref(dev);
			
			break;
			/*printf("Device found!");
			result = dev_node_path; //save the result
			break; //break out foreach, we found what we are looking for*/
		}

		udev_device_unref(dev);
	}

	/* Free the enumerator object */
	udev_enumerate_unref(enumerate);

	udev_unref(udev);

	if (this->dev_path.size() > 0){
		this->fd = open(this->dev_path.c_str(), O_RDWR|O_NONBLOCK|O_TRUNC); // Open the Device with non-blocking reads. In real life,don't use a hard coded path; use libudev instead. 
		if (this->fd < 0){
			return HID_ERROR_CANT_OPEN_FILE;
		}else{
			return HID_ERROR_NONE;
		}
	}

	return HID_ERROR_DEVICE_NOT_FOUND;       
}



//sends data
//sends cmd command to usb device
bool raw_hid_device::send_data (const char * cmd, int len){
	if (fd<0) return false;
	return write(fd, cmd, len) > 0;
}

//sends a command
//returns true if no errors
bool raw_hid_device::send_data (const char * cmd){
	return send_data(cmd, strlen(cmd));
}

//sends the command, if failed we will retry nr_retry times
bool raw_hid_device::send_data_retry(const char * cmd, int len, int nr_retry){
	int i=0;
	while (!send_data(cmd,len) && i<nr_retry) i++;
	return i<nr_retry;
}

bool raw_hid_device::send_data_retry(const char * cmd, int nr_retry){
	return send_data_retry(cmd, strlen(cmd), nr_retry);
}

//reads data and puts in buff with a maxlen read length
//returns true if data was written to the buffer
bool raw_hid_device::read_data (void * buff, int maxlen){
	if (fd<0) return false;
	return read(fd, buff, maxlen) > 0;
}

//same as read_data but will retry nr_retry times in case of a failure
bool raw_hid_device::read_data_retry(void * buff, int maxlen, int nr_retry){
	int i=0;
	while (!read_data(buff, maxlen) && i<nr_retry) i++;
	return i<nr_retry;
}


int raw_hid_device::get_vid(){
	return vid;
}

int raw_hid_device::get_pid(){
	return pid;
}

string raw_hid_device::get_dev_path(){
	return dev_path;
}

string raw_hid_device::get_manufacturer(){
	return manufacturer;
}

string raw_hid_device::get_serial(){
	return serial;
}

string raw_hid_device::get_product(){
	return product;
}



/*const char * bus_str(int bus){
	switch (bus) {
	case BUS_USB:
		return "USB";
		break;
	case BUS_HIL:
		return "HIL";
		break;
	case BUS_BLUETOOTH:
		return "Bluetooth";
		break;
	case BUS_VIRTUAL:
		return "Virtual";
		break;
	default:
		return "Other";
		break;
	}
}*/