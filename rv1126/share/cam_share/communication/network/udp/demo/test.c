/*************************************************************************
@File Name: test.c
@Author: luoyongkang
@Created Time: 2021骞?04鏈?08鏃? 鏄熸湡鍥? 15鏃?19鍒?31绉?
************************************************************************/
#include"os_network_multicast.h"
#include <stdlib.h>
#include<stdio.h>

/*鎺ュ彛娴嬭瘯*/
NetworkMulticast_S network_info;

void* data_handle(void* recv_info1)
{
	    UserRecv_S* recv_info =(UserRecv_S*)recv_info1;
		printf("鏁版嵁:");
		int i=0;
        while(i < recv_info->lenght)
			printf("%x ",recv_info->data[i++]);

		printf("%d.%d.%d.%d\n",recv_info->data[0],recv_info->data[1],recv_info->data[2],recv_info->data[3]);
	//	os_networkmulticast_send(recv_info->data, strlen(recv_info->data), &network_info);
	//	os_networkunque_send(recv_info->data, strlen(recv_info->data), &network_info);
	//	os_networkunque_ip_send(recv_info->data, strlen(recv_info->data), &network_info,"172.16.18.207", 8803);

}


int main()
{
	char val = 0x9a;

	for(int i = 7; i >= 0; i--)
	{

		if(val & (1 << i))
			printf("1\n");
		else
			printf("0\n");

		if((val & (1 << i))== 0)
		{
			val += 1 << i;
			printf("%02X\n",val);
			break;
		}	

		
	}



	char data[128] = {"aaaaa/bbbbbb/ccccccc"};
	char find = '/';

	char *finddata = strrchr(data,find);

	char onedata[64] = {0},twodata[64] = {0};

	snprintf(onedata,strlen(data)-strlen(finddata),"%s",data);



	memset(&network_info, 0 ,sizeof(network_info));

	network_info.entype=MULTICAST;
	//network_info.entype=UNICAST;
	//strcpy(network_info.stsrc_ip_info.ip, "172.16.18.71");
	network_info.stsrc_ip_info.port=20101;
	
	//strcpy(network_info.stdst_ip_info.ip,"172.16.18.244");
	//network_info.stdst_ip_info.port=8803;
	
	network_info.fnhandledata=data_handle;
	strcpy(network_info.amcast_ip,"224.0.0.1");

	os_networkmulticast_init(&network_info);

	char aaaa[4] = {0};
	
	aaaa[0] = 172;
	aaaa[1] = 16;
	aaaa[2] = 18;
	aaaa[3] = 177;

	//os_networkmulticast_send("1234567890", 10, &network_info);
	while(1)
	{
		//ReachGetIPaddrchar("eth0",aaaa);


		os_networkmulticast_send(aaaa,strlen(aaaa),&network_info);
		sleep(2);
	}
	return 0;
}
