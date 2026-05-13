##### Change the following for your environment:
# CROSS_COMPILE?=		/opt/ssg-linux/linux-x86/ssg359G/gcc-sigmastar-9.1.0-2020.07-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
# COMPILE_OPTS =		$(INCLUDES) -I.  -DSOCKLEN_T=socklen_t -DNO_SSTREAM=1 -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -DALLOW_RTSP_SERVER_PORT_REUSE -DNO_OPENSSL=1
# C =			c
# C_COMPILER =		$(CROSS_COMPILE)gcc
# C_FLAGS =		$(COMPILE_OPTS) -O3 -fPIC
# CPP =			cpp
# CPLUSPLUS_COMPILER =	$(CROSS_COMPILE)g++
# CPLUSPLUS_FLAGS =	$(COMPILE_OPTS) -Wall  -O3 -DBSD=1 -DXLOCALE_NOT_USED -fPIC
# OBJ =			o
# LINK =			$(CROSS_COMPILE)g++ -o
# LINK_OPTS =
# CONSOLE_LINK_OPTS =	$(LINK_OPTS)
# LIBRARY_LINK =		$(CROSS_COMPILE)ar cr 
# LIBRARY_LINK_OPTS =	$(LINK_OPTS)
# LIB_SUFFIX =			a
# LIBS_FOR_CONSOLE_APPLICATION =
# LIBS_FOR_GUI_APPLICATION =
# EXE =
# arm=1
##### End of variables to change

LIVEMEDIA_DIR = liveMedia
GROUPSOCK_DIR = groupsock
USAGE_ENVIRONMENT_DIR = UsageEnvironment
BASIC_USAGE_ENVIRONMENT_DIR = BasicUsageEnvironment
MEDIA_SERVER_DIR = mediaServer
all:
	cd $(LIVEMEDIA_DIR) ; $(MAKE)
	cd $(GROUPSOCK_DIR) ; $(MAKE)
	cd $(USAGE_ENVIRONMENT_DIR) ; $(MAKE)
	cd $(BASIC_USAGE_ENVIRONMENT_DIR) ; $(MAKE)
	cd $(MEDIA_SERVER_DIR) ; $(MAKE)

install:
	cd $(LIVEMEDIA_DIR) ; $(MAKE) install
	cd $(GROUPSOCK_DIR) ; $(MAKE) install
	cd $(USAGE_ENVIRONMENT_DIR) ; $(MAKE) install
	cd $(BASIC_USAGE_ENVIRONMENT_DIR) ; $(MAKE) install
	# cd $(MEDIA_SERVER_DIR) ; $(MAKE) install

clean:
	cd $(LIVEMEDIA_DIR) ; $(MAKE) clean
	cd $(GROUPSOCK_DIR) ; $(MAKE) clean
	cd $(USAGE_ENVIRONMENT_DIR) ; $(MAKE) clean
	cd $(BASIC_USAGE_ENVIRONMENT_DIR) ; $(MAKE) clean
	cd $(MEDIA_SERVER_DIR) ; $(MAKE) clean


