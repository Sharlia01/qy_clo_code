#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/time.h>
#include <pthread.h>
#include <fcntl.h>


#include "addr_req.h"
#include "interface_manage.h"
#include "data_handle.h"
#include "protocol.h"

//读取地址文件
void parse_addr_file(unsigned char *addr)
{
	FILE *fp = NULL;
	int ret;
	while(1){
		fp = fopen(ADDR_PATH,"r");
		if (!fp)
			ret = -1;
		else
			break;
	}
	unsigned char str[30] = {0};
	unsigned char data[30] = {0};
	int bytes_num_tmp = 0;
	int bytes_num = 0;

	
	sprintf(str, "dimmer_addr");

	while (fgets(data, sizeof(data), fp)){
		
		if (strstr(data, str) != NULL){
			bytes_num = bytes_num_tmp;
			break;
		}
		bytes_num_tmp +=strlen(data);
		memset(data, 0, sizeof(data));
	}

	fseek(fp, bytes_num, SEEK_SET);
	fscanf(fp, "%*s %s", addr);
	*addr = atoi(addr);
	fclose(fp);
}


void save_pir_func(UCHAR cir_num, UCHAR pir_sta){

	remote_update_state(pir_sta, cir_num);
}


void parse_pir_addr(unsigned char *addr){
	FILE *fp = fopen(ADDR_PATH, "r");
	unsigned char str[30] = {0};
	unsigned char data[30] = {0};
	int bytes_num_tmp = 0;
	int bytes_num = 0;

	
	sprintf(str, "pir_addr");

	while (fgets(data, sizeof(data), fp)){
		
		if (strstr(data, str) != NULL){
			bytes_num = bytes_num_tmp;
			break;
		}
		
		bytes_num_tmp +=strlen(data);
		memset(data, 0, sizeof(data));
	}

	fseek(fp, bytes_num, SEEK_SET);
	fscanf(fp, "%*s %s", addr);
	*addr = atoi(addr);
	fclose(fp);
}

//保存调光值和设备地址到photometric.txt中
void save_dimmer_value(unsigned char dimmer_value, unsigned char dev_addr, unsigned char sub_addr)
{
	int value = dimmer_value; //转换成十进制
	int dimmer_exist_flag = 0;
	FILE *fp = fopen(DIMMING_PATH, "a+");
	char str[30] = {0};
	char data[50] = {0};

	sprintf(str, "dimmer_%d_%d", dev_addr, sub_addr);

	while (fgets(data, sizeof(data), fp)) 
	{
        if (strstr(data, str) != NULL) {
				dimmer_exist_flag = 1;
        	}
		memset(data, 0, sizeof(data));
	}
	
	int ret = 0;
	if (!dimmer_exist_flag) {//如果文件中没有存储该回路的调光值
		//将设备地址，子地址、调光值写入文件
		ret = fprintf(fp, "dimmer_%d_%d#value %d\n",dev_addr, sub_addr, value);
		if (ret < 0)
		{
			printf("fprintf fail. dimmer_%d_%d#value %d",dev_addr, sub_addr, value);
		}

	} else {	//如果该回路已存在，追加到回路的最后
		fclose(fp);
		fp = fopen(DIMMING_PATH, "r+");
		FILE *fp_tmp = fopen(DIMMING_PATH_TMP,"a+"); //创建一个新文件进行增删查改工作
        if (!fp_tmp) {
            printf("add_dimmer_file. Fail to open [tmp] file!!!\n");
            dimmer_exist_flag = 0;
        }

		while (fgets(data,sizeof(data),fp))
		{
            if (strstr(data, str) != NULL) {
				ret = fprintf(fp_tmp, "dimmer_%d_%d#value %d\n",dev_addr, sub_addr, value);
				if (ret < 0)
				{
					printf("fprintf fail. dimmer_%d_%d#value %d",dev_addr, sub_addr, value);
				}
				continue;
					
            }
			
			ret = fputs(data,fp_tmp);
			if (ret < 0){
                printf("add_dimmer_file. fputs fail. %s", data);
			}
            memset(data, sizeof(data), 0);
		}

		fclose(fp);
		fclose(fp_tmp);
		
        fp = fopen(DIMMING_PATH, "w");
        fp_tmp = fopen(DIMMING_PATH_TMP, "r");
		
        while (fgets(data, sizeof(data), fp_tmp)) {
            ret = fputs(data, fp);
            if (ret < 0) {
                printf("add_dimmer_file. fputs fail. %s", data);
            }
            memset(data, sizeof(data), 0);
        }
        fclose(fp_tmp);
        remove(DIMMING_PATH_TMP);
		
	}

	fclose(fp);
	
}


