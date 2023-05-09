#include <stdio.h>  
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h>
#include <fcntl.h>


#include "protocol.h"
#include "addr_req.h"
#include "data_handle.h"
#include "interface_manage.h"

void clo_qy_trans(unsigned char *dimmer_value)
{
	int value_tmp;
	float value_tmp_tmp;
	value_tmp_tmp = *dimmer_value;

	value_tmp = (int)(value_tmp_tmp * TRANS_PARA * 10 + 0.5);
	value_tmp_tmp = value_tmp;
	value_tmp = (int)(value_tmp_tmp/10 + 0.5);

	*dimmer_value = value_tmp;
	
}

void qy_clo_trans(unsigned char *dimmer_value){
	int value_tmp;
	float value_tmp_tmp;
	value_tmp_tmp = *dimmer_value;

	value_tmp = (int)(value_tmp_tmp/TRANS_PARA);

	*dimmer_value = value_tmp;
}

//将clowire设备地址转化成启源回路地址
int addr_trans(unsigned char dev_addr, unsigned char sub_addr)
{
	unsigned char dimmer_addr;
	int circuit_num;
	
	if (sub_addr == 0x34 || sub_addr == 0x32 || sub_addr == 0x36 || sub_addr == 0x38)
		sub_addr = sub_addr - 0x01;
	
	/* 读取addr.txt */
	parse_addr_file(&dimmer_addr);
	circuit_num = (dev_addr - dimmer_addr)*DIMMER + (sub_addr - SUB_ADDR_MIN)/2 + 0x01;

	return circuit_num;
}

int pir_addr_trans(unsigned char dev_addr, unsigned char sub_addr){
	int circuit_num;
	unsigned char pir_addr;
	parse_pir_addr(&pir_addr);

	circuit_num = (dev_addr - pir_addr)*CIRCUIT_MAX + (sub_addr -SUB_PIR_ADDR )+ 0x01;
		 
	return circuit_num;
}

//将接受到的clowire网关数据转换为qiyuan控制指令
void single_control_trans(unsigned char *data)
{
	
	unsigned char dimmer_value = data[7];
	unsigned char data_send[BUF_MAX] = {0};
	unsigned char sub_addr = data[2];
	unsigned char dev_addr = data[1];
	
	printf("single_control\n");


	data_send[0] = 0x00;
	data_send[1] = 0x04;
	data_send[2] = 0x0B;
	data_send[3] = 0x00;
	data_send[4] = 0x04;
	data_send[5] = 0x3A;
	data_send[8] = 0x00; //时间

	if (sub_addr == 0x34 || sub_addr == 0x32 || sub_addr == 0x36 || sub_addr == 0x38)
		data_send[5] = 0x3B;
	
	/*设备地址转换*/
	data_send[6] = addr_trans(dev_addr,sub_addr);

	

	//调光灯调光值调整
	if (dimmer_value >= 0x00 && dimmer_value <= 0x64)
	{
	
		//保存调光值和设备地址到photometric.txt中
		if (dimmer_value != 0x00)
			save_dimmer_value(dimmer_value, dev_addr, sub_addr);

		dimmer_db_update(dev_addr, sub_addr, dimmer_value);
		
		clo_qy_trans(&dimmer_value);//转换为启源协议中的调光值
		data_send[7] = dimmer_value;
				
	}
	else if(dimmer_value == 0x9A){ //关闭
		data_send[7] = 0x00;
		dimmer_db_update(dev_addr, sub_addr, 0x00);
	}
	else if(dimmer_value == 0x90){ //按照调光值打开调光灯
		//读取photometric.txt文件，读取其中的调光值
		dimmer_value = parse_dimmer_file(dev_addr, sub_addr);
		dimmer_db_update(dev_addr, sub_addr, dimmer_value);
	
		clo_qy_trans(&dimmer_value);
	
		data_send[7] = dimmer_value;
		
	}

	memcpy(data, data_send, QY_DATA_LEN);

}

