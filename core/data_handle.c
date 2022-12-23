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

int addr_trans(unsigned char *dev_addr, unsigned char *sub_addr)
{
	unsigned char dimmer_addr;
	int circuit_num;

	/* 读取addr.txt */
	parse_addr_file(&dimmer_addr);
	circuit_num = (*dev_addr - dimmer_addr)*CIRCUIT_MAX + (*sub_addr - SUB_ADDR_MIN) + 0x01;

	return circuit_num;
}

int pir_addr_trans(unsigned char dev_addr, unsigned char sub_addr){
	int circuit_num;
	unsigned char pir_addr;
	unsigned char dimmer_num;
	parse_pir_addr(&pir_addr, &dimmer_num);

	circuit_num = (dev_addr - pir_addr)*CIRCUIT_MAX + (sub_addr -SUB_PIR_ADDR ) \
		+ dimmer_num*CIRCUIT_MAX + 0x01;

	return circuit_num;

}

void single_control_trans(unsigned char *data)
{
	
	unsigned char dimmer_value = data[7];
	unsigned char data_send[QY_DATA_LEN] = {0};
	unsigned char sub_addr = data[2];
	unsigned char dev_addr = data[1];

	data_send[0] = 0x3A;
	data_send[1] = 0x00;
	data_send[2] = 0x07;
	data_send[3] = 0x00;
	data_send[4] = 0x83;
	data_send[5] = 0x10;
	data_send[6] = 0x01;
	data_send[10] = 0x00;
	data_send[11] = 0xAA;
	data_send[12] = 0xBB;
	/*设备地址转换*/
	data_send[8] = addr_trans(&dev_addr,&sub_addr);

	//调光灯调光值调整
	if (dimmer_value >= 0x00 && dimmer_value <= 0x64)
	{
		clo_qy_trans(&dimmer_value);//转换为启源协议中的调光值
		data_send[7] = 0x3A;
		data_send[9] = dimmer_value;
		
		//保存调光值和设备地址到photometric.txt中
		save_dimmer_value(dimmer_value, dev_addr, sub_addr);
		
	}
	else if(dimmer_value == 0x9A){
		data_send[7] = 0x39;
		data_send[9] = 0x00;
	}
	else if(dimmer_value == 0x90){
		//读取photometric.txt文件，读取其中的调光值
		parse_dimmer_file(&dimmer_value, dev_addr, sub_addr);
		clo_qy_trans(&dimmer_value);
		data_send[9] = dimmer_value;
		
		if (dimmer_value == 0xff){
			data_send[7] = 0x38;
		}
		else if (dimmer_value == 0x00)
			data_send[7] = 0x39;
		else
			data_send[7] = 0x3A;
	}

	memcpy(data, data_send, strlen(data_send));

}

