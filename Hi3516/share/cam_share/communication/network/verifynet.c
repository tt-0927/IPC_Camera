


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
#include <ctype.h>

#include "verifynet.h"


#ifdef WIN32
#pragma comment(lib,"wsock32.lib")
#endif

/*参数UINT32都为网络字节顺序。*/

/*IP地址是否合法, 合法返回TURE，失败返回FALSE*/
int netIpIsValid(const char *ip)
{
	char* ptr;
	int count = 0;
	char str[16] = {0};

	memcpy(str,ip,sizeof(str));
	const char *p = str;

	//1、判断是不是三个 ‘.’
	//2、判断是不是先导0
	//3、判断是不是四部分数
	//4、第一个数不能为0

	while(*p != '\0')
	{
		if(*p == '.')
		count++;
		p++;
	}

	if(count != 3)
	{
		return FALSE;
	}

	count = 0;
	ptr = strtok(str,".");
	while(ptr != NULL)
	{
		count++;
		if(ptr[0] == '0' && isdigit(ptr[1]))
		{
			return FALSE;
		}

		int a = atoi(ptr);
		if(count == 1 && a == 0)
		{
			return FALSE;
		}

		if((a < 0) || (a > 255))
		{
			return FALSE;
		}

		ptr = strtok(NULL,".");
	}

	if(count == 4)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}

/*MASK子网掩码是否合法, 合法返回TURE，失败返回FALSE*/
int netMaskIsValid(char *mask)
{
	int i;
	unsigned long ii;
	i = netIpIsValid(mask);
	if(i == TRUE)
	{
		ii = ntohl(inet_addr(mask));
		if((ii | (ii-1)) == 0xffffffff)
		{
			//1和0的位都一定是全部都要连续的才是有效的子网掩码。
			return TRUE;	//合法的子网掩码
		}
	}

	return FALSE;
}

/*MASK子网掩码是否合法, 合法返回TURE，失败返回FALSE*/
int netMaskAndIpIsValid(char *str, char *maskstr)
{
	int i = 0;
	int a = 0, b = 0, c = 0;
	UINT32 IP;
	UINT32 mask;

	i = netIpIsValid(str);
	if(i != TRUE)
	{
		return FALSE;
	}

	i = netMaskIsValid(maskstr);
	if(i != TRUE)
	{
		return FALSE;
	}

	IP = inet_addr(str);
	mask = inet_addr(maskstr);
	a = IP&0x000000ff;
	b = ntohl(mask);

//	printf("ip[0x%x] mask[0x%x]\n",IP,mask);
//	printf("a[0x%x] b[0x%x]\n",a,b);

	/*首先与默认子网掩码比较*/
	if((a > 0) && (a < 127))
	{
		if(mask < 0x000000ff)
		{

			return FALSE;
		}
		if(mask > 0x000000ff)
		{
			b -= 0xff000000;
		}
	}

	if((a >= 128) && (a <= 191))
	{
		if(mask < 0x0000ffff)
		{
			return FALSE;
		}
		if(mask > 0x0000ffff)
		{
			b -= 0xffff0000;
		}
	}

	if((a >= 192) && (a <= 223))
	{
		if(mask < 0x00ffffff)
		{
			return FALSE;
		}
		if(mask>0x00ffffff)
		{
			b -= 0xffffff00;
		}
	}

	/*每个子网段的第一个是网络地址,用来标志这个网络,最后一个是广播地址,用来代表这个网络上的所有主机.这两个IP地址被TCP/IP保留,不可分配给主机使用.*/
	c = ~ntohl(mask) & ntohl(IP);
	if(c == 0 || c == ~ntohl(mask))
	{
		return FALSE;
	}

	/*RFC 1009中规定划分子网时，子网号不能全为0或1，会导致IP地址的二义性*/
	if(b > 0)
	{
		c = b & (ntohl(IP));
		if((c == 0) || (c == b))
		{
			return FALSE;
		}
	}

	return TRUE;
}

/*测试主网和子网是否匹配，也可测试两个主机IP是否在同一网段内*/
int netIPAndSubnetValid(char *str, char *substr, char *maskstr)
{
	int i;
	int addr1, addr2;
	UINT32 IP;
	UINT32 subIP;
	UINT32 mask;

	i = netMaskAndIpIsValid(str, maskstr);
	if(i != TRUE)
	{
		return FALSE;
	}

	i = netMaskAndIpIsValid(substr, maskstr);
	if(i != TRUE)
	{
		return FALSE;
	}

	IP = inet_addr(str);
	subIP = inet_addr(substr);
	mask = inet_addr(maskstr);

	addr1 = IP&mask;
	addr2 = subIP&mask;

	if(addr1 != addr2)
	{
		return FALSE;
	}

	return TRUE;
}

/*测试ipv6是否合法*/
int net_is_validipv6(char *pIpv6Addr)
{
	if (!pIpv6Addr) return FALSE;

	struct sockaddr_in6 addr;
	char aPtr[256]={0};
	int i;
	memcpy(aPtr, pIpv6Addr, strlen(pIpv6Addr));

	for(i=0; i<strlen(aPtr); i++)
	{
		if(aPtr[i]=='/')
		{
			//aPtr[i]='\0';
			memset(aPtr+i, '\0', strlen(aPtr)-i);
			break;
		}
	}
	//去掉换行符
	if(aPtr[strlen(aPtr)-1]==0x0a)
		aPtr[strlen(aPtr)-1]='\0';


	//printf("check ipv6 %s\n",aPtr);

	if (strchr(aPtr, '.')) return FALSE;//暂时排除::ffff:204.152.189.116
	if (inet_pton(AF_INET6, aPtr, &addr.sin6_addr) != 1)
	{
		printf("aPtr error %s\n",aPtr);
		return FALSE;
	}

	return TRUE;
}












