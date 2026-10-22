#****************************************************************************
#
# Makefile for TinyXml test.
# Lee Thomason
# www.grinninglizard.com
#
# This is a GNU make (gmake) makefile
#****************************************************************************

# DEBUG can be set to YES to include debugging info, or NO otherwise
DEBUG          := NO

# PROFILE can be set to YES to include profiling info, or NO otherwise
PROFILE        := NO

# TINYXML_USE_STL can be used to turn on STL support. NO, then STL
# will not be used. YES will include the STL files.
TINYXML_USE_STL := NO

#****************************************************************************
#CROSS  := arm-none-linux-gnueabi-
CC     := $(CROSS)gcc
CXX    := $(CROSS)g++
LD     := $(CROSS)g++
AR     := $(CROSS)ar rc
RANLIB := ranlib

DEBUG_CFLAGS     := -fpic -Wall -Wno-format -g -D_DEBUG
RELEASE_CFLAGS   := -fpic -Wall -Wno-unknown-pragmas -Wno-format -O3

LIBS		 :=

DEBUG_CXXFLAGS   := ${DEBUG_CFLAGS} 
RELEASE_CXXFLAGS := ${RELEASE_CFLAGS}

DEBUG_LDFLAGS    := -g
RELEASE_LDFLAGS  :=

ifeq (YES, ${DEBUG})
   CFLAGS       := ${DEBUG_CFLAGS}
   CXXFLAGS     := ${DEBUG_CXXFLAGS}
   LDFLAGS      := ${DEBUG_LDFLAGS}
else
   CFLAGS       := ${RELEASE_CFLAGS}
   CXXFLAGS     := ${RELEASE_CXXFLAGS}
   LDFLAGS      := ${RELEASE_LDFLAGS}
endif

ifeq (YES, ${PROFILE})
   CFLAGS   := ${CFLAGS} -pg -O3
   CXXFLAGS := ${CXXFLAGS} -pg -O3
   LDFLAGS  := ${LDFLAGS} -pg
endif

#****************************************************************************
# Preprocessor directives
#****************************************************************************

ifeq (YES, ${TINYXML_USE_STL})
  DEFS := -DTIXML_USE_STL
else
  DEFS :=
endif

#****************************************************************************
# Include paths
#****************************************************************************

#INCS := -I/usr/include/g++-2 -I/usr/local/include
INCS :=./


#****************************************************************************
# Makefile code common to all platforms
#****************************************************************************

CFLAGS   := ${CFLAGS}   ${DEFS}
CXXFLAGS := ${CXXFLAGS} ${DEFS}

#****************************************************************************
# Targets of the build
#****************************************************************************

LIB := libtixmlconfig.so 
DEMO :=demo_app

all:${LIB}
# ${DEMO}
#${LIB}
#****************************************************************************
# Output
#****************************************************************************

${LIB}: tixmlmgr.o
	${LD} -shared -o $@ tixmlmgr.o -L.-ltixml
#	rm -f libtinyxmlmgr.so
#	ln -s libtinyxmlmgr.so.1.0	libtinyxmlmgr.so

${DEMO}:./Demo/demo_app.c
	${CC}	./Demo/demo_app.c -o ${INCS} $@ -L.-ltixmlconfig -L.-ltixml 

#****************************************************************************
# common rules
#****************************************************************************

# Rules for compiling source files to object files
%.o : %.cpp
	${CXX} -c ${CXXFLAGS} ${INCS} $< -o $@

%.o : %.c
	${CC} -c ${CFLAGS} ${INCS} $< -o $@

dist:
	bash makedistlinux

clean:
	rm -f *.o ${DEMO} ${LIB}
	rm -f *.so
install:
	cp ./lib* ./../../../../lib/
	cp ./lib* ./../../../../bin/
depend:
	#makedepend ${INCS} ${SRCS}

tixmlmgr.o: tixmlconfig.h
demo_app.o: tixmlconfig.h
