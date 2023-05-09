#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#include <sys/time.h>

#include "interface_manage.h"
#include "protocol.h"
//异或校验
unsigned char XOR_check(unsigned char *data, unsigned char lenth)
{
   unsigned char sum = 0;
   if (data == NULL) {   
       return 0;                                                                    
   }       

   int i;   
   for (i = 0; i < lenth; i++) {       
       sum ^= *(data+i);
   }
   return sum;
}   

//crc16校验

/*unsigned char crc16(unsigned char *arr, unsigned int len)
{
	unsigned char crc_in = 0x0000; 
	unsigned char crc_poly = 0x8408; //多项式hex码
	unsigned char ch = 0;
	unsigned char i;
	while(len--)
	{
		ch = *(arr++);
		
		crc_in ^=ch; //异或，若两个二进制位相同，则结果为0，否则为1
		for(i=0; i<8; i++) {
			if(crc_in&0x0001) {
				crc_in=(crc_in<<1)^crc_poly;
			} else {
				crc_in=crc_in<<1;
			}
		}
	}
	return	(crc_in);
	
}*/



int read_to_tmp(READ_PARM_GROUP *grp, unsigned char *buf_read, int len)
{
    if ((*grp->p_index + len) >= BUF_MAX) {
        *grp->p_index = 0;
        memset(grp->buf_tmp, 0, BUF_MAX);
        return -1;
    }
    memcpy(&grp->buf_tmp[*grp->p_index], buf_read, len);
    *grp->p_index = *grp->p_index + len;

    // printf("i = %d, buf_tmp:", *grp->p_index);
    // int i;
    // for (i = 0; i < *grp->p_index ; i++) {
    //     printf("%02x ", grp->buf_tmp[i]);
    // }
    // printf("\n");

    return 0;
}

//接收转义.(FE 01)->(FA), (FE 02)->(FE)
void ES_opt_recv(READ_PARM_GROUP *grp)
{
    int i, count = 0;
    unsigned char *buf_tmp = grp->buf_tmp;

    for (i = 0; i < BUF_MAX; i++) {
        if (buf_tmp[i] == 0xFE) {
            count = 0;
            if (buf_tmp[i + 1] == 0x01) {
                buf_tmp[i] = 0xFA;
                memcpy(&buf_tmp[i+1], &buf_tmp[i+2], BUF_MAX-(i+2));
                (*grp->p_index)--;

            } else if (buf_tmp[i + 1] == 0x02) {
                buf_tmp[i] = 0xFE;
                memcpy(&buf_tmp[i+1], &buf_tmp[i+2], BUF_MAX-(i+2));
                (*grp->p_index)--;
            }

        } else if (buf_tmp[i] == 0x00) {
            count++;
            if (count >= 20) {
                count = 0;
                return;
            }

        } else {
            count = 0;
        }
    }
}

//发送转义.(FA)->(FE 01)(包头FA不用转), (FE)->(FE 02)
void ES_opt_send(unsigned char *data, int *p_len)
{
    int i;
    for (i = 1; i < *p_len; i++) { 
        if (data[i] == 0xFE) {
            data[i] = 0xFE;
            memcpy(&data[i+2], &data[i+1], *p_len-i-1);
            data[i+1] = 0x02;
            (*p_len)++;
        }

        if (data[i] == 0xFA) {
            data[i] = 0xFE;                                   
            memcpy(&data[i+2], &data[i+1], *p_len-i-1);
            data[i+1] = 0x01;
            (*p_len)++;
        }
    }
}

//截取clowire网关发过来的有效数据
void get_valid_cbus_data(READ_PARM_GROUP *grp)
{
    int count = 0;
    int read_ok = 0;
    int i, j, ret, del_len, package_size;
    unsigned char *buf_tmp = grp->buf_tmp;
    unsigned char buf_tmp_tmp[BUF_MAX] = {0};
    pthread_t tid;

    //每次遍历整个数组，发现FA之后截取一包数据
    for (i = 0; i < BUF_MAX; i++) {
        if (buf_tmp[i] == 0xFA) { //包头
            count = 0;
            package_size = buf_tmp[i + 5]; //读取包长度
            if ((buf_tmp[i + (package_size - 1)] == XOR_check(buf_tmp + i, (package_size - 1))) \
                    && ((package_size > 0) && (package_size < DATA_MAX)) && grp->len >= package_size) {  //校验成功并且数据长度小于30
                while (*grp->p_flag);

                memcpy(grp->buf_final, &buf_tmp[i], package_size); //得到一包有效数据

                //创建线程将有效数据放入该串口的读缓冲区和其他串口的写缓冲区
                *grp->p_flag = 1;

				grp->len -= package_size;
				
                ret = pthread_create(&tid, NULL, data_to_other_thread, grp);
                if (ret != 0)
                    printf("uart send other thread create failed.\n");
                else
                    pthread_detach(tid);

                del_len = i + package_size;
                *grp->p_index = *grp->p_index - del_len;

                //读到完整的一包后,把后面的数据放到buf_tmp的最前面
                memset(buf_tmp_tmp, 0, BUF_MAX);
                memcpy(buf_tmp_tmp, buf_tmp, BUF_MAX);
                memset(buf_tmp, 0, BUF_MAX);
                memcpy(buf_tmp, buf_tmp_tmp+del_len, BUF_MAX-del_len);

                //从头遍历数组
                i = -1;
            }

        } else if (buf_tmp[i] == 0x00) {
            count++;
            if (count >= 20) {
                count = 0;
                break;
            }

        } else {
            count = 0;
        }
    }
 }


//qy串口发过来的数据
 void get_valid_trans_data(READ_PARM_GROUP *grp) 
{
	int count = 0;
	int read_ok = 0;
	int i,j,ret, del_len, package_size;
	unsigned char *buf_tmp = grp->buf_tmp;
	unsigned char buf_tmp_tmp[BUF_MAX] = {0};
	pthread_t tid;

	/*每次遍历整个数组，发现00 04之后截取一包数据*/
	for (i =0; i < BUF_MAX; i++) 
	{
		if (buf_tmp[i] == 0x80 && buf_tmp[i+1] == 0xFF )//启源数据的帧头
		{
			count = 0;
			package_size = buf_tmp[i+4]+5;
			//不进行校验位检测
			if(buf_tmp[i+6]!=0 &&((package_size > 0) && (package_size < DATA_MAX)) \
				&& grp->len >= package_size)
			{
				while(*grp->p_flag);

				memcpy(grp->buf_final, &buf_tmp[i], package_size);
				//创建线程将有效数据放入该串口的读缓冲区和其他串口的写缓冲区
                *grp->p_flag = 1;
                ret = pthread_create(&tid, NULL, data_to_clo_thread, grp);

                if (ret != 0)
                    printf("uart send other thread create failed.\n");
                else
                    pthread_detach(tid);

				
                del_len = i + package_size;
                *grp->p_index = *grp->p_index - del_len;
				grp->len -=package_size;

                //读到完整的一包后,把后面的数据放到buf_tmp的最前面
                memset(buf_tmp_tmp, 0, BUF_MAX);
                memcpy(buf_tmp_tmp, buf_tmp, BUF_MAX);
                memset(buf_tmp, 0, BUF_MAX);
                memcpy(buf_tmp, buf_tmp_tmp+del_len, BUF_MAX-del_len);

                //从头遍历数组
                i = -1;
				
			}

		}else if (buf_tmp[i+2] == 0x00) {
            count++;
            if (count >= 20) {
                count = 0;
                break;
            }

        } else {
            count = 0;
        }
	}

}




