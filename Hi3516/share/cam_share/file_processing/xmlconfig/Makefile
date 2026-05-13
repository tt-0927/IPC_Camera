#CROSS=
#CROSS=arm-none-linux-gnueabi-
#include ../../../build/head.mk
all:
	@cd tinyxml;	make CROSS?=arm-linux-gnueabihf-
	@cd tinyxml;	make install
	@cd tinyxmlmgr;	make CROSS?=arm-linux-gnueabihf-
	@cd tinyxmlmgr;	make install
clean:
	@cd tinyxml;  make  clean
	@cd tinyxmlmgr;       make  clean
