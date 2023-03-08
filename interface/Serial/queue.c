#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "interface_manage.h"
#include "protocol.h"

QUEUE queue_buf[SERIAL_CNT] = {0};

int is_queue_full(QUEUE *q)
{
    if (((q->w_pointer+1)%QUEUE_MAX) == q->r_pointer) {
        return 0;
    } else {
        return 1;
    }
}

int is_queue_empty(QUEUE *q)
{
    if ((q->w_pointer) == (q->r_pointer)) {
        return 0;
    } else {
        return 1;
    }
}

int remote_enqueue(QUEUE *q, unsigned char *buf)
{
    pthread_mutex_lock(&q->queue_mutex);

    if (!is_queue_full(q)) {
        pthread_mutex_unlock(&q->queue_mutex);
        return -1;
    }

    int len = REMOTE_DATA_SIZE;
    memcpy(q->data[q->w_pointer], buf, len);

    q->w_pointer = (q->w_pointer + 1) % QUEUE_MAX;
    pthread_mutex_unlock(&q->queue_mutex);
    return 0;
}

int remote_dequeue(QUEUE *q, unsigned char *buf)
{
    pthread_mutex_lock(&q->queue_mutex);

    if (!is_queue_empty(q)) { 
        pthread_mutex_unlock(&q->queue_mutex);
        return -1;
    }

    int len = REMOTE_DATA_SIZE;
    memcpy(buf, q->data[q->r_pointer], len);

    q->r_pointer = (q->r_pointer + 1) % QUEUE_MAX;
    pthread_mutex_unlock(&q->queue_mutex);
    return 0;
}

int cbus_enqueue(QUEUE *q, unsigned char *buf) //数据写入缓冲区
{
	
	
    pthread_mutex_lock(&q->queue_mutex);
    if (!is_queue_full(q)) { //读指针和写指针必须相等，保证先写再读
        pthread_mutex_unlock(&q->queue_mutex);
        return -1;
    }
    int len = buf[5];
    memcpy(q->data[q->w_pointer], buf, len);

    q->w_pointer = (q->w_pointer + 1) % QUEUE_MAX; //当写指针为299时，下一个又是0
	

	pthread_mutex_unlock(&q->queue_mutex);
    return 0;
}

int qy_enqueue(QUEUE *q, unsigned char buf[][BUF_MAX], int line_num) //数据写入启源串口的缓冲区
{
    pthread_mutex_lock(&q->queue_mutex);

    if (!is_queue_full(q)) { //读指针和写指针必须相等
        pthread_mutex_unlock(&q->queue_mutex);
        return -1;
    }

	int len = 0;
	int i;

	for (i =0; i < line_num; i++){
		memcpy(q->data[q->w_pointer],buf[i], QY_DATA_LEN);
		q->w_pointer = (q->w_pointer + 1) % QUEUE_MAX;
	}

    pthread_mutex_unlock(&q->queue_mutex);
    return 0;
}

int qy_dequeue(QUEUE *q, unsigned char *buf) //将缓冲区的数据提取出来
{
    pthread_mutex_lock(&q->queue_mutex);

    if (!is_queue_empty(q)) { //读缓冲区数据时写指针一定大于读指针
        pthread_mutex_unlock(&q->queue_mutex);
        return -1;
    }

    int len = q->data[q->r_pointer][4] + 5;
    memcpy(buf, q->data[q->r_pointer], len);

    q->r_pointer = (q->r_pointer + 1) % QUEUE_MAX;
    pthread_mutex_unlock(&q->queue_mutex);
    return 0;
}


int cbus_dequeue(QUEUE *q, unsigned char *buf) //将缓冲区的数据提取出来
{
    pthread_mutex_lock(&q->queue_mutex);

    if (!is_queue_empty(q)) { //读缓冲区数据时写指针一定大于读指针
        pthread_mutex_unlock(&q->queue_mutex);
        return -1;
    }

    int len = q->data[q->r_pointer][5];
    memcpy(buf, q->data[q->r_pointer], len);

    q->r_pointer = (q->r_pointer + 1) % QUEUE_MAX;
    pthread_mutex_unlock(&q->queue_mutex);
    return 0;
}

void queue_int(QUEUE *q)
{
    q->r_pointer = 0;
    q->w_pointer = 0;
    pthread_mutex_init(&q->queue_mutex, NULL);
}