//读取photometric.txt文件，获取其中的调光值
int parse_dimmer_file(unsigned char dev_addr, unsigned char sub_addr)
{
	FILE *fp = fopen(DIMMING_PATH, "a+");
	char data[40] = {0};
	char str[40] = {0};
	int dimmer_exist_flag = 0;
	int bytes_num_tmp = 0;
	int bytes_num = 0;
	UCHAR dimmer_value;
	int ret;

	//如果txt文件中没有保存调光值则默认设置为最大值
	sprintf(str, "dimmer_%d_%d", dev_addr, sub_addr);

	while (fgets(data, sizeof(data), fp)) {
		if (strstr(data, str) != NULL){
			dimmer_exist_flag = 1; 
			bytes_num = bytes_num_tmp;
			}
		bytes_num_tmp += strlen(data);
		memset(data, 0, sizeof(data));
	}

	if (!dimmer_exist_flag){//没有保存调光值
		ret = 100;
	}
	else { //文件中保存了调光值
		fclose(fp);
		fopen(DIMMING_PATH, "r");
		fseek(fp,bytes_num, SEEK_SET);
		fscanf(fp, "%*s %s", &dimmer_value);//取出文件中保存的调光值
		ret = atoi(&dimmer_value);

	}
		
	fclose(fp);

	return ret;
}

void bin_to_dec(char *val, unsigned char *data){
	int i = 0;
	*data = 0x00;
	
	for (i = 0; i < DIAL_NUM; i++){
		*data = (*data)*2 + val[i];
	}
	
}

void read_dev_num(int *dimmer_cnt, int *pir_num){
	FILE *fp = fopen(DEV_NUM, "r");
	char data[50];
	int i = 0;
	char str[2][50] = {0};
	while (fgets(data, sizeof(data), fp)){
		
		sscanf(data, "%*s %s", str[i]);
		i++;
	}
	*dimmer_cnt = atoi(str[0]);
	*pir_num = atoi(str[1]);
	fclose(fp);
}

int match_addr_flag(UCHAR data){
	UCHAR str[50] = {0};
	UCHAR contents[50] = {0};
	FILE *fp = fopen(ADDR_PATH, "r");

	sprintf(str, "dimmer_addr %d", data);

	while (fgets(contents,sizeof(contents), fp)){
		if (strstr(contents, str) != NULL) {
			fclose(fp);
			return 1;
		}
	}

	return 0;

}

void save_to_addr(unsigned char data){

	FILE *fp = fopen(ADDR_PATH, "w");
	int i;
	int PIR_NUM = 0;
	int DIMMER_CNT = 0;
	int ret;
	UCHAR str[50] = {0};

	read_dev_num(&DIMMER_CNT, &PIR_NUM);
	
	for (i  = 0; i < DIMMER_CNT; i++){ //保存调光灯设备地址
		ret = fprintf(fp, "dimmer_addr %d\n", data);
		if (ret < 0)
		{
			printf("fprintf fail. dimmer_addr\n");
		}
		
		data++;
	}

	for (i = 0; i< PIR_NUM; i++){//保存红外传感器设备地址
		ret = fprintf(fp, "pir_addr %d\n", data);
	
		if (ret < 0)
		{
			printf("fprintf fail. dimmer_addr\n");
		}
		data++;
	}

	fclose(fp);

}

void remove_all_files(void)
{
	if((access(DIMMING_PATH,F_OK))!=-1)//文件存在
		remove(DIMMING_PATH); //删除调光值文件

	
	if((access(DIMMER_SCENE,F_OK))!=-1)//文件存在
		remove(DIMMER_SCENE); //删除场景文件

	if((access(REMOTE_DB_PATH,F_OK))!=-1)
		remove(REMOTE_DB_PATH); //删除红外传感器的db文件

	if((access(DIMMER_DB_PATH,F_OK))!=-1)
		remove(DIMMER_DB_PATH); //删除调光灯db文件
		
}

