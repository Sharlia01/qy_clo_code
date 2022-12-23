#ifndef __TRANSFORM_H__
#define __TRANSFORM_H__

#include "data_handle.h"

void clowire_to_qiyuan(unsigned char *complite_data, unsigned char qy_data[][BUF_MAX], SCENE_P *para);

void qiyuan_to_clowire(unsigned char *);

#endif