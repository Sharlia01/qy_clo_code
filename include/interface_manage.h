#ifndef __INTERFACE_MANAGER_H__
#define __INTERFACE_MANAGER_H__

#include <sys/types.h>
#include "queue.h"

#define WATCHDOG   1
#define BUF_MAX    512
#define DIMMER_NUM 200

#define CLOWIRE_PORT "/dev/ttyS1"
#define SERIAL_CNT 2

struct hardware;


//串口读取数据的各个函数所需要的参数打包成结构体
typedef struct read_parameter_group {
    struct hardware *dev;
    int     *p_flag;
    int     *p_index; 
	unsigned char *buf_tmp;
    unsigned char *buf_final;
    fd_set  *p_readfd;
    struct timeval *p_timeout;
}READ_PARM_GROUP, P_READ_PARM_GROUP;

typedef struct device_qy{
	unsigned char dev_addr;
	unsigned char dev_seq;
	unsigned char data[BUF_MAX];
	unsigned char dev_type;
	struct hardware *dev;
}DEV_QY, *P_DEV_QY;


typedef struct hardware {
    char name[20];
    int  type;
    int  fd;
    int  write_flag;
    int  handle_flag; 
    QUEUE queue_read;
    QUEUE queue_write;
    void(*init)(struct hardware *);
    void(*exit)(int);
    void(*read)(READ_PARM_GROUP *);
    void(*write)(struct hardware *);
    struct hardware *next;
}INTERFACE, *P_INTERFACE;

//interface_manage.c
void hardware_init(void);
void hardware_exit(void);
void interface_add_link(INTERFACE *);
void register_interface(void);
INTERFACE *find_link_by_name(char *);



void register_watchdog(void);

//serial.c
void register_serial(void);
void *data_to_other_thread(void *);
void *data_to_clo_thread(void *);



#endif

