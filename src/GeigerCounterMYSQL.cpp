#include "GeigerCounterMYSQL.h"
#include <mysql/mysql.h>
#include <pthread.h>
#include "log.h"
#include "functions.h"
#include <sstream>

using namespace std;

MYSQLclient::MYSQLclient(GeigerCounterApplication * app):GeigerCounterExtension(app), name("MYSQLClient"){
	enabled=false;
	keep_uploading=false;
	mysql_table.assign("counter");
	mysql_insert_query.assign("INSERT INTO %t (`start_time`, `end_time`, `duration`, `counts`) VALUES (FROM_UNIXTIME(%s), FROM_UNIXTIME(%e), %d, %c);");
	name.assign("mysqlclient");
}

MYSQLclient::~MYSQLclient(){
	disable();
}

void MYSQLclient::enable(){
	if (!keep_uploading){ //check already enabled
		if (enabled){
			LOG_DEBUG ("Mysql client enabled, using server " << mysql_host);
			keep_uploading = true;
			pthread_create( & client_thread, NULL, (void* (*)(void*)) & MYSQLclient::client_thread_helper, this);
		}
	}
}

void MYSQLclient::disable(){
	if (keep_uploading){
		keep_uploading =false;
		pthread_join( client_thread,NULL);
	}
}

const char * MYSQLclient::get_name(){
	return name.c_str();
}

//process all the mysql settings lines
void MYSQLclient::read_setting (const std::string & key, const std::string & value){
	if (key.compare("mysql_host")==0){
		mysql_host = value;
	}else if (key.compare("mysql_user")==0){
		mysql_user = value;
	}else if (key.compare("mysql_password")==0){
		mysql_pwd = value;
	}else if (key.compare("mysql_db")==0){
		mysql_db = value;
	}else if (key.compare("mysql_insert_query")==0){
		mysql_insert_query = value;
	}else if (key.compare("mysql_table")==0){
		mysql_table = value;
	}else if (key.compare("mysql_upload_enabled")==0){
		enabled = value.compare("1")==0;
	}
}

void MYSQLclient::on_interval(const GeigerCounterIntervalData & data){
	if (enabled)	data_fifo.write(data); //5min sample stored (async buffer)
}

void MYSQLclient::on_packet_received (const USBGeigerCounterPacket & data){
//USB data received, do nothing
}

void * MYSQLclient::client_thread_helper(void * param){
	((MYSQLclient *) param)->start_upload();
}

void MYSQLclient::start_upload(){
	while (keep_uploading){
		if (data_fifo.count()>0){
			upload_buffer();
		}
		usleep(100000);
	}
	pthread_exit(NULL);
	return;
}

// http://zetcode.com/db/mysqlc/
void MYSQLclient::upload_buffer(){
	MYSQL *con = mysql_init(NULL);
	string query_string;
	char number[32];

	LOG_DEBUG ("Updating mysql table");

	if (con == NULL){
		LOG_ERROR ("Mysql init error: " << mysql_error(con));
		return; //exit(1);
	}

	if (mysql_host.length()<=0){
		LOG_ERROR("No mysql host set in settings.txt.");
		return;
	}

	if (mysql_real_connect(con, mysql_host.c_str(), mysql_user.c_str(), mysql_pwd.c_str(), mysql_db.c_str(), 0, NULL, 0) == NULL){
	  LOG_ERROR("Mysql connection error: " << mysql_error(con));
	  mysql_close(con);
	  return;
	}

	GeigerCounterIntervalData data;
	while (data_fifo.peek(data)){ //read, don't remove in case of a mysql error
		//char query[512];
		//sprintf(query, query_string, mysql_table.c_str(), data.start_time, data.end_time, (data.end_time-data.start_time) ,data.counts);

		query_string = str_replace(mysql_insert_query, "%t", mysql_table);
		sprintf(number, "%u", data.start_time);
		query_string = str_replace(query_string, "%s", number);
		sprintf(number, "%u", data.end_time);
		query_string = str_replace(query_string, "%e", number);
		sprintf(number, "%u", (data.end_time - data.start_time));
		query_string = str_replace(query_string, "%d", number);
		sprintf(number, "%u", data.counts);
		query_string = str_replace(query_string, "%c", number);

		if (mysql_query(con, query_string.c_str())){
			LOG_ERROR("Mysql error: " << mysql_error(con) << " query_string: " << query_string );
			mysql_close(con);
			return;
		}
		data_fifo.read(data); //read & remove
	}
    mysql_close(con);
}

bool MYSQLclient::is_enabled(){
	return enabled;
} 

std::string MYSQLclient::get_status_json(){
	ostringstream res;
	res << "{\"enabled\": " << (enabled ? 1 : 0) << "}";
	return res.str();
}