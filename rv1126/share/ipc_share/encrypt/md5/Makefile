NAME := test

BIT:=64
PLATFORM:=linux
ifeq ($(PLATFORM), arm)
    PRE:=arm-none-linux-gnueabi-
    BIT:=
endif

CC:=$(PRE)gcc
CXX:=$(PRE)g++
AR:=$(PRE)ar
LINK:=$(PRE)gcc
LINKFLAGS := -fpic -shared
#LINKFLAGS := 
CFLAGS := -fpic -g
Q_ := 
.PHONY:	clean

INCDIR += -I ../reach_share/\
	  -I ../log/\
          -I ../md5/
SRC = $(wildcard *.c *.cpp)
SRC +=$(filter ../reach_share/reach_os.c,$(wildcard ../reach_share/*.c))

DIR = $(notdir$(SRC))
OBJ += $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SRC)))

CFLAGS += $(INCDIR)
 
#DYN_LINKS_WITH +=  -lrt -lm -lstdc++
DYN_LINKS_WITH +=  -lrt -lm
ifeq ($(PLATFORM), arm)
     DYN_LINKS_WITH += 
endif

STATIC_LINKS_WITH += 

ifeq ($(PLATFORM), linux)
    STATIC_LINKS_WITH += $(wildcard ../../lib64/*.a)
endif

%.o: %.c
	echo '<$(CC)>[$(DBG)] Compiling object file "$@" ...'
	$(Q_)${CC} $(CFLAGS) -c $< -o $@

%.o: %.cpp
	echo '<$(CXX)>[$(DBG)] Compiling object file "$@" ...'
	$(Q_)${CXX} $(CFLAGS) -c $< -o $@

all: $(NAME)
$(NAME):$(OBJ)
	echo 1$(OBJ)
	echo 2'<$(AR)>creating binary "$(NAME)"'
	$(Q_)$(LINK)  -o $(NAME) $(OBJ) $(DYN_LINKS_WITH) '-Wl,--start-group' $(STATIC_LINKS_WITH) '-Wl,--end-group'  && cp $(NAME) ~/workdir/rfs/.

install:
	cp $(NAME) /usr/local/lib
	cp $(NAME) ~/workdir/rfs/usr/lib
	cp *.h /usr/local/include
	cp *.h ~/workdir/rfs/usr/include

clean:
	rm $(NAME) -f
	rm $(OBJ)  -rf

mkimage:mkimage.o ../md5/md5lib.o ../reach_share/reach_os.o
	$(Q_)$(LINK)  -o mkimage mkimage.o ../md5/md5lib.o ../reach_share/reach_os.o -lpthread -lrt
mkimage_clean:
	rm -f *.o  ../md5/md5lib.o ../reach_share/reach_os.o mkimage
