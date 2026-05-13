
ARCH = $(shell arch)
BITS =
ifeq "$(findstring x86_64,$(ARCH))" "x86_64"
BITS = 64
endif

ON = 1
OFF = 0


VERSION =1.1


#APP = control_$(VERSION)_$(ARCH)
APP = client_demo
BIN_DIR = bin
$(shell mkdir -p $(BIN_DIR)) 
TARGET =$(BIN_DIR)/$(APP)


SRC = $(wildcard ./*.c)\
 $(wildcard ./../../*.c)\
 $(wildcard ./../../../../list/*.c)\
 $(wildcard ./../../../osa/src/*.c)\
 $(wildcard ./../../../container/*.c)\

OBJS = $(SRC:.c=.o) $(CPPOBJS)
OBJS_DIR = objs
$(shell mkdir -p $(OBJS_DIR)) 
#OBJS_DEST = $(wildcard $(OBJS_DIR)/*.o)
EXTERN_OBJ :=  $(foreach n,$(notdir $(OBJS)),$(OBJS_DIR)/$(n))


CC = arm-hisiv400-linux-gcc

INCLUDE := -I./ \
 -I./../../ \
 -I./../../libevent/ \
 -I./../../libevent/event2/ \
 -I./../../../../list/ \
 -I./../../../osa/inc/ \
 -I./../../../container/ \
 
LIBS += ../../../../../../lib/staticso/libevent.a -lpthread -ldl -lrt
LIBS += ../../../../../../lib/staticso/libevent_pthreads.a 

 
CFLAGS += -D_FILE_OFFSET_BITS=64
#-Werror
#CFLAGS := -O2 -Wall -Wno-strict-aliasing -g
#CFLAGS = -O2
all: $(TARGET)


$(TARGET):FIRST $(OBJS)
	echo #$(OBJS)#
	echo #$(EXTERN_OBJ)#
	$(CC) -o $(TARGET) $(EXTERN_OBJ) $(LIBS)

FIRST:
	rm -rf $(OBJS)
	
.SUFFIXES:.c	

.c.o:
	$(CC) $(INCLUDE) $(CFLAGS) -c $<
	@mv *.o $(OBJS_DIR)

.cpp.o:
	$(CC) $(INCLUDE) $(CFLAGS) -c $<
	@mv *.o $(OBJS_DIR)

pkgs:
	mkdir -p pkg/$(APP)
	cp -rf $(TARGET) pkg/$(APP)
	cp -rf pkg/$(SERVICE_TYPE)d pkg/$(APP)
	cp -rf pkg/$(TARGET)d pkg/$(APP)
	cp -rf pkg/install_$(SERVICE_TYPE) pkg/$(APP)
	cp -rf pkg/uninstall_$(SERVICE_TYPE) pkg/$(APP)
	cp -rf pkg/readme_$(SERVICE_TYPE) pkg/$(APP)
	cp -rf pkg/cfg_$(SERVICE_TYPE).xml pkg/$(APP)
	cp -rf pkg/nslog_$(SERVICE_TYPE).conf pkg/$(APP)
	echo $(APP) > pkg/version
	cp -rf pkg/version pkg/$(APP)
	cd pkg;tar cfvz $(APP).build$(DATE).tgz $(APP);rm -rf $(APP)
	cd pkg;cat inst_script.sh $(APP).build$(DATE).tgz > $(APP)_intaller;chmod +x $(APP)_intaller
install:
	cp ./bin/control /home/zxb/work/nfsroot/opt/dvr_rdk/ti816x_4.0/bin

.PHONY:clean
clean:
	@rm -rf $(OBJS_DIR) $(TARGET)
	@rm -rf pkg/$(APP)*
	@rm -rf $(OBJS_DIR)
	@rm -rf $(BIN_DIR)
	@rm -rf version.h
	@rm -rf control_config.xml log_file