//不需要发送给启源
void pir_control(unsigned char *data){
	UCHAR pir_state = data[7];
	UCHAR dev_addr = data[1];
	UCHAR sub_addr = data[2];
	UCHAR cir_num;

	if(pir_state == 0x00)//发过来一个关的指令相当于打开红外传感器
		pir_state = 0x01; 
	else if(pir_state == 0x01)//发过来一个开的指令相当于屏蔽红外传感器
		pir_state = 0x00; 
		
	//将传感器地址转换成启源协议中的组地址
	cir_num = pir_addr_trans(dev_addr,sub_addr);

	//保存传感器功能到remote.db
	save_pir_func(cir_num, pir_state);
	
}


//场景配置，不需要向启源发送数据(包括调光灯和红外传感器配置)
void scene_config_trans(unsigned char *data){
	SCENE_P scene_para;
	unsigned char data_send[50] = {0};
	int len;
	
	//保存发过来的调光灯设备地址、调光值、场景号、延时时间
	scene_para.dev_addr[0] = data[1];
	scene_para.sub_addr[0] = data[2];
	scene_para.scene_id = data[7];

	if (match_addr(scene_para.dev_addr[0],"dimmer_addr") == 1){ //若是调光灯设备
		scene_para.dimmer_value[0] = data[8];//调光值
		scene_para.time_delay[0] = data[9];

		if (scene_para.dimmer_value[0] != 0x00){
			save_dimmer_value(scene_para.dimmer_value[0], scene_para.dev_addr[0], \
			scene_para.sub_addr[0]);

		}
	}
	else {//若是红外传感器设备(先将状态保存到scene.txt)
		scene_para.pir_state[0] = data[8]; //继电器状态
		scene_para.time_delay[0] = data[9];
	}

	save_scene_para(&scene_para);

	//反馈到clowire串口,场景配置反馈
	data_send[0] = 0xFA;
	data_send[1] = 0x00;
	data_send[2] = 0x00;
	data_send[3] = scene_para.dev_addr[0];
	data_send[4] = scene_para.sub_addr[0];
	data_send[5] = 0x09;
	data_send[6] = 0x0A;
	data_send[7] = 0x01;
	len = data_send[5];
	
	data_send[8] = XOR_check(data_send, len-1);

	memcpy(data, data_send, len);
	
}


//需要配置发送给启源的数据
void scene_control_trans(unsigned char *data, unsigned char qy_data[][BUF_MAX], SCENE_P *para){
	int i;
	UCHAR cir_num;
	UCHAR pir_state;
	
	para->scene_id = data[7];
	
	//读取scene.txt文件，匹配场景号
	match_dimmer_scene(para);
	

	if (para->scene_num != 0){ //若文件中保存了该场景号
	
		for (i = 0; i < para->scene_num; i++){

			if (match_addr(para->dev_addr[i],"dimmer_addr") == 1){ //若是调光灯设备
				//调光值的转换
				dimmer_db_update(para->dev_addr[i], para->sub_addr[i],para->dimmer_value[i]);

				clo_qy_trans(&para->dimmer_value[i]);
				qy_data[i][7] = para->dimmer_value[i];
				//地址的转换
				qy_data[i][6] = addr_trans(para->dev_addr[i],para->sub_addr[i]);
				qy_data[i][5] = 0x3A;
				//判断是色温灯还是调光灯
				if (para->sub_addr[i] == 0x34 || para->sub_addr[i] == 0x32 || \
					para->sub_addr[i] == 0x36 || para->sub_addr[i] == 0x38)
					
					qy_data[i][5] = 0x3B;
			}
			else if(match_addr(para->dev_addr[i],"pir_addr") == 1) {//若是红外传感器

				cir_num = pir_addr_trans(para->dev_addr[i], para->sub_addr[i]);
				if (para->pir_state[i] == 0x01){
					pir_state = 0x00;
				}
				else if (para->pir_state[i] == 0x00){
					pir_state == 0x01;
				}
				save_pir_func(cir_num, pir_state);		
				continue;
			}
			
			qy_data[i][0] = 0x00;
			qy_data[i][1] = 0x04;
			qy_data[i][2] = 0x0B;
			qy_data[i][3] = 0x00;
			qy_data[i][4] = 0x04;
			

			qy_data[i][8] = para->time_delay[i];
				
			
		}

	}

}

