#include <fcntl.h> 
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <pthread.h>
#include <sys/stat.h> 
#include <linux/serial.h>


#include "interface_manage.h"
#include "transform.h"
#include "protocol.h"
#include "data_handle.h"

#define DELAYTIME  80	//延时时间ms
#define TIOCGRS485 0x542E
#define TIOCSRS485 0x542F


INTERFACE serial_dev_buf[SERIAL_CNT] = {0};

static struct termios newtios, oldtios; /*termianal settings */
static struct termios newtios_rts, oldtios_rts;
static int saved_portfd = -1;           /*serial port fd */
static int saved_portfd_rts = -1;

static void reset_tty_atexit(void)
{
	if (saved_portfd != -1)
		tcsetattr(saved_portfd, TCSANOW, &oldtios);
}

static void reset_tty_rts_atexit(void)
{
	if (saved_portfd_rts != -1)
		tcsetattr(saved_portfd_rts, TCSANOW, &oldtios_rts);
}

static void reset_tty_handler(int signal)
{
	if (saved_portfd != -1)
		tcsetattr(saved_portfd, TCSANOW, &oldtios);
	_exit(EXIT_FAILURE);
}

static void reset_tty_rts_handler(int signal)
{
	if (saved_portfd_rts != -1)
		tcsetattr(saved_portfd_rts, TCSANOW, &oldtios_rts);
	_exit(EXIT_FAILURE);
}



static int open_port(const char *portname)
{
	int portfd;
    struct sigaction sa;

    //printf("opening serial port:%s\n",portname);
    /* open serial port */                                                                             
    if ((portfd = open(portname, O_RDWR | O_NOCTTY)) < 0) {
        printf("open serial port %s failed\n ",portname);
        return portfd;
    }

    /* get serial port parnms,save away */
    tcgetattr(portfd, &newtios);
    memcpy(&oldtios, &newtios, sizeof newtios);
    /* configure new values */
    cfmakeraw(&newtios); /* see man page */
    newtios.c_iflag 	|= IGNPAR; /*ignore parity on input */
    newtios.c_oflag  	&= ~(OPOST | ONLCR | OLCUC | OCRNL | ONOCR | ONLRET | OFILL);
    newtios.c_cflag    	=  CS8 | CLOCAL | CREAD;
    newtios.c_cc[VMIN]  =  1; /* block until 1 char received 读数据的最小字节,没读到这个字节就不返回 */
    newtios.c_cc[VTIME] =  0; /* no inter-character timer  等待第一个数据的时间 */
	
		
	cfsetospeed(&newtios_rts, B9600);
	cfsetispeed(&newtios_rts, B9600);

    /* register cleanup stuff */
    atexit(reset_tty_atexit);
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = reset_tty_handler;
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGPIPE,&sa, NULL);
    sigaction(SIGTERM,&sa, NULL);

    /*apply modified termios */
    saved_portfd = portfd;
    tcflush(portfd, TCIFLUSH);
    tcsetattr(portfd, TCSADRAIN, &newtios);

    return portfd;
}