//配置发送给启源的数据
void pir_control(unsigned char *data){
	unsigned char pir_state;
	unsigned char dev_addr;
	unsigned char sub_addr;
	unsigned char data_send[QY_DATA_LEN] = {0};

	pir_state = data[7];
	dev_addr = data[1];
	sub_addr = data[2];

	data_send[0] = 0x3A;
	data_send[1] = 0x00;
	data_send[2] = 0x07;
	data_send[3] = 0x00;
	data_send[4] = 0x83;
	data_send[5] = 0x10;
	data_send[6] = 0x01;

	if (pir_state == 0x01){
		data_send[7] = 0x38;
		data_send[9] = 0xFF;
	}
	else if(pir_state == 0x00){
		data_send[7] = 0x39;
		data_send[9] = 0x00;
	}
	//将传感器地址转换成启源协议中的组地址
	data_send[8] = pir_addr_trans(dev_addr,sub_addr);
	data_send[10] = 0x00;
	data_send[11] = 0xAA;
	data_send[12] = 0xBB;
	
	memcpy(data, data_send, strlen(data_send));
	
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

	if (match_dimmer_addr(scene_para.dev_addr[0]) == 1){ //若是调光灯设备
		scene_para.dimmer_value[0] = data[8];
		scene_para.time_delay[0] = data[9];
		
		save_dimmer_value(scene_para.dimmer_value[0], scene_para.dev_addr[0], \
		scene_para.sub_addr[0]);
	}
	else {//若是红外传感器设备
		scene_para.pir_state[0] = data[8];
	}

	save_scene_para(&scene_para);

	//反馈到clowire串口
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
	
	para->scene_id = data[7];
	
	//读取dimmer_scene.txt文件，匹配场景号
	match_dimmer_scene(para);

	if (para->scene_num != 0){ //若文件中保存了该场景号
	
		for (i = 0; i < para->scene_num; i++){
			qy_data[i][0] = 0x3A;
			qy_data[i][1] = 0x00;
			qy_data[i][2] = 0x07;
			qy_data[i][3] = 0x00;
			qy_data[i][4] = 0x83;
			qy_data[i][5] = 0x10;
			qy_data[i][6] = 0x01;
			
			qy_data[i][10] = 0x00;
			qy_data[i][11] = 0xAA;
			qy_data[i][12] = 0xBB;

			if (match_dimmer_addr(para->dev_addr[i]) == 1){ //若是调光灯设备
				//调光值的转换
				clo_qy_trans(&para->dimmer_value[i]);
				qy_data[i][9] = para->dimmer_value[i];
				//地址的转换
				qy_data[i][8] = addr_trans(&para->dev_addr[i],&para->sub_addr[i]);

				
				if (qy_data[i][9] == 0x00)
					qy_data[i][7] = 0x39;
				else if (qy_data[i][9] == 0xFF)
					qy_data[i][7] = 0x38;
				else
					qy_data[i][7] = 0x3A;
			}
			else if(match_pir_addr(para->dev_addr[i]) == 1) {//若是红外传感器
				if (para->pir_state[i] == 1){
					qy_data[i][9] = 0xFF;
					qy_data[i][7] = 0x38;
				}
				else if (para->pir_state[i] == 0){
					qy_data[i][9] == 0x00;
					qy_data[i][7] == 0x39;
				}
				qy_data[i][8] = pir_addr_trans(para->dev_addr[i], para->sub_addr[i]);
				
			}
				
			
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
	
	data_send[8] = XOR_check(data_send, len-1);

	memcpy(data, data_send, len);
	
}

//启源反馈
void dimmer_feedback(unsigned char *data){
	unsigned char dev_addr;
	unsigned char sub_addr;
	unsigned char data_send[BUF_MAX];
	unsigned char circuit_num = data[8];
	unsigned char dimmer_value = data[10];
	int len;
	
	//地址转换 读取addr.txt文件
	addr_trans_qy(&dev_addr, &sub_addr, circuit_num);

	data_send[0] = 0xFA;
	data_send[1] = 0xFF;
	data_send[2] = 0xFF;
	data_send[3] = dev_addr;
	data_send[4] = sub_addr;
	data_send[5] = 0x09;
	data_send[6] = 0x03;
	
	//调光值转换
	qy_clo_trans(&dimmer_value);
	data_send[7] = dimmer_value;
	len = data_send[5];

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
	char led_status = 0;

	fd = open("/dev/innetLed", O_RDWR);
	if(fd == -1){
		printf("can not open file innetled\n");
		return;
	}

	write(fd, &led_status, 1);

	close(fd);
}

void send_innet_cmd(INTERFACE * dev,unsigned char *complite_data){
	//接收到网关发送的入网反馈指令
	DEV_QY qy_device;
	qy_device.dev = dev;
	int all_dev_num;
	int dimmer_cnt = 0;
	int pir_cnt = 0; 

	read_dev_num(&dimmer_cnt, &pir_cnt);
	all_dev_num = dimmer_cnt + pir_cnt; //计算启源总的设备数目
	
	if (complite_data[9] == 0x01){//入网成功，发送下一个设备的入网指令
		qy_device.dev_seq = match_innet_dev(complite_data);
		if (qy_device.dev_seq > all_dev_num)
			led_off();
		else{
			config_data_innet(&qy_device);
			cbus_enqueue(qy_device.dev->queue_write, qy_device.data);
		}
	}
	else if(complite_data[9] == 0x00){
		printf("qiyuan device innet failed! tuya's problem\n");
		
	}

}


void event_handler(INTERFACE * dev, unsigned char *complite_data)
{
	int ret = 0;

	unsigned char cmd = complite_data[6]; 

	switch(cmd){
	case IN_NET_CALLBACK:
		printf("innet callback from clowire\n");
		send_innet_cmd(dev, complite_data);
		
	}
	
}

void get_handle(void){
	INTERFACE *dev = NULL;
	char name[20] = {0};
	unsigned char data[BUF_MAX] = {0};
	int ret;

	sprintf(name, CLOWIRE_PORT);
	dev = find_link_by_name(name);
	if (!dev)
		printf("get clowire port failed!\n");

	while(1){
		//循环获取该串口的读缓冲区函数
		if(cbus_dequeue(&dev->queue_read, data) < 0)
			break;

		//处理函数
		event_handler(dev,data);
		
	}

}

