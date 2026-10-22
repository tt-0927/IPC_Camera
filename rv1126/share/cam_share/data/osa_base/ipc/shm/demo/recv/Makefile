###Makefile###

APP = recv
BIN_DIR = bin
$(shell mkdir -p $(BIN_DIR))
TARGET = $(BIN_DIR)/$(APP)

CC = gcc
STRIP = strip

SRC = $(wildcard ./*.c)
SRC += $(wildcard ./../../*.c)
SRC += $(wildcard ./../../../../osa/src/*.c)
SRC += $(wildcard ./../../../../container/*.c)

INCLUDE := -I./
INCLUDE += -I./../../
INCLUDE += -I./../../../../osa/inc/
INCLUDE += -I./../../../../container/

LIBS := -lpthread

OBJS = $(SRC:.c=.o)
OBJS_DIR = objs
$(shell mkdir -p $(OBJS_DIR))
EXTERN_OBJ := $(foreach n,$(notdir $(OBJS)),$(OBJS_DIR)/$(n))

CFLAGS +=

all : $(TARGET)

$(TARGET):FIRST $(OBJS)
	$(CC) -o $(TARGET) $(EXTERN_OBJ) $(LIBS)

FIRST:
	rm -rf $(OBJS)

.c.o:
	$(CC) $(INCLUDE) $(CFLAGS) -c $<
	@mv *.o $(OBJS_DIR)


clean:
	@rm -rf $(OBJS_DIR) $(TARGET)
	@rm -rf $(OBJS_DIR)
	@rm -rf $(BIN_DIR)
