#include "db_middle.h"
#include "share_port.h"
#include "dlog.h"
#include "share_os.h"
#include "share_socket.h"
#include "xml_base.h"
#include <linux/rtc.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static int DBLog_connect_server()
{
	struct sockaddr_in serv_addr;
	const char *pAddr = LOCAL_IP;
	int sock_fd;

	sock_fd = socket(PF_INET, SOCK_STREAM, 0);

	if (sock_fd < 1)
	{
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(DB_LOG_SERVER_PORT);
	inet_aton(pAddr, (struct in_addr *)&serv_addr.sin_addr);
	bzero(&(serv_addr.sin_zero), 8);

	if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1)
	{
		RH_Close(__FILE__, (char *)__func__, sock_fd);
        dlog(LOG_ERROR, "connect DB server failed!!");
		return -1;
	}

	return sock_fd;
}

char *His_management_findLog(char *logSelect, DBLog_Communicate_t logInfo, int code)
{
	DB_Communicate_Head head;
	DBLog_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	int mode = 0;
	char *xml_buf = NULL;
	int head_len = sizeof(DB_Communicate_Head);
	int send_len = 0;
	int recv_len = 0;
	if (!logSelect)
	{
		dlog(LOG_ERROR, "name=%p\n", logSelect);
		return NULL;
	}
	if (sock_fd <= 0)
	{
		sock_fd = DBLog_connect_server();
	}
	if (sock_fd <= 0)
	{
		return NULL;
	}
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DBLog_Communicate_t);
	head.msg_code = code;
	if (mode <= 0)
	{
		mode = 1;
	}
	head.msg_result = 0;
	head.msg_type = 0;
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);
	if (ret < 0)
	{
		dlog(LOG_ERROR, "Failed to send errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return NULL;
	}
	send_len = sizeof(DBLog_Communicate_t);
	memset(&load, 0, send_len);
	load.mode = logInfo.mode; // 1-userName
	load.from = logInfo.from;
	load.to = logInfo.to;
	load.pageSize = logInfo.pageSize;
	load.CurPage = logInfo.CurPage;
	if (strlen(logSelect) != 0)
	{
		snprintf(load.find, sizeof(load.find), "%s", logSelect);
	}
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);
	if (ret < 0)
	{
		dlog(LOG_ERROR, "Failed to send errno=%d,%s.\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return NULL;
	}
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);
	if (recv_len != head_len || ret < 0)
	{
		dlog(LOG_ERROR, "Failed to send errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return NULL;
	}
	dlog(LOG_DEBUG, "head.load_len=%d\n", head.load_len);
	if (1 != head.msg_result || head.load_len <= 0)
	{
		dlog(LOG_WARN, "Array failed errno =%d\n", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return NULL;
	}
	//获取返回的xml
	xml_buf = (char *)malloc(head.load_len + 1);
	if (!xml_buf)
	{
		dlog(LOG_WARN, "Array failed errno=%d,xml_buf=%p\n", head.msg_result, xml_buf);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return NULL;
	}
	recv_len = 0;
	memset(xml_buf, 0, head.load_len + 1);
	ret = RH_TcpRcvBlockFd(sock_fd, xml_buf, head.load_len, &recv_len);
	if (recv_len != head.load_len || ret < 0)
	{
		dlog(LOG_ERROR, "Failed to send errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		r_free(xml_buf);
		return NULL;
	}
	RH_Close(__FILE__, (char *)__func__, sock_fd);
	DEBUG_INFO("%s", xml_buf);
	//返回获取成功的xml
	return xml_buf;
}