//场景配置删除
void scene_delete_trans(unsigned char *data){
	SCENE_P scene_para;
	unsigned char data_send[50] = {0};
	int len;

	scene_para.scene_id = data[7];
	scene_para.dev_addr[0] = data[1];
	scene_para.sub_addr[0] = data[2];

	//删除文件中保存的有关该场景的设备信息
	delete_scene_para(&scene_para);

	//配置场景删除反馈
	data_send[0] = 0xFA;
	data_send[1] = 0x00;
	data_send[2] = 0x00;
	data_send[3] = scene_para.dev_addr[0];
	data_send[4] = scene_para.sub_addr[0];
	data_send[5] = 0x0a;
	data_send[6] = 0x0c;
	data_send[7] = scene_para.scene_id;
	data_send[8] = 0x01;
	len = data_send[5];
	
	data_send[9] = XOR_check(data_send, len-1);

	memcpy(data, data_send, len);
	
}

//触点输入配置
void delay_config_inf(UCHAR *data){
	UCHAR dev_addr = data[1];
	UCHAR sub_addr = data[2];

	CONF_PIR delay_conf;

	sub_addr -=0x40;
	if(!match_addr(dev_addr, "pir_addr"))
		return;

	//初始化delay_conf 结构体
	delay_conf.pir_value = data[7];

	delay_conf.dev_addr = data[8];
	delay_conf.sub_addr = data[9];
	

	//将结构体里的信息保存至remote.db
		
	if(delay_conf.pir_value == 0x24) //删除配置信息
		delete_delay_conf(dev_addr,sub_addr);
	else{
		save_delay_conf(delay_conf, dev_addr, sub_addr);
	}
	

}

//启源反馈 调光灯反馈和红外反馈
void dimmer_feedback(unsigned char *data){
	unsigned char dev_addr;
	unsigned char sub_addr;
	unsigned char data_send[BUF_MAX];
	unsigned char app = data[5];
	unsigned char cmd = data[2];
	unsigned char circuit_num = data[6];
	unsigned char dimmer_value;
	int len;

	if (cmd == 0x0B)
		dimmer_value = data[7];
	else if (cmd == 0x5B)
		dimmer_value = data[8];
	
	//地址转换 读取addr.txt文件
	addr_trans_qy(&dev_addr, &sub_addr, circuit_num);

	data_send[0] = 0xFA;
	data_send[1] = 0xFF;
	data_send[2] = 0xFF;
	data_send[3] = dev_addr;
	
	if (app == 0x3A)
		data_send[4] = sub_addr;
	else if (app == 0x3B)
		data_send[4] = sub_addr + 0x01;
	
	data_send[5] = 0x09;
	data_send[6] = 0x03;

	len = data_send[5];
	
	//调光值转换
	if (app == 0x3A || app == 0x3B){
		qy_clo_trans(&dimmer_value);
		data_send[7] = dimmer_value;
	}
	else if(app == 0x38){
		if(dimmer_value == 0xFF)
			data_send[7] = 0x01;
		else if (dimmer_value == 0x00)
			data_send[7] = 0x00;
	}
		
	data_send[8] = XOR_check(data_send, len-1);
	
	memcpy(data, data_send, len);
}



void config_data_innet(DEV_QY *qy_device){
	int len;

	read_addr_file(qy_device);
	
	qy_device->data[0] = 0xFA;
	qy_device->data[1] = 0x00;
	qy_device->data[2] = 0x00;
	qy_device->data[3] = qy_device->dev_addr;
	qy_device->data[4] = 0x00;
	qy_device->data[5] = 0x0b;
	qy_device->data[6] = 0x19;
	qy_device->data[7] = 0x00;
	qy_device->data[8] = 0x00;
	qy_device->data[9] = qy_device->dev_type;

	len = 0x0b;
	
	qy_device->data[10] = XOR_check(qy_device->data, len-1);
		
}

void led_off(void){
	int fd;
	char led_status = 1;

	fd = open("/dev/inetled0", O_RDWR);
	if(fd == -1){
		printf("can not open file innetled\n");
		return;
	}

	write(fd, &led_status, 1);//熄灭灯

	close(fd);
}


