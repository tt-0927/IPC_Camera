#include "share_dbBase.h"



int DB_connect_server(int port)
{
	struct sockaddr_in serv_addr;
	const char *pAddr  = LOCAL_IP;
	int sock_fd;

	sock_fd = socket(PF_INET, SOCK_STREAM, 0);

	if(sock_fd < 1) {
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);//;htons(DB_SERVER_PORT);
	inet_aton(pAddr, (struct in_addr *)&serv_addr.sin_addr);
	bzero(&(serv_addr.sin_zero), 8);

	if(connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1)	{
		RH_Close(__FILE__, (char *)__func__, sock_fd);
        dlog(LOG_ERROR, "connect DB server failed!!");
		return -1;
	}

	return sock_fd;
}

