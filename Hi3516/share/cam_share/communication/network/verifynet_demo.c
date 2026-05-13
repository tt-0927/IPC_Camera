
#include <stdio.h>
#include "verifynet.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>


// int main(int argc,char *argv[])
// {
// 	int ret = 0;
// 	unsigned int netip = 0;
// 	char ip[16] = {0};

// 	netip = inet_addr(argv[1]);
// 	printf("ip[%s] netip[0x%x] 0x%x\n",argv[1],netip,inet_addr(argv[1]));

// 	ret = netIpIsValid(argv[1]);
// 	printf("ip result = %d\n",ret);

// 	printf("mask result = %d\n",netMaskIsValid(argv[2]));

// 	printf("ip and mask result = %d\n",netMaskAndIpIsValid(argv[1],argv[2]));

// 	return 0;
// }





