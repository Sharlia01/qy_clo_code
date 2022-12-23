/**********************************************************
	>This is created for qiyuan dimmer lights to join 
	>Clowire smart home system
	>And it's the main code file 
*************************************************************/


#include <stdio.h>
#include "addr_req.h"
#include "transform.h"
#include "interface_manage.h"
#include "data_handle.h"


int main(int argc, char **argv[])
{

	/* 申请8路调光模块和8路继电器模块设备地址 */
	request_address();


	/* 注册硬件(看门狗,485串口)加入到链表进行管理 */
	register_interface();
	
    //初始化链表中的硬件
    hardware_init();

	//处理clowire串口读缓冲区的数据
	while (1){
		get_handle(); //启源设备入网
		usleep(5);
	}

    hardware_exit();

	 return 0;
}

