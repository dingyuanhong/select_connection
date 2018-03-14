CORSS_PREFIX :=
SYSROOT :=

$(warning $(CORSS_PREFIX))
$(warning $(SYSROOT))

CC = $(CORSS_PREFIX)gcc
AR = $(CORSS_PREFIX)ar
RM = rm -rf

#原始目录
SRC_PATH :=.
MODULES :=
#目标名
TARGET :=
#源文件
MODULES += $(wildcard $(SRC_PATH)/src/*.c)
MODULES += $(wildcard $(SRC_PATH)/*.c)
SRCS += $(MODULES)
#中间文件
OBJS := $(SRCS:.c=.o)
TARGET =

MODULE_OBJS := $(MODULES:.c=.o)
OBJ_TEST=win32/server.o
OBJS_TEST=$(MODULE_OBJS) $(OBJ_TEST)
TARGET_TEST=server

OBJ_INFO=win32/client.o
OBJS_INFO=$(MODULE_OBJS) $(OBJ_INFO)
TARGET_INFO=client

ALL_OBJS=$(OBJS) $(OBJS_TEST) $(OBJ_INFO)

#动态库
LIBS := pthread

#模块库文件
MOULE_LIBRARY_PATH = $(SYSROOT)/usr/lib/ $(SYSROOT)/usr/local/lib/

#头文件路径
INCLUDE_PATH :=.
#动态库路径
LIBRARY_PATH :=

INCLUDE_PATH += $(SYSROOT)/usr/include
INCLUDE_PATH += ./include
INCLUDE_PATH += ./src
INCLUDE_PATH += ./test
INCLUDE_PATH += ./win32
LIBRARY_PATH += $(MOULE_LIBRARY_PATH)
LIBRARY_PATH += ./

RELEASE = 1
BITS =

#ifeq ( 1 , ${DBG_ENABLE} )
#	CFLAGS += -D_DEBUG -O0 -g -DDEBUG=1
#endif

CFLAGS = -Wall -DMAIN_TEST -DUSE_BOOL
LFLAGS =

#头文件
CFLAGS += $(foreach dir,$(INCLUDE_PATH),-I$(dir))

#库路径
LDFLAGS += $(foreach lib,$(LIBRARY_PATH),-L$(lib))

#库名
LDFLAGS += $(foreach lib,$(LIBS),-l$(lib))

#检查版本
ifeq ($(RELEASE),0)
	#debug
	CFLAGS += -g
else
	#release
	CFLAGS += -O3 -DNDEBUG
endif

#检查位宽
ifeq ($(BITS),32)
	CFLAGS += -m32
	LFLAGS += -m32
else
	ifeq ($(BITS),64)
		CFLAGS += -m64
		LFLAGS += -m64
	else
	endif
endif

$(warning $(OBJS))

#操作命令
all:clean build

$(ALL_OBJS):%.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@

build_static:$(OBJS)
	# $(AR) -cru $(TARGET) $(OBJS)

build_test:build_static $(OBJ_TEST)
	$(CC) $(CFLAGS) $(LFLAGS) -o $(TARGET_TEST) $(OBJ_TEST) $(LDFLAGS)

build_info:build_static $(OBJ_INFO)
	$(CC) $(CFLAGS) $(LFLAGS) -o $(TARGET_INFO) $(OBJ_INFO) $(LDFLAGS)

build:build_test build_info
	$(RM) $(ALL_OBJS)

clean:
	echo $(SRCS)
	$(RM) $(ALL_OBJS) $(TARGET) $(TARGET_TEST) $(TARGET_INFO)
