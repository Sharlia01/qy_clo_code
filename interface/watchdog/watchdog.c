

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>

#include "interface_manage.h"

void *feed_dog(void *fd_tmp)
{
    int fd = (int)fd_tmp;
    while (1) {
        ioctl(fd, WDIOC_KEEPALIVE, 0);
        sleep(1);
    }
}
void watchdog_init(INTERFACE *dev)
{
    dev->fd = open(dev->name, O_RDWR);
    if (dev->fd < 0) {
        perror("watchdog");
        exit(EXIT_FAILURE);
    }

    int ret;
    pthread_t tid;
    ret = pthread_create(&tid, NULL, feed_dog, (void *)dev->fd);
    if (ret) {
        printf("feed_dog thread creat failed...\n");
        exit(EXIT_FAILURE);
    }
    pthread_detach(tid);
}

void watchdog_exit(int fd)
{
    close(fd);
}

INTERFACE watchdog = {
    .name = "/dev/watchdog",
    .type = WATCHDOG,
    .init = watchdog_init,
    .exit = watchdog_exit,
    .next = NULL,
};
	
void register_watchdog(void)
{
   interface_add_link(&watchdog); 
}