void send_innet_cmd(INTERFACE *dev,unsigned char *complite_data){
	//接收到网关发送的入网反馈指令
	DEV_QY qy_device;
	qy_device.dev = dev;
	int qy_dev_num = 0;
	int dimmer_cnt = 0;
	int pir_cnt = 0; 
	int i,len;

	read_dev_num(&dimmer_cnt, &pir_cnt);

	qy_dev_num = dimmer_cnt + pir_cnt; //计算启源总的设备数目

	if (complite_data[9] == 0x01){//入网成功，发送下一个设备的入网指令
	
		qy_device.dev_seq = match_innet_dev(complite_data);
		if (qy_device.dev_seq > qy_dev_num)		
			led_off();
		else{
			printf("\n (* n *)>>>--------------入网下一组模块--------------\n");
			config_data_innet(&qy_device);
			sleep(1);
			len = qy_device.data[5];
			
			printf("\n-------(* 'ω')>︻╦╤─❇---- send data to clowire---------\n");						   
			for (i = 0; i < len; i++)												 
				printf("%02x ", qy_device.data[i]);													  
			printf("\n-----------------------------------------------------\n");
			cbus_enqueue(&qy_device.dev->queue_write, qy_device.data);
		}
	}
	else if(complite_data[9] == 0x00){
		printf("qiyuan device innet failed! tuya's problem\n");
		
	}

}


void singlcontrl_feedback(UCHAR *data)
{
	UCHAR dev_addr = data[1];
	UCHAR sub_addr = data[2];
	UCHAR value = data[7];
	UCHAR dim_value;
	UCHAR data_send[BUF_MAX] = {0};
	int dimmer_flag = 0;
	int i,len;

	dimmer_flag = match_addr(dev_addr, "dimmer_addr");


	data_send[0] = 0xfa;
	data_send[1] = 0xff;
	data_send[2] = 0xff;
	data_send[3] = dev_addr;
	data_send[4] = sub_addr;
	data_send[5] = 0x09;
	data_send[6] = 0x03;

	len = data_send[5];
	
	if (dimmer_flag&&value != 0x9a){
		//调光灯反馈要反馈调光当前值
		dim_value = parse_dimmer_file(dev_addr, sub_addr);
		data_send[7] = dim_value;
	}
	else if (value == 0x9a)
		data_send[7] = 0x00;
	else {
		data_send[7] = data[7];
	}

	data_send[8] = XOR_check(data_send, len-1);
	

	memcpy(data, data_send, len);
	
}


void delay_config_feedback(INTERFACE *dev, UCHAR *data){
	UCHAR data_send[BUF_MAX] = {0};
	
	data_send[0] = 0xfa;
	data_send[3] = data[1];
	data_send[4] = data[2];
	data_send[1] = 0x00;
	data_send[2] = 0x00;
	data_send[5] = 0x09;
	data_send[6] = 0x12;
	data_send[7] = 0x01;

	data_send[8] = XOR_check(data_send, data_send[5]);
	
	cbus_enqueue(&dev->queue_write, data_send);
}

