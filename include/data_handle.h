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




#endif

