#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/time.h>
#include <pthread.h>


#include "addr_req.h"
#include "interface_manage.h"
#include "data_handle.h"
#include "protocol.h"

//读取地址文件
void parse_addr_file(unsigned char *addr)
{
	FILE *fp = fopen(ADDR_PATH, "r");
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

void parse_pir_addr(unsigned char *addr, unsigned char *dimmer_num){
	FILE *fp = fopen(ADDR_PATH, "r");
	unsigned char str[30] = {0};
	unsigned char data[30] = {0};
	int bytes_num_tmp = 0;
	int bytes_num = 0;

	*dimmer_num = 0;
	sprintf(str, "pir_addr");

	while (fgets(data, sizeof(data), fp)){
		
		if (strstr(data, str) != NULL){
			bytes_num = bytes_num_tmp;
			break;
		}
		*dimmer_num +=1;
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
void parse_dimmer_file(unsigned char *dimmer_value, unsigned char dev_addr, unsigned char sub_addr){
	FILE *fp = fopen(DIMMING_PATH, "a+");
	char data[40] = {0};
	char str[40] = {0};
	int dimmer_exist_flag = 0;
	int bytes_num_tmp = 0;
	int bytes_num = 0;

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
		*dimmer_value = 0x64;
	}
	else {
		fclose(fp);
		fopen(DIMMING_PATH, "r");
		fseek(fp,bytes_num, SEEK_SET);
		fscanf(fp, "%*s %s", dimmer_value);//取出文件中保存的调光值
		*dimmer_value = atoi(dimmer_value);
	}
		
	fclose(fp);
}

void bin_to_dec(char *val, unsigned char *data){
	int i = 0;
	*data = 0x00;

	for (i = 0; i < DIAL_NUM; i++){
		*data = (*data)*2 + (val[i] - '0');
	}
	
}

void read_dev_num(int *dimmer_cnt, int *pir_num){
	FILE *fp = fopen(DEV_NUM, "r+");
	char data[30];
	int i = 0;
	char str[2][30];
	while (fgets(data, sizeof(data), fp)){
		
		sscanf(data, "%*s %s", str[i]);
		i++;
	}
	
	*dimmer_cnt = atoi(str[0]);
	*pir_num = atoi(str[1]);
}

void save_to_addr(unsigned char data){

	FILE *fp = fopen(ADDR_PATH, "w");
	int i;
	int PIR_NUM = 0;
	int DIMMER_CNT = 0;
	int ret;

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

//读取拨码盘数据并存入addr.txt
void *read_addr(void *args){
	char dial[DIAL_NUM][BUF_MAX] = {0};
	int i;
	int fd[DIAL_NUM];
	char val[DIAL_NUM] = {0};
	unsigned char data;
	char val_tmp;

	for (i = 0; i < DIAL_NUM; i++){
		sprintf(dial[i], "/dev/dial%d", i);
		
		/*打开文件*/
		fd[i] = open(dial[i], O_RDWR);

		if (fd[i] == -1){
			printf("can not open file %s \n", dial[i]);
		}
		
	}


	/* 读拨码数据 */
	while(1){
		
		for (i = 0; i < DIAL_NUM; i++){
			read(fd[i], &val_tmp, 1);
			val[i] = val_tmp;
		}

		bin_to_dec(val, &data); //将二进制的val转换成十进制

		if (data == 0x00)
			printf("拨码不正确，请重新拨码\n");
		else
			break;

	}

	for (i = 0; i< DIAL_NUM; i++){
		close(fd[i]); 
	}

	/* 将数据存入addr.txt */
	save_to_addr(data);
	
}

//申请地址，保存设备地址（调光灯和红外传感器）
void request_address(void)
{
	//创建线程读取拨码盘数据并保存至addr.txt
	
    pthread_t tid_read;
    pthread_create(&tid_read, NULL, read_addr, NULL); 
    pthread_detach(tid_read);

}



//匹配addr.txt中存储的调光灯设备地址
int match_dimmer_addr(unsigned char dev_addr){
	unsigned char str[30] = {0};
	unsigned char data[30] = {0};
	FILE *fp = fopen(ADDR_PATH, "r");
	
	sprintf(str, "dimmer_addr %d",dev_addr);

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

int match_pir_addr(unsigned char dev_addr){//匹配红外传感器地址（8路继电器）
	unsigned char str[30] = {0};
	unsigned char data[30] = {0};
	FILE *fp = fopen(ADDR_PATH, "r");

	sprintf(str, "pir_addr %d", dev_addr);

	while (fgets(data, sizeof(data), fp)){
		if (strstr(data, str)!=NULL){
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
			sscanf(data, "%*s %s %s %*s %*s %*s %s", \
				dev_addr[para->scene_num], sub_addr[para->scene_num], \
				pir_state[para->scene_num]);
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

	if (match_dimmer_addr(scene_para->dev_addr[0]) == 1){ //若是调光设备
		ret = fprintf(fp, "dimmer %d %d #scene_id %d #value %d #delay %d\n",scene_para->dev_addr[0], \
			scene_para->sub_addr[0], scene_para->scene_id, scene_para->dimmer_value[0], scene_para->time_delay[0]);
		
		if (ret < 0)
		{
			printf("fprintf fail. dimmer %d %d #scene\n",scene_para->dev_addr[0], scene_para->sub_addr[0]);
		}
		
	}
	else if (match_pir_addr(scene_para->dev_addr[0]) == 1){ //若是红外传感器设备
		ret = fprintf(fp, "pir %d %d #scene_id %d #state %d", scene_para->dev_addr[0], \
			scene_para->sub_addr[0], scene_para->scene_id, scene_para->pir_state[0]);

		if (ret < 0)
		{
			printf("fprintf fail. pir %d %d #scene\n",scene_para->dev_addr[0], scene_para->sub_addr[0]);
		}
	}
		
	fclose(fp);

}

void delete_scene_para(SCENE_P *para){
	int ret;
	FILE *fp = fopen(DIMMER_SCENE, "r+");
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


void addr_trans_qy(unsigned char *dev_addr, unsigned char *sub_addr, unsigned char circuit_num){
	FILE *fp = fopen(ADDR_PATH, "r");
	int module_num = 0;

	parse_addr_file(dev_addr);

	module_num = circuit_num/CIRCUIT_MAX;

	*dev_addr = module_num + *dev_addr;
	*sub_addr = circuit_num - module_num*CIRCUIT_MAX + SUB_ADDR_MIN - 0x01;

}

void read_addr_file(DEV_QY *qy_device){
	FILE *fp = fopen(ADDR_PATH, "r");
	char str[30] = {0};
	char type[30] = {0};
	int i;
	int ret;
	
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

