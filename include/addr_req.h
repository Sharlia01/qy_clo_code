#ifndef __ADDR_REQ_H__
#define __ADDR_REQ_H__

#define LINE_LENGTH_PHO 19
#define DIAL_NUM 8
#define ALL_DEV 2
#define QY_DIMMER 0x35
#define QY_PIR 0x24

#define ADDR_PATH "/config/addr.txt"
#define DIMMING_PATH "/config/photometric.txt"
#define DIMMING_PATH_TMP "/config/photemetric_tmp.txt"
#define DIMMER_SCENE "/config/scene.txt"
#define DIMMER_SCENE_TMP "/config/scene_tmp.txt"
#define MODULE_NUM "/config/module_tmp.txt"
#define DEV_NUM "/config/dev_num.txt"

#define REMOTE_DB_PATH "/config/remote.db"
#define DIMMER_DB_PATH "/config/dimmer.db"

#include "data_handle.h"




int match_addr(unsigned char, char[]);
void read_dev_num(int *dimmer_cnt, int *pir_num);
int parse_dimmer_file(unsigned char dev_addr, unsigned char sub_addr);
void match_dimmer_scene(SCENE_P *para);
void get_callback_para(BACK_P * back_p, SCENE_P para);
int caculate_scene_module(SCENE_P para);




#endif
