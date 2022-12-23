#ifndef __ADDR_REQ_H__
#define __ADDR_REQ_H__

#define LINE_LENGTH_PHO 19
#define DIAL_NUM 8
#define ALL_DEV 2
#define QY_DIMMER 0x05
#define QY_PIR 0x14

#define ADDR_PATH "/config/addr.txt"
#define DIMMING_PATH "/config/photometric.txt"
#define DIMMING_PATH_TMP "/config/photemetric_tmp.txt"
#define DIMMER_SCENE "/config/scene.txt"
#define DIMMER_SCENE_TMP "/config/scene_tmp.txt"
#define PIR_PATH "/config/pir.txt"
#define PIR_TMP_PATH "/config/pir_tmp.txt"
#define DEV_NUM "/config/dev_num.txt"



int match_dimmer_addr(unsigned char);
int match_pir_addr(unsigned char);

#endif
