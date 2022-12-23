#基准目录
ROOT_DIR = $(abspath .)
USER_SRC_BASE_DIR = $(ROOT_DIR)

#设置头文件路径
INC_BASE_DIR = $(ROOT_DIR)/include
CFLAGS = -I$(INC_BASE_DIR)

#列出所有文件夹的名字
USER_SRC_DIRS = $(shell find $(ROOT_DIR) -type d)
#列出所有后缀名字
USER_SRCS += $(foreach dir, $(USER_SRC_DIRS), $(wildcard $(dir)/*.c))

#所有.c文件变成.o结尾
USER_OBJ = $(basename $(USER_SRCS))
USER_OBJS = $(addsuffix .o, $(basename $(USER_SRCS) ) )

#列出所有.o的文件名字
USER_SRCSO += $(foreach dir, $(USER_SRC_DIRS), $(wildcard $(dir)/*.o))

#编译输出位置
OUTPUT_DIR = $(ROOT_DIR)/output
OUTPUT_DIR_OBJS = $(OUTPUT_DIR)/objs
USER_OBJS_OUT = $(subst $(ROOT_DIR),$(OUTPUT_DIR_OBJS), $(USER_OBJS))

COMPILE_PREX ?=/home/7200/arm-2014.05/bin/arm-none-linux-gnueabi-
CC = $(COMPILE_PREX)gcc

qc_code: $(USER_OBJS)
		$(CC) $(CFLAGS) -lpthread -o qc_code $(USER_OBJS)
$(USER_OBJ)%.o: $(USER_OBJ)%.c
		$(CC) $(CFLAGS) -c $< 

clean:
	rm $(USER_OBJS)
	rm qc_code