//读取拨码盘数据并存入addr.txt
void *read_addr(void *args){
	char dial[DIAL_NUM][BUF_MAX] = {0};
	int i;
	int fd[DIAL_NUM];
	char val[DIAL_NUM] = {0};
	unsigned char data = 0x00;
	char val_tmp;
	int flag = 2;

	for (i = 0; i < DIAL_NUM; i++){
		//拨码盘引脚dial0-dial7
		sprintf(dial[i], "/dev/dial%d", i);
		
		/*打开文件*/
		fd[i] = open(dial[i], O_RDWR);
		if (fd[i] == -1){
			printf("can not open file \n");
		}
	}


	/* 读拨码数据 */
	while(1){
		
		for (i = 0; i < DIAL_NUM; i++){
						
			read(fd[i], &val_tmp, 1);
			val[i] = val_tmp;
			val_tmp = 0;
			
		}
		
		bin_to_dec(val, &data); //将二进制的val转换成十进制
		
		if (data > 0xf0)
		{
			printf("\n (* n *)>>>--------------拨码过大 : %02x，请重新拨码--------------\n",data);
			sleep(2);
			continue;
		}
		else{
			printf("\n (* n *)>>>--------------拨码 : %02x，请按键入网--------------\n",data);
		}

		if (access(ADDR_PATH,F_OK) != -1) //如果该文件存在
			flag = match_addr_flag(data);

		save_to_addr(data); //将数据存入addr.txt

		if (flag == 0)
		{
			printf("delete all config files");
			remove_all_files();
		}
		break;
	}

	
}



//申请地址，保存设备地址（调光灯和红外传感器）
void request_address(void)
{
	
    pthread_t tid_read;

	//创建线程读取拨码盘数据并保存至addr.txt
    pthread_create(&tid_read, NULL, read_addr, NULL); 
    pthread_detach(tid_read);

}


//匹配addr.txt中存储的调光灯设备地址
int match_addr(unsigned char dev_addr, char addr_name[30]){
	unsigned char str[30] = {0};
	unsigned char data[30] = {0};
	FILE *fp = fopen(ADDR_PATH, "r");
	
	sprintf(str, "%s %d", addr_name,dev_addr);

	while (fgets(data,sizeof(data), fp)){
		if (strstr(data, str) != NULL) {
			fclose(fp);
			return 1;
		}
		memset(data, 0, sizeof(data));
	}
	fclose(fp);
	return 0;
}

void match_dimmer_scene(SCENE_P *para){
	char str[50] = {0};
	char str_pir[50] = {0};
	char data[50] = {0};
	char dev_addr[20][50] ={0};
	char sub_addr[20][50] ={0};
	char dimmer_value[20][50] ={0};
	char time_delay[20][50] ={0};
	char pir_state[20][50] = {0};
	int scene_match_flag = 0;
	int i;
	
	para->scene_num = 0; //场景里配置的启源灯加红外传感器的数目
	memset(para->dev_addr, 0, BUF_MAX);
	memset(para->sub_addr, 0, BUF_MAX);
	memset(para->dimmer_value, 0, BUF_MAX);
	memset(para->time_delay, 0, BUF_MAX);
	memset(para->pir_state, 0, BUF_MAX);
	
	FILE *fp = fopen(DIMMER_SCENE, "r");

	sprintf(str, "scene_id %d #value", para->scene_id);
	sprintf(str_pir, "scene_id %d #state", para->scene_id);
	
	while(fgets(data, sizeof(data), fp)){
		if(strstr(data, str) !=NULL){
			scene_match_flag = 1;
			sscanf(data, "%*s %s %s %*s %*s %*s %s %*s %s", \
				dev_addr[para->scene_num],sub_addr[para->scene_num], \
				dimmer_value[para->scene_num],time_delay[para->scene_num]);
		
			para->scene_num++;
		}
		else if (strstr(data, str_pir)!=NULL){
			scene_match_flag = 1;
			sscanf(data, "%*s %s %s %*s %*s %*s %s %*s %s", \
				dev_addr[para->scene_num], sub_addr[para->scene_num], \
				pir_state[para->scene_num],time_delay[para->scene_num]);
		
			para->scene_num++;
		}
		memset(data, 0, sizeof(data));
	}

	if (scene_match_flag){

		for(i = 0; i < para->scene_num; i++){
			para->dev_addr[i] = atoi(dev_addr[i]); 
			para->sub_addr[i] = atoi(sub_addr[i]);
			para->dimmer_value[i] = atoi(dimmer_value[i]);
			para->pir_state[i] = atoi(pir_state[i]);
			para->time_delay[i] = atoi(time_delay[i]);
		}
	}
	
	fclose(fp);

}

