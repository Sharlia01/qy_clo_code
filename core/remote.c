#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

#include "interface_manage.h"
#include "protocol.h"
#include "addr_req.h"

static sqlite3 *remote_db = NULL;
#define REMOTE_TABLE_NAME "remote"
#define DIMMER_TABLE_NAME "dimmer"
static sqlite3 *dimmer_db = NULL;


void remote_db_open()
{
	int rc;
	char *errmsg;
	char sql[100] = {0};

	rc = sqlite3_open(REMOTE_DB_PATH, &remote_db);
	if(rc != SQLITE_OK){
		printf("db open failed!!!, %d\n", rc);
		return;
	}

	sprintf(sql, "create table %s(num int,state int,value int,devaddr int,\
	subaddr int,sceneid int)",REMOTE_TABLE_NAME);
	rc = sqlite3_exec(remote_db, sql, NULL, NULL, &errmsg);
	if (rc == SQLITE_OK)
		printf("create remote db success!!!\n");
}

void dimmer_db_open()
{
	int rc;
	char *errmsg;
	char sql[100] = {0};

	rc = sqlite3_open(DIMMER_DB_PATH, &dimmer_db);
	if(rc != SQLITE_OK){
		printf("db open failed!!!, %d\n", rc);
		return;
	}

	sprintf(sql, "create table %s(dev int,sub int,value int)",DIMMER_TABLE_NAME);
	rc = sqlite3_exec(dimmer_db, sql, NULL, NULL, &errmsg);
	if (rc == SQLITE_OK)
		printf("create dimmer db success!!!\n");
}



void remote_db_close(){
	int rc;
    rc = sqlite3_close(remote_db);
    if (rc != SQLITE_OK)
        printf("remote db close failed!!!, %d\n", rc);
}

void dimmer_db_close()
{
    int rc;
    rc = sqlite3_close(dimmer_db);
    if (rc != SQLITE_OK)
        printf("dimmer db close failed!!!, %d\n", rc);
}


int if_dimmer_exist(UCHAR dev_addr){
	int rc;
	char sql[100];
	sqlite3_stmt *stmt = NULL;

    sprintf(sql, "select * from %s where dev=%d", DIMMER_TABLE_NAME, dev_addr);
    sqlite3_prepare(dimmer_db, sql, -1, &stmt, 0);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
        rc = 1;
    else
        rc = 0;

    sqlite3_finalize(stmt);
    return rc;

}



void remote_table_init(int key){
    int rc;
    char *errmsg;
    char sql[100] = {0};

	int i;
	for(i = 1; i <= key; i++){
		memset(sql, 0, sizeof(sql));
		sprintf(sql, "insert into %s(num,state,value,devaddr,subaddr,sceneid) \
		values(%d,%d,%d,%d,%d,%d)", REMOTE_TABLE_NAME, i, 1, 0, 0, 0, 0);
		if(if_remote_exist(i))
			continue;
		sqlite3_exec(remote_db, sql, NULL, NULL, &errmsg);

	}
	printf("remote table init success!!!\n");

}

void dimmer_table_init(UCHAR dev_addr, int dimmer_cnt){
    int rc;
    char *errmsg;
    char sql[100] = {0};

	int i,j;
	for(i = 0; i <dimmer_cnt; i++){
		
		if(if_dimmer_exist(dev_addr))
			continue;
		for(j = 0;j<8;j++){
			memset(sql, 0, sizeof(sql));
			sprintf(sql, "insert into %s(dev,sub,value) values(%d,%d,%d)", \
			DIMMER_TABLE_NAME, dev_addr, (0x31+j), 0);

			sqlite3_exec(dimmer_db, sql, NULL, NULL, &errmsg);

		}
		dev_addr++;

	}
	printf("dimmer table init success!!!\n");

}

//读取状态和值
void remote_table_read(UCHAR num, UCHAR *data) 
{
    int rc;
    char sql[100];
    sqlite3_stmt *stmt = NULL;

    sprintf(sql, "select * from %s where num=%d", REMOTE_TABLE_NAME, num);
    sqlite3_prepare(remote_db, sql, -1, &stmt, 0);
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        int i = 0;
        for (i = 0; i < 5; i++) 
            data[i+1] = sqlite3_column_int(stmt, 1+i);
    }

    sqlite3_finalize(stmt);
}

