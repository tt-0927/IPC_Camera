NAME := mkpack

CC:=gcc
CXX:=g++
AR:=ar
LINK:=gcc
LINKFLAGS := -fpic -shared
#LINKFLAGS := 
CFLAGS := -fpic -g
Q_ := 
.PHONY:	clean

INCDIR += -I .
SRC = $(wildcard *.c *.cpp)
DIR = $(notdir$(SRC))
OBJ += $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SRC)))

CFLAGS += $(INCDIR)
 
#DYN_LINKS_WITH +=  -lpthread -lrt -lm -lstdc++
DYN_LINKS_WITH +=  -lpthread -lrt -lm
STATIC_LINKS_WITH += 



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
	$(Q_)$(LINK)  -o $(NAME) $(OBJ) $(DYN_LINKS_WITH) '-Wl,--start-group' $(STATIC_LINKS_WITH) '-Wl,--end-group'

install:
	cp $(NAME) /usr/local/bin

clean:
	rm $(NAME) -f
	rm $(OBJ)  -rf

