#ifndef __DATA_HANDLE_H__
#define __DATA_HANDLE_H__

#include "interface_manage.h"


void single_control_trans(unsigned char *);
void scene_config_trans(unsigned char *);



typedef struct scene_p {
    unsigned char scene_id;
    unsigned char dev_addr[BUF_MAX];
	unsigned char sub_addr[BUF_MAX];
	unsigned char dimmer_value[BUF_MAX];
	unsigned char time_delay[BUF_MAX];
	unsigned char pir_state[BUF_MAX];
	int scene_num;
	
}SCENE_P, *P_SCENE_P;

typedef struct back_p{
	UCHAR dev_addr[BUF_MAX];
	UCHAR dimmer_value[BUF_MAX][BUF_MAX];
	UCHAR pir_state[BUF_MAX];
	int module_num;
}BACK_P, *P_BACK_P;

typedef struct pir_conf{
	INTERFACE *dev;
	UCHAR scene_id;
	UCHAR dev_addr;
	UCHAR sub_addr;
	UCHAR nums[BUF_MAX];//同一组内的红外传感器的回路地址
	UCHAR pir_value;

}CONF_PIR,*P_CONF_PIR;


#endif

