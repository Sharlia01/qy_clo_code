
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "transform.h"
#include "protocol.h"
#include "addr_req.h"
#include "data_handle.h"
#include "interface_manage.h"


// clowire网关发送的数据转换成启源控制指令
void clowire_to_qiyuan(unsigned char *complite_data, unsigned char qy_data[][BUF_MAX], SCENE_P *para)
{
    unsigned char dev_addr = complite_data[1];
    unsigned char sub_addr = complite_data[2];
    unsigned char cmd      = complite_data[6];
    unsigned char scene_id = 0;
	int dimmer_addr_flag = 0;
	int pir_addr_flag = 0;
	
	dimmer_addr_flag = match_dimmer_addr(dev_addr);
	pir_addr_flag = match_pir_addr(dev_addr);
	
		switch(cmd) 
		{
			case SINGLE_CONTROL: //单独控制
				printf("single dev control command from clowire\n");
				if (dimmer_addr_flag)
					single_control_trans(complite_data);
				else if (pir_addr_flag)
					pir_control(complite_data);
					
				memcpy(qy_data[0], complite_data, strlen(complite_data));
				
				
			case SCENE_CONFIG: //场景配置
				printf("scene config command from clowire\n");
				if (dimmer_addr_flag || pir_addr_flag)
					scene_config_trans(complite_data);
				
				//memcpy(qy_data[0], complite_data, strlen(complite_data));
				
			case SCENE_CONTROL: //场景控制
				printf("scene control command from clowire\n");
				scene_control_trans(complite_data, qy_data, para);
			
			case SCENE_CONFIG_DELETE://场景配置删除
				printf("scene config delete command from clowire\n");
				if (dimmer_addr_flag || pir_addr_flag)
				scene_delete_trans(complite_data);
				//memcpy(qy_data[0], complite_data, strlen(complite_data));
				
		}
	
}

void qiyuan_to_clowire(unsigned char *complite_data)
{	
	unsigned char circuit_num = complite_data[8];
	unsigned char control_id = complite_data[7];

	printf("dimmer feedback from qiyuan\n");
	dimmer_feedback(complite_data);

	
}