//保存场景参数至scene.txt
void save_scene_para(SCENE_P *scene_para){
	FILE *fp = fopen(DIMMER_SCENE,"a+");
	int ret;

	if (match_addr(scene_para->dev_addr[0],"dimmer_addr") == 1){ //若是调光设备
		ret = fprintf(fp, "dimmer %d %d #scene_id %d #value %d #delay %d\n",scene_para->dev_addr[0], \
			scene_para->sub_addr[0], scene_para->scene_id, scene_para->dimmer_value[0], scene_para->time_delay[0]);
		
		if (ret < 0)
		{
			printf("fprintf fail. dimmer %d %d #scene\n",scene_para->dev_addr[0], scene_para->sub_addr[0]);
		}
		
	}
	else if (match_addr(scene_para->dev_addr[0], "pir_addr") == 1){ //若是红外传感器设备
		ret = fprintf(fp, "pir %d %d #scene_id %d #state %d #delay %d\n", scene_para->dev_addr[0], \
			scene_para->sub_addr[0], scene_para->scene_id, scene_para->pir_state[0], \
			scene_para->time_delay[0]);

		if (ret < 0)
		{
			printf("fprintf fail. pir %d %d #scene\n",scene_para->dev_addr[0], scene_para->sub_addr[0]);
		}
	}
		
	fclose(fp);
}

void delete_scene_para(SCENE_P *para){
	int ret;
	FILE *fp = fopen(DIMMER_SCENE, "a+");
	FILE *fp_tmp = fopen(DIMMER_SCENE_TMP, "a+");
	char str[50] = {0};
	char data[50] = {0};
	int delete_scene_flag = 0;

	sprintf(str, "%d %d #scene_id %d", para->dev_addr[0], para->sub_addr[0], para->scene_id);

	while (fgets(data, sizeof(data), fp)){
		
		if(strstr(data, str) !=NULL){
			delete_scene_flag = 1;
			continue; //删除这几行有关场景的数据
		}
		
		fputs(data, fp_tmp);
		memset(data, 0, sizeof(data));
	}
	
	fclose(fp);
	fclose(fp_tmp);
	
	if (delete_scene_flag){
		fp = fopen(DIMMER_SCENE,"w");
		fp_tmp = fopen(DIMMER_SCENE_TMP,"r");
		if (!fp || !fp_tmp) {
			printf("add_scene_file. Fail to open [fp or tmp] file!!!\n");
		}

		while (fgets(data,sizeof(data),fp_tmp)){
			ret = fputs(data, fp);
			if (ret < 0) {
				printf("modify_scene_file.fputs fail. %s",data);
			}
			memset(data, sizeof(data), 0);
		}
		fclose(fp_tmp);
		remove(DIMMER_SCENE_TMP);
		
		fclose(fp);
	}
	else
		remove(DIMMER_SCENE_TMP);
	
}

//将启源回路地址转换为clowire设备地址
void addr_trans_qy(unsigned char *dev_addr, unsigned char *sub_addr, unsigned char circuit_num){
	FILE *fp = fopen(ADDR_PATH, "r");
	int module_num = 0;

	parse_addr_file(dev_addr);

	module_num = (circuit_num-0x01)/DIMMER;

	*dev_addr = module_num + *dev_addr;
	//回路地址02代表0x33
	*sub_addr = (circuit_num - module_num*DIMMER- 0x01)*2+ SUB_ADDR_MIN;

}

void pir_addr_clo(UCHAR *dev_addr,UCHAR *sub_addr,UCHAR num)
{
	FILE *fp = fopen(ADDR_PATH,"r");
	int module_num = 0;

	parse_pir_addr(dev_addr);

	module_num = (num-0x01)/CIRCUIT_MAX;

	*dev_addr = module_num + *dev_addr;
	*sub_addr = (num - module_num*CIRCUIT_MAX - 0x01) + SUB_PIR_ADDR;
}


