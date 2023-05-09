
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
	int i,len;
	
	dimmer_addr_flag = match_addr(dev_addr,"dimmer_addr");
	pir_addr_flag = match_addr(dev_addr,"pir_addr");
	
		switch(cmd) 
		{
			case SINGLE_CONTROL: //单独控制
				printf("single dev control command from clowire\n");
				if (dimmer_addr_flag) {
					single_control_trans(complite_data);
					
					//单独控制指令只有一条
					len = complite_data[4] + 5;
					memcpy(qy_data[0], complite_data, len);
					
					printf("\n-------(* 'ω')>︻╦╤─❇---- handle data ---------\n");						   
					for (i = 0; i < len; i++)												 
						printf("%02x ", complite_data[i]);													  
					printf("\n------------------------------------------------------\n");

				}
				else if (pir_addr_flag) //红外传感器的控制指令
					pir_control(complite_data);
				break;
				
			case SCENE_CONFIG: //场景配置
				printf("scene config command from clowire\n");
				if (dimmer_addr_flag || pir_addr_flag)
					scene_config_trans(complite_data);
				break;
				
			/*case SCENE_CONTROL: //场景控制
				printf("scene control command from clowire\n");
				scene_control_trans(complite_data, qy_data, para);
				break;*/
				
			case SCENE_CONFIG_DELETE://场景配置删除
				printf("scene config delete command from clowire\n");
				if (dimmer_addr_flag || pir_addr_flag)
				scene_delete_trans(complite_data);		
				break;
			case DELAY_CONFIG:
				printf("delay input config from clowire\n");
				delay_config_inf(complite_data);
				break;		
				
		}	
	
}

void qiyuan_to_clowire(unsigned char *complite_data)
{	

	printf("dimmer feedback from qiyuan\n");
	
	dimmer_feedback(complite_data);

}

