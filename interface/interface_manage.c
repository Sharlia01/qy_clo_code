#include <stdio.h>
#include "interface_manage.h"

INTERFACE *g_hw_link_head = NULL;

INTERFACE *find_link_by_name(char *name)
{
    INTERFACE *p_tmp = g_hw_link_head;
    while (p_tmp) {
        if ((!strcmp(p_tmp->name, name)) && (p_tmp->fd > 0))
            return p_tmp;
        p_tmp = p_tmp->next;
    }
    return NULL;
}
void hardware_init(void)
{
    INTERFACE *p_tmp = g_hw_link_head;
    int ret;

    //遍历链表,调用硬件的初始化函数
    while (p_tmp) {
        p_tmp->init(p_tmp);
        p_tmp = p_tmp->next;
    }
}

void hardware_exit(void)
{
    INTERFACE *p_tmp = g_hw_link_head;
    while (p_tmp) {
        p_tmp->exit(p_tmp->fd);
        p_tmp = p_tmp->next;
    }
}



void interface_add_link(INTERFACE *dev)
{
    dev->next = g_hw_link_head;
    g_hw_link_head = dev;
}


void register_interface(void)
{
    register_watchdog();
    register_serial();
	
}