void read_addr_file(DEV_QY *qy_device){
	char str[30] = {0};
	char type[30] = {0};
	int i;
	int ret;
	FILE *fp = NULL;

	while(1){
		fp = fopen(ADDR_PATH,"r");
		if (!fp)
			ret = -1;
		else
			break;
	}
	
	for (i = 0; i < qy_device->dev_seq; i++){
		memset(str, 0, sizeof(str));
		memset(type, 0, sizeof(type));
		ret = fscanf(fp, "%s %s", type, str);

	}
	qy_device->dev_addr = atoi(str);

	if (!strcmp(type, "dimmer_addr")){
		qy_device->dev_type = QY_DIMMER;
	}
	else if(!strcmp(type, "pir_addr"))
		qy_device->dev_type = QY_PIR;	

	fclose(fp);

}

int match_innet_dev(unsigned char *complite_data){
	FILE *fp = fopen(ADDR_PATH, "r");
	char data[30] = {0};
	char str[30] = {0};
	int line_num = 0;

	sprintf(str, " %d", complite_data[1]);

	while (fgets(data, sizeof(data), fp)){
		line_num++;
		if (strstr(data, str) != NULL){
			break;
		}
	}
	fclose(fp);
	
	return line_num+1;

}

int caculate_scene_module(SCENE_P para){
	int i;
	int num = 1;
	FILE *fp = fopen(MODULE_NUM, "w+");
	UCHAR str[50] = {0};
	UCHAR data[50] = {0};
	int flag = 0;

	fprintf(fp, "%d \n", para.dev_addr[0]);

	for (i = 0; i < para.scene_num; i++){ //scene_num = 3;
		
		fseek(fp, 0, SEEK_SET);
		sprintf(str, "%d", para.dev_addr[i]);
		
		while (fgets(data, sizeof(data), fp)){

			if (strstr(data, str)!= NULL){//检索到子串
				flag = 1;
				break;
			}
			else{
				flag = 0;
			}
			memset(data, 0, sizeof(data));
			
		}

		if (!flag){
			fprintf(fp, "%d \n", para.dev_addr[i]);
			num += 1;
		}
			
		
		memset(str, 0, sizeof(str));
	
	}

	fclose(fp);
	return num;

}

void get_callback_para(BACK_P * back_p, SCENE_P para){
	FILE *fp = fopen(MODULE_NUM, "r");
	UCHAR data[50] = {0};
	UCHAR str[50] = {0};
	int i,j,bytes_num = 0;
	int num = 0;
	UCHAR dev_addr;
	
	while(fgets(data, sizeof(data), fp)){ //data is part of fp 

		sscanf(data, "%s",&dev_addr);
		back_p->dev_addr[num] = atoi(&dev_addr);
		memset(back_p->dimmer_value[num],0, BUF_MAX);
		
		dimmer_table_read(back_p->dimmer_value[num], back_p->dev_addr[num]);
		bytes_num += strlen(data);
		memset(data, 0, sizeof(data));
		num++;
	}

}

int read_by_num(UCHAR num){
	UCHAR data[6] = {0};
	remote_table_read(num, data);

	if(data[1] == 0x01)
		return 1;
	else if(data[1] == 0x00)
		return 0;
}


void save_delay_conf(CONF_PIR p_conf, UCHAR dev_addr, UCHAR sub_addr){
	UCHAR data[4] = {0};

	data[0] = pir_addr_trans(dev_addr, sub_addr);

	if(p_conf.dev_addr == 0x00)
		data[3] = p_conf.sub_addr;//表示sceneid
	else {
		data[1] = p_conf.dev_addr;
		data[2] = p_conf.sub_addr;
	}
	remote_table_update(data);
}

void delete_delay_conf(UCHAR dev_addr, UCHAR sub_addr){
	UCHAR data[4] = {0};

	data[0] = pir_addr_trans(dev_addr, sub_addr);

	remote_table_update(data);

}

void read_pir_scene(CONF_PIR *pir_conf, UCHAR num){
	UCHAR data[6] = {0};

	remote_table_read(num, data);

	pir_conf->dev_addr = data[3];
	pir_conf->sub_addr = data[4];
	pir_conf->scene_id = data[5];

	remote_read_num(data, pir_conf->nums,num);

}

int read_pir_value(UCHAR *nums){
	int i = 0,j;
	UCHAR data[6] = {0};
	UCHAR group[BUF_MAX] = {0};
	
	while(nums[i] != 0x00){
		remote_table_read(nums[i], data);
		group[i] = data[2];
		i++;
	}

	for(j = 0; j<i; j++){
		if(group[i] == 0x01)
			return 0;
	}

	return 1;

}

void save_pir_value(UCHAR num, UCHAR pir_value){

	remote_update_value(pir_value, num);
}




