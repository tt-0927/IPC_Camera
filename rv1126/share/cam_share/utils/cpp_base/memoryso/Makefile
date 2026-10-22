#Set cxx
CPLUSPLUS_COMPILER=/opt/ssg-linux/linux-x86/ssg359G/gcc-sigmastar-9.1.0-2020.07-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++
#Set install path and moudle name
MODULENAME=../../../lib/libMemoryCheck.so

LDFLAGS =-shared

MEDIASERVERSRC=$(wildcard ./*.cpp)
CXXOBJS=$(patsubst %.cpp,%.o,$(MEDIASERVERSRC))

CXXOBJTARGET=$(CXXOBJS):%.o:%.cpp

COBJS=$(patsubst %.c,%.o,$(CCMEDIASERVERSRC))
COBJTARGET=$(COBJS):%.o:%.c

OBJS=$(COBJS) $(CXXOBJS)

INCLUDES=
LIB=-lpthread
ifeq ($(CHECKDEBUG),1)
	DEFINE+=-DCHECKDEBUG
else
endif


all:$(OBJS)
	$(CPLUSPLUS_COMPILER) $(LDFLAGS) -o $(MODULENAME) $^ $(LIB)
$(CXXOBJTARGET)
	$(CPLUSPLUS_COMPILER) $(DEFINE) $(CPLUSPLUS_FLAGS) $<  -c -fPIC -o $@ $(INCLUDES)
	
 
clean:
	rm -f $(MODULENAME) 
	rm -f $(OBJS)