static int open_port_rts(const char *portname)
{
	int portfd;
	struct sigaction sa;
	struct serial_rs485 rs485conf;
	struct serial_rs485 rs485conf_bak;

	//printf("opening serial port, rts.....:%s\n",portname);
	/*open serial port */
	if ((portfd = open(portname, O_RDWR|O_NOCTTY, 0)) < 0) {
   		printf("open serial port %s failed\n", portname);
   		return portfd;
	}

	/*get serial port parnms,save away */
	tcgetattr(portfd, &newtios_rts);
	memcpy(&oldtios, &newtios_rts, sizeof newtios_rts);
	/* configure new values */
	cfmakeraw(&newtios_rts); /*see man page */
	newtios_rts.c_iflag    |=  IGNPAR; /*ignore parity on input */
	newtios_rts.c_oflag    &=  ~(OPOST | ONLCR | OLCUC | OCRNL | ONOCR | ONLRET | OFILL); 
	newtios_rts.c_cflag     =  CS8 | CLOCAL | CREAD;
	newtios_rts.c_cc[VMIN]  =  1; /* block until 1 char received */
	newtios_rts.c_cc[VTIME] =  0; /* no inter-character timer */


    cfsetospeed(&newtios_rts, B9600);
    cfsetispeed(&newtios_rts, B9600);


	/* register cleanup stuff */
	atexit(reset_tty_rts_atexit);
	memset(&sa, 0, sizeof sa);
	sa.sa_handler = reset_tty_rts_handler;
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGPIPE,&sa, NULL);
	sigaction(SIGTERM,&sa, NULL);
	/*apply modified termios */
	saved_portfd = portfd;
	tcflush(portfd, TCIFLUSH);
	tcsetattr(portfd, TCSADRAIN, &newtios_rts);
	
	if (ioctl(portfd, TIOCGRS485, &rs485conf) < 0) {
		/* Error handling.*/ 
		printf("ioctl1 TIOCGRS485 1 error.\n");
	}
	/* Enable RS485 mode: */
	rs485conf.flags |= SER_RS485_ENABLED;

	/* Set logical level for RTS pin equal to 1 when sending: */
	rs485conf.flags |= SER_RS485_RTS_ON_SEND;
	//rs485conf.flags |= SER_RS485_RTS_AFTER_SEND;

	/* set logical level for RTS pin equal to 0 after sending: */ 
	rs485conf.flags &= ~(SER_RS485_RTS_AFTER_SEND);
	//rs485conf.flags &= ~(SER_RS485_RTS_ON_SEND);

	/* Set rts delay after send, if needed: */
	rs485conf.delay_rts_after_send = 0x80;

	if (ioctl(portfd, TIOCSRS485, &rs485conf) < 0) {
		/* Error handling.*/ 
		printf("ioctl TIOCSRS485 2 error.\n");
	}

	if (ioctl(portfd, TIOCGRS485, &rs485conf_bak) < 0) {
		/* Error handling.*/ 
		printf("ioctl TIOCGRS485 3 error.\n");
	}
	return portfd;
}

void *data_to_other_thread(void *grp_tmp)
{
	int i, len; 
	READ_PARM_GROUP *grp = (READ_PARM_GROUP *)grp_tmp;
	unsigned char *data = grp->buf_final;
	unsigned char data_qy[DIMMER_NUM][BUF_MAX] = {0};//发送给启源串口的数据
	SCENE_P scene_para;

	scene_para.scene_num = 1;

	len = data[5];
	printf("\n-------(* 'ω')>︻╦╤─❇----%s recv data from clowire---------\n", grp->dev->name);                           
	for (i = 0; i < len; i++)                                                
   		printf("%02x ", data[i]);                                                     
	printf("\n------------------------------------------------------\n");

	//将数据放入该串口的读缓冲区
    cbus_enqueue(&grp->dev->queue_read, data);

	//将clowire指令转换成启源控制指令
	clowire_to_qiyuan(data, data_qy, &scene_para);

	if (data[0] == 0xFA && (data[6] == 0x0A || data[6] == 0x0c)){ //场景配置和删除反馈
		
		//将场景反馈指令写入clowire串口的写缓冲区
		cbus_enqueue(&grp->dev->queue_write, data);
	}
	
	//将数据放入启源串口的写缓冲区
	char name[20] = {0};
	INTERFACE *dev_other;
	for (i = 0; i < SERIAL_CNT; i++) {
		sprintf(name, "/dev/ttyS%d", i+1);
		if ((!strcmp(name, grp->dev->name)))
			continue;
		dev_other = find_link_by_name(name);
		if (!dev_other)
			continue;
		if (data_qy[0][0] == 0x3A)
			qy_enqueue(&dev_other->queue_write, data_qy, scene_para.scene_num);
	}

    *grp->p_flag = 0;
}


void *data_to_clo_thread(void *grp_tmp)
{
	int i, len; 
	READ_PARM_GROUP *grp = (READ_PARM_GROUP *)grp_tmp;
	unsigned char *data = grp->buf_final;

	len = data[2]+6;
	printf("\n-------(* 'ω')>︻╦╤─❇----%s recv data from qiyuan---------\n", grp->dev->name);                           
	for (i = 0; i < len; i++)                                                
   		printf("%02x ", data[i]);                                                     
	printf("\n------------------------------------------------------\n");

	//将数据放入该串口的读缓冲区
    cbus_enqueue(&grp->dev->queue_read, data);

	//将启源控制指令转换成clowire指令(反馈指令)
	qiyuan_to_clowire(data);
	
	//将数据放入其他串口的写缓冲区
	char name[20] = {0};
	INTERFACE *dev_other;
	for (i = 0; i < SERIAL_CNT; i++) {
		sprintf(name, "/dev/ttyS%d", i+1);
		if ((!strcmp(name, grp->dev->name)))
			continue;
		dev_other = find_link_by_name(name);
		if (!dev_other)
			continue;
		cbus_enqueue(&dev_other->queue_write, data);
	}

    *grp->p_flag = 0;

}