void dimmer_table_read(UCHAR *value, UCHAR dev_addr){
    int rc,i = 0;
    char sql[100];
    sqlite3_stmt *stmt = NULL;

    sprintf(sql, "select * from %s where dev=%d", DIMMER_TABLE_NAME, dev_addr);
    sqlite3_prepare(dimmer_db, sql, -1, &stmt, 0);
	
	while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
		value[i] = sqlite3_column_int(stmt, 2);
		i++;
	}
}

void remote_read_num(UCHAR *data, UCHAR *output, UCHAR num)
{
    int rc,i = 0;
    char sql[100];
    sqlite3_stmt *stmt = NULL;
	UCHAR dev_addr,sub_addr,sceneid;
	dev_addr = data[3];
	sub_addr = data[4];
	sceneid = data[5];

    sprintf(sql, "select * from %s where devaddr=%d and subaddr=%d and sceneid=%d",\
		REMOTE_TABLE_NAME, dev_addr, sub_addr, sceneid);
    sqlite3_prepare(remote_db, sql, -1, &stmt, 0);
	
	while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
		output[i] = sqlite3_column_int(stmt, 0);
		if(output[i] == num){
			output[i] = 0x00;
			continue;
		}
		i++;
	}
	
}

void remote_table_update(UCHAR *data){
    int rc;
    char *errmsg;
    char sql[100];
    UCHAR num, dev_addr, sub_addr, scene_id;

	num = data[0];

	dev_addr = data[1];
	sub_addr = data[2];
	scene_id = data[3];

	sprintf(sql, "update %s set devaddr='%d',subaddr='%d',\
		sceneid='%d' where num=%d", REMOTE_TABLE_NAME, dev_addr, sub_addr,\
		scene_id,num);
	rc = sqlite3_exec(remote_db, sql, NULL, NULL, NULL);
	if(rc !=SQLITE_OK)
		printf("FUNC:%s, update failed!\n", __FUNCTION__);
}

void dimmer_db_update(UCHAR dev_addr, UCHAR sub_addr, UCHAR value){
	int rc;
	char *errmsg;
    char sql[100];


	sprintf(sql, "update %s set value='%d' where dev=%d and sub=%d", DIMMER_TABLE_NAME, \
		value, dev_addr, sub_addr);
		
	rc = sqlite3_exec(dimmer_db, sql, NULL, NULL, NULL);
	if(rc !=SQLITE_OK)
		printf("FUNC:%s, update failed!\n", __FUNCTION__);
}


void remote_update_state(UCHAR state, UCHAR num)
{
    int rc;
    char *errmsg;
    char sql[100];
	UCHAR data[6] = {0};

	if(state == 0x02){
		remote_table_read(num, data);
		if(data[1] == 0x00)
			state = 0x01;
		else
			state = 0x00;
	}

	sprintf(sql, "update %s set state='%d' where num=%d", REMOTE_TABLE_NAME,state,num);
	rc = sqlite3_exec(remote_db, sql, NULL, NULL, NULL);
	if(rc !=SQLITE_OK)
		printf("FUNC:%s, update failed!\n", __FUNCTION__);
}

void remote_update_value(UCHAR value, UCHAR num)
{
    int rc;
    char *errmsg;
    char sql[100];

	sprintf(sql, "update %s set value='%d' where num=%d", REMOTE_TABLE_NAME,value,num);
	rc = sqlite3_exec(remote_db, sql, NULL, NULL, NULL);
	if(rc !=SQLITE_OK)
		printf("FUNC:%s, update failed!\n", __FUNCTION__);
}



int if_remote_exist(int num){
	int rc;
	char sql[100];
	sqlite3_stmt *stmt = NULL;

    sprintf(sql, "select * from %s where num=%d", REMOTE_TABLE_NAME, num);
    sqlite3_prepare(remote_db, sql, -1, &stmt, 0);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
        rc = 1;
    else
        rc = 0;

    sqlite3_finalize(stmt);
    return rc;

}

void *remote_init_thread(void *args){

	int dimmer_cnt;
	int pir_cnt;
	UCHAR dev_addr;
	read_dev_num(&dimmer_cnt, &pir_cnt);
	parse_addr_file(&dev_addr);

	int key = pir_cnt*CIRCUIT_MAX;

	remote_db_open();

	remote_table_init(key);

	dimmer_db_open();
	dimmer_table_init(dev_addr,dimmer_cnt);

}

void remote_db_init(){

	//开一个初始化线程
	pthread_t remote_t;

    pthread_create(&remote_t, NULL, remote_init_thread, NULL);
    pthread_detach(remote_t);

}

