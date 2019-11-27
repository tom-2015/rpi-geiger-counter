*******************************************************
*USB Geiger counter software for Raspberry Pi / Linux *
*http://radioactivity.dfzone.be/                      *
*******************************************************

Use this software to automatically upload data from your
USB geiger counter to a website or Mysql database

Installation:

1. Attach your counter to the USB port of your Raspberry
   and turn the power on.

2. Install some required libs and programs:
   For running the binary:
   sudo apt-get update
   sudo apt-get install curl libcurl4-openssl-dev libmysqlclient-dev libudev-dev make libusb-dev
   For compiling you need:
   sudo apt-get install libcurl4-openssl-dev default-libmysqlclient-dev libudev-dev libusb-dev
   
   on some new Raspbian version libmysqlclient-dev is named default-libmysqlclient-dev
   
3. Now you can clone and compile the code:
   git clone https://github.com/tom-2015/rpi-geiger-counter.git
   cd rpi-geiger-counter
   make
   
*The following steps are only needed if you want to automatically upload the
*number of counts measured to our website.

4. Create account and add a counter.
   Go to radioactivity.dfzone.be and create an account. 
   Log in to your account and hover your user name in the navigation bar
   you should see a drop down menu item called 'my counters', click it. 
   Here you click 'Add new counter', provide the needed information and click the save button. 
   A counter ID will now be displayed at the top of the page (copy it).
   
5. Edit configuration:
   cd bin
   nano geiger.conf
   
6. In the configuration file you fill in the counter id and counter password from
   step 4 after http_counter_id=<your id here> and http_counter_password=<your password>
   Also enable the http_client: http_upload_enabled=1

   If you want the device on https://www.uradmonitor.com/:
   Create an account and enter the urad details below [urad_monitor_client]
   urad_client_enabled = 1
   urad_user_id=
   urad_user_key=
   The urad_user_id and urad_user_key can be obtained from the uradmonitor site, first create account.
   Then login and got to dashboard, API there it shows user-id and user-key.
   
7. Exit nano (with CTR+X) and type Y, hit enter to save the data

8. Now you can make the program executable and run it:
   sudo chmod 777 geiger.elf
   sudo ./geiger.elf
   
   you should see something like this on the console:
   
   Automatic USB Geiger counter logger V3.
   Loading configuration file geiger.conf.
   Hit any key to exit this program.
   Connected to device:
   USB Geiger Counter
     -path: /dev/hidraw0
     -vid: 1240
     -pid: 12
   start polling of data...
   
   after about 10 minutes you should see data uploaded to your account
   
9. Make the program start automatically when you power on your Raspberry
   
   -If your Raspberry pi boots to desktop you can create a desktop entry for it:
	   cd ~
	   cd .config
	   cd autostart
	   sudo nano geiger_counter.desktop
	   
	   Enter the following text:
	   [Desktop Entry]
		Encoding=UTF-8
		Type=Application
		Name=geiger_counter
		Comment=
		Exec=sudo <enter path to the geiger.elf executable here>
		StartupNotify=false
		Terminal=false
		Hidden=false
		
		CTRL+X and choose Y to save
	
	-If your Raspberry boots to command line you can create a daemon script like this:
		copy the daemon.sh from the directory of this readme file to /etc/init.d:
		sudo cp daemon.sh /ect/init.d/geigercounter
		
		edit the the file you copied
		sudo nano /etc/init.d/geigercounter
		
		Find text "DAEMON=<place path to geiger.elf executable here>"
		Replace it with the location of the geiger.elf file in the bin directory.
		For example: 
		DAEMON=/home/pi/rpi-geiger-counter/bin/geiger.elf
		Ctrl+X to close nano and save changes.
		
		Now you update the daemon to auto start:
		update-rc.d /etc/init.d/geigercounter defaults 90
		

*******************************************************
*HTTP Server         								  *
*******************************************************	
	