void scene_control_feedback(INTERFACE *dev, UCHAR scene_id){

	SCENE_P para;
	BACK_P callback_p;
	int i,j,module_num,len;
	UCHAR data_send[BUF_MAX][BUF_MAX] = {0};
	

	para.scene_id = scene_id;


	match_dimmer_scene(&para);

	module_num = caculate_scene_module(para);

	callback_p.module_num = module_num;
	
	memset(callback_p.pir_state, 0, BUF_MAX);
	

	get_callback_para(&callback_p, para);

	for (i = 0; i < callback_p.module_num; i++){ //调光模块的整体反馈
		data_send[i][0] = 0xFA;
		data_send[i][1] = 0xFF;
		data_send[i][2] = 0xFF;
		data_send[i][3] = callback_p.dev_addr[i];
		data_send[i][4] = 0x00;
		data_send[i][5] = 0x10;
		data_send[i][6] = 0x04;
		
		
		if (match_addr(callback_p.dev_addr[i],"pir_addr")){
			continue;
		}
		else {
			len = data_send[i][5];
			for (j = 0; j < 8; j++)
				data_send[i][7+j] = callback_p.dimmer_value[i][j];
			
			data_send[i][15] = XOR_check(data_send[i], len);
		}

		usleep(50000);
		
		printf("\n-------(* 'ω')>︻╦╤─❇---- scene control feedback to clowire ---------\n");						   
		for (j = 0; j < len; j++)												 
			printf("%02x ", data_send[i][j]);													  
		printf("\n------------------------------------------------------\n");

		cbus_enqueue(&dev->queue_write, data_send[i]);
		
	}	
	//红外传感器的单独反馈
	for(i = 0; i<para.scene_num;i++){
		if(match_addr(para.dev_addr[i],"dimmer_addr"))
			continue;
		memset(data_send[i], 0, BUF_MAX);
		
		data_send[i][0] = 0xfa;
		data_send[i][1] = 0xff;
		data_send[i][2] = 0xff;
		
		data_send[i][3] = para.dev_addr[i];
		data_send[i][4] = para.sub_addr[i];
		data_send[i][5] = 0x09;
		data_send[i][6] = 0x03;
		data_send[i][7] = para.pir_state[i];

		len = data_send[i][5];

		data_send[i][8] = XOR_check(data_send[i], len);

		printf("\n-------(* 'ω')>︻╦╤─❇---- scene control feedback to clowire ---------\n");						   
		for (j = 0; j < len; j++)												 
			printf("%02x ", data_send[i][j]);													  
		printf("\n------------------------------------------------------\n");
		
		cbus_enqueue(&dev->queue_write, data_send[i]);

	}
	
}

void feed_back(UCHAR *data){
	INTERFACE *dev = NULL;
	char name[20] = {0};
	UCHAR data_send[BUF_MAX] = {0};
	UCHAR dev_addr;
	UCHAR sub_addr;
	UCHAR circuit_num;
	UCHAR dim_value;
	int len;
	
	sprintf(name, "/dev/ttyS1");
	circuit_num = data[6];
	
	dev = find_link_by_name(name);
	if (!dev)
		printf("\n (* n *)>>>------------get clowire port failed------------ \n");

	addr_trans_qy(&dev_addr, &sub_addr, circuit_num);
	dim_value = data[7];
	
	if (data[5] == 0x3b)
	{
		sub_addr +=0x01;
	}
	data_send[0] = 0xFA;
	data_send[1] = 0xFF;
	data_send[2] = 0xFF;
	data_send[3] = dev_addr;
	data_send[4] = sub_addr;
	data_send[5] = 0x09;
	data_send[6] = 0x03;
	if (match_addr(dev_addr, "dimmer_addr")){
		qy_clo_trans(&dim_value);
		data_send[7] = dim_value;
	}
	else {
		if (dim_value == 0x00)
			data_send[7] = 0x00;
		else
			data_send[7] = 0x01;
	}

	len = data_send[5];

	data_send[8] = XOR_check(data_send, len);	

	cbus_enqueue(&dev->queue_write, data_send);
}

void turn_off_dimmer(CONF_PIR pir_conf){
	UCHAR data_send[BUF_MAX][BUF_MAX] = {0};
	SCENE_P para;
	int i;
	UCHAR name[30] = {0};
	INTERFACE *dev_other;
	
	sprintf(name,"/dev/ttyS1");
	dev_other = find_link_by_name(name);
	
	if(pir_conf.scene_id != 0x00){
		para.scene_id = pir_conf.scene_id;
		match_dimmer_scene(&para);
		

		for(i=0;i<para.scene_num;i++){
			data_send[i][0] = 0x00;
			data_send[i][1] = 0x04;
			data_send[i][2] = 0x0b;
			data_send[i][3] = 0x00;
			data_send[i][4] = 0x04;
			data_send[i][5] = 0x3a;

			if (para.sub_addr[i] == 0x34 || para.sub_addr[i] == 0x32 || \
				para.sub_addr[i] == 0x36 || para.sub_addr[i] == 0x38)
				
				data_send[i][5] = 0x3b;

			data_send[i][6] = addr_trans(para.dev_addr[i], para.sub_addr[i]);

			data_send[i][7] = 0x00;
			data_send[i][8] = 0x00;
			dimmer_db_update(para.dev_addr[i], para.sub_addr[i], 0x00);
		}

		qy_enqueue(&pir_conf.dev->queue_write, data_send, para.scene_num);

		//反馈
		scene_control_feedback(dev_other, pir_conf.scene_id);
		
	}
	else{ //如果是普通设备
	
		data_send[0][1] = pir_conf.dev_addr;
		data_send[0][2] = pir_conf.sub_addr;
		data_send[0][7] = 0x9a;
		if(match_addr(pir_conf.dev_addr,"dimmer_addr")){
			data_send[1][1] = pir_conf.dev_addr;
			data_send[1][2] = pir_conf.sub_addr;
			data_send[1][7] = 0x9a;

			single_control_trans(data_send[0]);
			qy_enqueue(&pir_conf.dev->queue_write, data_send, 1);
			
			//反馈
			singlcontrl_feedback(data_send[1]);
			cbus_enqueue(&dev_other->queue_write, data_send[1]);
			
		}
		else { //不是启源的调光灯
			data_send[0][0] = 0xFA;
			data_send[0][3] = 0x00;
			data_send[0][4] = 0x00;
			data_send[0][5] = 0x09;
			data_send[0][6] = 0x01;
			data_send[0][8] = XOR_check(data_send, data_send[5]);
			
			cbus_enqueue(&dev_other->queue_write, data_send);
		}
	}

}