void *serial_read_thread(void *dev_tmp) 
{
    INTERFACE *dev = (INTERFACE *)dev_tmp;
	int flag = 0;
	int index = 0; 
	unsigned char buf_tmp[BUF_MAX] = {0};
	unsigned char buf_final[DATA_MAX] = {0};
	fd_set readfd;
    struct timeval timeout;
    timeout.tv_sec  = 0;
    timeout.tv_usec = DELAYTIME * 1000; //select超时时间

	READ_PARM_GROUP parm_group;
	parm_group.dev       = dev;
	parm_group.buf_tmp   = buf_tmp;
	parm_group.buf_final = buf_final;
	parm_group.p_flag    = &flag;
	parm_group.p_index   = &index; 
	parm_group.p_readfd  = &readfd;
	parm_group.p_timeout = &timeout;

    while (1) { //一直运行该串口的读函数
        dev->read(&parm_group);
		usleep(5);
    }
}

void *serial_write_thread(void *dev_tmp)
{
	INTERFACE *dev = (INTERFACE *)dev_tmp;

	while (1) { //一直运行该串口的写函数
		dev->write(dev);
		usleep(5);
	}
}

void *monitor_button(void *qy_device){
	int fd[2];
	char str[30] = {0};
	char str_led[30] = {0};
	char status = 1;
	char val;
	DEV_QY *qy_dev = (DEV_QY *)qy_device;

	sprintf(str, "/dev/inetbut0");
	sprintf(str_led, "/dev/inetled0"); 

	fd[0] = open(str, O_RDWR);
	if (fd[0] == -1){
		printf("can not open file %s \n", str);
	}

	fd[1] = open(str_led, O_RDWR);
	if (fd[1] == -1){
		printf("can not open file %s \n", str_led);
	}


	/* 监听按键数据 */
	while(1){
		
		read(fd[0], &val, 1);
		
		if(val == 0){ //检测到按键按下
			cbus_enqueue(qy_dev->dev->queue_write, qy_dev->data);
		
			write(fd[1], &status, 1);//将灯点亮

			break;
		}
	}
	
	close(fd[0]);
	close(fd[1]);

}

void innet_button(INTERFACE *dev){
	DEV_QY qy_device;
    pthread_t tid_read;

	qy_device.dev_seq = 1;
	
	//配置要写入该串口写缓冲区的入网指令
	config_data_innet(&qy_device);
	qy_device.dev = dev;//clowire串口

	//开线程监听按键数据，若按键按下则将指令写入串口写缓冲区
    pthread_create(&tid_read, NULL, monitor_button, &qy_device);
	
    pthread_detach(tid_read);

}

void general_serial_init(INTERFACE *dev)//初始化
{
	char delims[] = "/dev/ttyS";
	int num = atoi(strtok(dev->name, delims));

	if ((num % 2) && (num != 1))
		dev->fd = open_port(dev->name);//除1以外的奇数
	else
		dev->fd = open_port_rts(dev->name);

    if (dev->fd < 0) {
		dev->fd = -1;
        return;
    }

    //初始化缓冲区(读缓冲和写缓冲)
    queue_int(&dev->queue_read);
	queue_int(&dev->queue_write);

	if (!strcmp(dev->name, CLOWIRE_PORT)){//如果初始化的串口是clowire串口
		innet_button(dev);//入网第一组8路调光灯模块
	}

    //创建读线程
    pthread_t tid_read;
    pthread_create(&tid_read, NULL, serial_read_thread, dev);
    pthread_detach(tid_read);

	//创建写线程
    pthread_t tid_write;
    pthread_create(&tid_write, NULL, serial_write_thread, dev);
    pthread_detach(tid_write);

}

void general_serial_exit(int fd)
{
    close(fd);
}

//read data from clowire_port 
void general_serial_read(READ_PARM_GROUP *grp)
{
    int ret, len;
    unsigned char  buf_read[BUF_MAX] = {0};

	//select计时监听
	FD_ZERO(grp->p_readfd);
    FD_SET(grp->dev->fd, grp->p_readfd);
	ret = select(grp->dev->fd+1, grp->p_readfd, NULL, NULL, grp->p_timeout);
	if (!ret) {
		grp->dev->write_flag = 1;	//超时,表示监听的这段内总线上没有数据可读,可以开始发送数据
		return;
	} else if (ret > 0) {
		grp->dev->write_flag = 0;
	} else {
		return;
	}

    len = read(grp->dev->fd, buf_read, BUF_MAX);
    if (len <= 0) {
		printf("read failed!\n");
        return;
    }
	// printf("len = %d, read:", len);
	// int j;
	// for (j = 0; j < len; j++) {
	// 	printf("%02x ", buf_read[j]);
	// }
	// printf("\n");

    while (*grp->p_flag); 

    //把read数组的数据放入tmp数组中.(因为一次read可能读不到完整的一包,需要多次read然后将数据进行拼接)
    ret = read_to_tmp(grp, buf_read, len);
    if (ret < 0)
        return;

    //转义
    
    ES_opt_recv(grp);

    //截取有效数据
    get_valid_cbus_data(grp);
}