This options enables an internal web server for easy visualising
the measured 'radiation' in a web browser. Set enable_http_server
to 1 to enable it.

enable_http_server = 1
http_server_port = 79
http_www_dir = www
http_www_enabled = 1

if http_www_enabled pages from the http_www_dir will be served 
automatically. If you don't have apache running you can change
the http_server_port to 80.
By default the webserver will display the last packet as json
data on: http://localhost:79/json

*******************************************************
*Saving to MYSQL     								  *
*******************************************************	

You can save the measured samples to your MYSQL database.
See the MYSQL section in the geiger.conf file.

[mysql]
mysql_upload_enabled = 0
mysql_host = localhost
mysql_user = root
mysql_password =
mysql_db =
mysql_table =
mysql_insert_query = INSERT INTO %t (`start_time`, `end_time`, `duration`, `counts`) VALUES (FROM_UNIXTIME(%s), FROM_UNIXTIME(%e), %d, %c);

To enable set mysql_upload_enabled=1 and fill in your MYSQL
host, user name, password, database (db), table and query.
In the query you can use the following variables:
%t     mysql_table name
%s     start time (UTC)
%s     stop time
%c     number of counts detected in that period. 
	
*******************************************************
*Saving to text files								  *
*******************************************************

Use this option if you want to save the samples into a text
file. Every 300 seconds a text file will be generated with
the number of counts measured in that period. The period
can be adjusted by changing the 'interval=300' line.
Note: If you want to upload the measured data to our website
      please do not change the interval!

You can enable it in the geiger.conf file:
[text]
text_dir=/dev/shm
text_enabled=1
#text_exec_cmd=

Change the directory to save the text files (text_dir). The files
have a name like gc_*.txt where * is an incrementing number for each
file saved. The content of each file will look like this:
start=<timestamp>
stop=<timestamp>
counts=<nr of counts>

Don't forget to set text_enabled=1.

Optional you can enter a command to be executed every time a file
is saved with 'text_exec_cmd='. At the end of this command the
path to the saved file will be added automatically.

*******************************************************
*Command line options                                 *
*******************************************************

-Test the connection to your counter with:
  sudo ./geiger.elf -r
  
  This should print something like:
	Counts:          22465
	Time counting:   262933s
	Pulse width:     76µs
	Threshold:       65µs
	Tube voltage:    424V
	ADC cal value:   1000000
	DCDC duty cycle: 3158
	Watchdog events: 0
	Errors:          0
	
-Read info every second and display in the terminal:
  sudo ./geiger.elf -r -c -cls
  
-Calibrate the measured voltage / ADC
  sudo ./geiger.elf -adc <val>
  
  If the measured voltage on the counter tube is not accurate
  enough you can save a calibration factor in the EEPROM of 
  you counter. To do so you should measure the exact value 
  with a volt meter and calculate a factor the measured ADC
  value must be multiplied with. For example if you measure
  450V and the software says it's 460V the factor is 
  450/460 = 0.978260 Multiply this with 1000000, this gives 
  978260. Execute sudo ./geiger.elf -adc 978260
  To save the value in the EEPROM of the counter do:
  sudo ./geiger.elf -save
 
-Set threshold value
  sets the threshold value, this is the minimum pulse time 
  coming out of the tube before the counter will register it as 
  a 'count'.
  sudo ./geiger.elf -thres <threshold in µs>
  
-Full reset of the counter PIC micro controller
  sudo ./geiger.elf -reset 
 
-Reset only the counter value
  sudo ./geiger.elf -clear
  
-Disable internal voltage regulation (do this before using -pwm)
  sudo ./geiger.elf -autopwm 0
 
-Adjust the output voltage on the counter tube by changing the 
 PWM duty cycle value
  sudo ./geiger.elf -pwm <value>
  value goes from 0 to 3195 and will change the output voltage
  
-Read firmware version
  sudo ./geiger.elf -sw
 
-Reset settings to default values
  sudo ./geiger.elf -default
  

	
	