void send_cmd(CONF_PIR pir_inf){
	UCHAR data_send[BUF_MAX] = {0};
	UCHAR name[30] = {0};
	UCHAR qy_data[BUF_MAX][BUF_MAX] = {0};
	INTERFACE *dev_other;

	if(pir_inf.scene_id == 0x00 && pir_inf.dev_addr == 0x00)
		return;
	
	
	sprintf(name,"/dev/ttyS1");
	dev_other = find_link_by_name(name);

	if(pir_inf.pir_value == 0xFF){ //触发触点保存的场景或设备
		if(pir_inf.scene_id != 0x00){
			data_send[0] = 0xFA;
			data_send[1] = 0xFF;
			data_send[2] = 0xFF;
			data_send[3] = 0x00;
			data_send[4] = 0x00;
			data_send[5] = 0x09;
			data_send[6] = 0x0d;
			data_send[7] = pir_inf.scene_id;
			data_send[8] = XOR_check(data_send, data_send[5]);
			cbus_enqueue(&dev_other->queue_read, data_send);
			cbus_enqueue(&dev_other->queue_write, data_send);
		}
		else{
			
			data_send[1] = pir_inf.dev_addr;
			data_send[2] = pir_inf.sub_addr;
			data_send[7] = 0x90;

			if(match_addr(pir_inf.dev_addr, "dimmer_addr")){//如果是启源调光

				single_control_trans(data_send);
				memcpy(qy_data[0], data_send, (data_send[4]+5));

				qy_enqueue(&pir_inf.dev->queue_write, qy_data, 1);

				//反馈
				memset(data_send, 0, BUF_MAX);
				data_send[1] = pir_inf.dev_addr;
				data_send[2] = pir_inf.sub_addr;
				data_send[7] = 0x90;
				singlcontrl_feedback(data_send);
				cbus_enqueue(&dev_other->queue_write, data_send);

			}
			else{
				data_send[0] = 0xFA;
				data_send[3] = 0x00;
				data_send[4] = 0x00;
				data_send[5] = 0x09;
				data_send[6] = 0x01;
				data_send[8] = XOR_check(data_send, data_send[5]);

				cbus_enqueue(&dev_other->queue_write, data_send);

			}
		}

	}
	else if(pir_inf.pir_value == 0x00){ //将场景里的设备关掉

		turn_off_dimmer(pir_inf);

	}

}