void general_serial_write(INTERFACE *dev)
{
	int len;
	int i;
    unsigned char data[BUF_MAX] = {0};

	//将本串口写缓冲区的数据提取到data中
    if (!cbus_dequeue(&dev->queue_write, data)) {
		//这包数据的长度
        len = data[5];

        //转义
		ES_opt_send(data, &len);

		//保证总线上一段时间内没有读取到数据才可以进行发送
    	while (1) {			
    	    if (dev->write_flag) {
				break;
			}
			usleep(5);
    	}
		
		//发送
        write(dev->fd, data, len);
    }
}

//将数据写进启源串口
void trans_serial_write(INTERFACE *dev)
{
	int i;
	int len;
	unsigned char data[DIMMER_NUM][QY_DATA_LEN] = {0};
	int line_num;
	//将写缓冲区的数据提取到data中
	if(!qy_dequeue(&dev->queue_write, data))
		{
			/* 不需要进行转义 */
			/*保证总线上一段时间内没有读取到数据才可以进行发送*/
			while (1)
			{
				if (dev->write_flag)
					break;
				usleep(5);
			}

			for(i = 0; i < (sizeof(data)/sizeof(data[0])); i++){
				len += strlen(data[i]);
			}
			line_num = len/strlen(data[0]);

			for (i = 0; i<line_num; i++){
				
				if(line_num == 1) //单独控制
					write(dev->fd, data[i], sizeof(data[i]));
				
				else if (line_num > 1){//场景控制中的延时发送
					sleep(data[i][10]);
					data[i][10] = 0x00;
					write(dev->fd, data[i], sizeof(data[i]));
				}
					
			}
			
	}
}

//read data from qiyuan port
void trans_serial_read(READ_PARM_GROUP *grp)
{	
	int ret,len;
	unsigned char buf_read[BUF_MAX] = {0};

	//select计时监听
	FD_ZERO(grp->p_readfd);
	FD_SET(grp->dev->fd, grp->p_readfd);
	ret = select(grp->dev->fd+1, grp->p_readfd, NULL, NULL, grp->p_timeout);	
	if (!ret) {
		grp->dev->write_flag = 1;	//超时,表示监听的这段内总线上没有数据可读,可以开始发送数据
		return;
	} else if (ret > 0) {
		grp->dev->write_flag = 0;
	} else {
		return;
	}

	len = read(grp->dev->fd, buf_read, BUF_MAX);
    if (len <= 0) {
		printf("read failed!\n");
        return;
    }
	
	while (*grp->p_flag);
	
    //把read数组的数据放入tmp数组中.(因为一次read可能读不到完整的一包,需要多次read然后将数据进行拼接)
    ret = read_to_tmp(grp, buf_read, len);
    if (ret < 0)
        return;

    //截取有效数据
    get_valid_trans_data(grp);


}


void register_serial(void)
{
	int i;
	char name[20] = {0};
	
	for (i = 0; i < SERIAL_CNT; i++) {
		
		if (i == 0){
			sprintf(name, "/dev/ttyS1");
			strcpy(serial_dev_buf[i].name, name);
		}
		else{
			sprintf(name, "/dev/ttyS8");
			strcpy(serial_dev_buf[i].name, name);
		}

		if (!strcmp(name, CLOWIRE_PORT)){
			serial_dev_buf[i].init  = general_serial_init;
			serial_dev_buf[i].exit  = general_serial_exit;
			serial_dev_buf[i].read  = general_serial_read;
			serial_dev_buf[i].write = general_serial_write;
		}
		else
		{
			/* The data of this uart port is sent by can port */
			serial_dev_buf[i].init  = general_serial_init;
			serial_dev_buf[i].exit  = general_serial_exit;
			serial_dev_buf[i].read  = trans_serial_read;
			serial_dev_buf[i].write = trans_serial_write;
		}
			
	}
	
}
		

