#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <pthread.h>


#define QUEUE_MAX 300 
#define DATA_MAX  30
#define REMOTE_DATA_SIZE 5

typedef struct queue {
    pthread_mutex_t queue_mutex;
    unsigned char data[QUEUE_MAX][DATA_MAX];
    int r_pointer;  //读指针
    int w_pointer; //写指针
}QUEUE, *P_QUEUE;

void queue_int(QUEUE *);
int is_queue_full(QUEUE *);
int is_queue_empty(QUEUE *);
int enqueue(QUEUE *q, unsigned char *);
int dequeue(QUEUE *q, unsigned char *);

#endif