void event_handler(INTERFACE *dev, UCHAR *complite_data)
{
	int i,len;
	UCHAR cmd = complite_data[6]; 
	UCHAR dev_addr = complite_data[1];
	UCHAR data_qy[DIMMER_NUM][BUF_MAX] = {0};
	UCHAR scene_id;
	SCENE_P scene_para;
	int dimmer_addr_flag = 0;
	int pir_addr_flag = 0;
	
	dimmer_addr_flag = match_addr(dev_addr,"dimmer_addr");
	pir_addr_flag = match_addr(dev_addr,"pir_addr");

	scene_para.scene_num = 1;
	memset(scene_para.time_delay, 0, BUF_MAX);

	switch(cmd){
	case IN_NET_CALLBACK:
		printf("innet callback from clowire\n");
		if(!dimmer_addr_flag && !pir_addr_flag)
			return;
		
		printf("innet callback from clowire\n");
		send_innet_cmd(dev, complite_data);
		break;
	
	case SINGLE_CONTROL: //单独控制反馈
		if(!dimmer_addr_flag && !pir_addr_flag)
			return; 
		
		len = complite_data[5];
		printf("feedback command to clowire\n");
		singlcontrl_feedback(complite_data);
		sleep(1);
		cbus_enqueue(&dev->queue_write, complite_data);
	
		printf("\n-------(* 'ω')>︻╦╤─❇---- single control feedback to clowire ---------\n");						   
		for (i = 0; i < len; i++)												 
			printf("%02x ", complite_data[i]);													  
		printf("\n------------------------------------------------------\n");
		
		break;

	case SCENE_CONTROL:
		scene_id = complite_data[7];
		scene_control_trans(complite_data, data_qy, &scene_para);
	
		//将数据放入启源串口的写缓冲区
		char name[20] = {0};
		INTERFACE *dev_other;
		
		
		sprintf(name, "/dev/ttyS10");
		dev_other = find_link_by_name(name);
		qy_enqueue(&dev_other->queue_write, data_qy, scene_para.scene_num);
		
		printf("scene control feedback to clowire\n");
		
		scene_control_feedback(dev, scene_id);
		printf("scene control feedback done\n");
		break;
	
	case DELAY_CONFIG:
		printf("delay config feedback to clowire\n");

		delay_config_feedback(dev, complite_data);

	}
	
}




void pir_handler(INTERFACE *dev, UCHAR *complite_data){
	UCHAR app = complite_data[5];
	UCHAR num = complite_data[6];
	UCHAR cmd = complite_data[2];

	int state;
	UCHAR pir_value; 
	CONF_PIR pir_conf;
	
	if (app != 0x3F || cmd == 0x5B)
		return;

	
	//读取remote.db文件中保存的值
	state = read_by_num(num);
	if(state == 0)//若红外传感器被屏蔽
		return;

	pir_value = complite_data[7];

	//初始化结构体pir_conf
	pir_conf.dev_addr = 0x00;
	pir_conf.sub_addr = 0x00;
	pir_conf.scene_id = 0x00;
	memset(pir_conf.nums,0,BUF_MAX);
	pir_conf.dev = dev;
	pir_conf.pir_value = pir_value;

	//读取remote.db中保存的值
	read_pir_scene(&pir_conf,num);

	
	if(pir_conf.nums[0] == 0x00)//不属于组
	{
		
		send_cmd(pir_conf);
	}
	else 
	{
		
		if(pir_value == 0x00){
		//读取remote.db中同组的其他传感器状态
			if(read_pir_value(pir_conf.nums))//若有一个是1则返回0,(没有检测到人)返回1
				send_cmd(pir_conf);
		}
		else if(pir_value == 0xFF)
			send_cmd(pir_conf);
	}

	if(pir_value == 0xFF){
		pir_value = 0x01;
	}

	//保存回路地址和状态到remote.db(有初始化默认值)
	save_pir_value(num, pir_value);
	
}

void get_handle(void){
	INTERFACE *dev = NULL;
	char name[20] = {0};
	unsigned char data[BUF_MAX] = {0};
	int i;

	sprintf(name, "/dev/ttyS1");
	dev = find_link_by_name(name);
	if (!dev)
		printf("\n (* n *)>>>------------wait to get clowire port------------ \n");

	while(1){
		//循环获取该串口的读缓冲区函数
		if(cbus_dequeue(&dev->queue_read, data) < 0)
			break;

		event_handler(dev, data);
	}

	sprintf(name,"/dev/ttyS10");
	dev = find_link_by_name(name);
	if(!dev)
		printf("\n (* n *)>>>------------failed to get qy port------------ \n");

	while(1){
		if(qy_dequeue(&dev->queue_read, data) < 0)
			break;
		
		pir_handler(dev,data);
	}

}

