#include "db_middle.h"
#include "share_port.h"
#include "dlog.h"
#include "share_dbBase.h"
#include "share_os.h"
#include "share_socket.h"
#include "xml_base.h"
#include <linux/rtc.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

char *His_get_UserInfo(char *UserName)
{
	DBUser_Communicate_t recv_info;
	memset(&recv_info, 0, sizeof(DBUser_Communicate_t));
	recv_info.from = 0;
	recv_info.to = 10;
	recv_info.mode = 0;
	recv_info.CurPage = 1;
	recv_info.pageSize = 100; // 10
	char *xmlBuf = NULL;
	strcpy(recv_info.find, UserName);
	xmlBuf = His_management_finduserAll(UserName, recv_info, DB_FIND_USER_PASSWD);
	if (xmlBuf == NULL)
	{
		return NULL;
	}
	return xmlBuf;
}

char *His_management_finduser(char *userName, int code)
{
	char *xmlBuf = NULL;
	char passwd[64] = {0};
	xmlBuf = His_get_UserInfo(userName);
	if (xmlBuf == NULL)
	{
		return NULL;
	}
	if (xml_get_charNode1("/RequestMsg/MsgBody/UserInfo/Passwd/", passwd, xmlBuf, sizeof(passwd)) == FALSE)
	{
		dlog(LOG_ERROR, "/RequestMsg/MsgBody/UserInfo/Passwd/ is error\n");
		return NULL;
	}
	memset(xmlBuf, 0, strlen(xmlBuf) + 1);
	memcpy(xmlBuf, passwd, strlen(passwd));
	return xmlBuf;
}

int His_management_findGroup(char *userName, int code)
{
	char *xmlBuf = NULL;
	int group = 0;
	xmlBuf = His_get_UserInfo(userName);
	if (xmlBuf == NULL)
	{
		return 0;
	}
	if (xml_get_intNode1("/RequestMsg/MsgBody/UserInfo/Reserve2/", &group, xmlBuf) == FALSE)
	{
		dlog(LOG_ERROR, "/RequestMsg/MsgBody/UserInfo/Reserve2/ is error\n");
		return 0;
	}
	r_free(xmlBuf);
	return group;
}

char *His_management_finduserAll(char *userName, DBUser_Communicate_t userInfo, int code)
{
	DB_Communicate_Head head;
	DBUser_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	int mode = 0;
	char *xml_buf = NULL;
	int head_len = sizeof(DB_Communicate_Head);
	int send_len = 0;
	int recv_len = 0;
	if (!userName)
	{
		dlog(LOG_ERROR, "name=%p\n", userName);
		return NULL;
	}
	if (sock_fd <= 0)
	{
		sock_fd = DB_connect_server(DB_USER_SERVER_PORT);
	}
	if (sock_fd <= 0)
	{
		return NULL;
	}
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DBUser_Communicate_t);
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
	send_len = sizeof(DBUser_Communicate_t);
	memset(&load, 0, send_len);
	load.mode = userInfo.mode; // 1-userName
	load.from = userInfo.from;
	load.to = userInfo.to;
	load.pageSize = userInfo.pageSize;
	load.CurPage = userInfo.CurPage;
	if (strlen(userName) != 0)
	{
		snprintf(load.find, sizeof(load.find), "%s", userName);
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

int DBUser_modify_elem(int mode, char *modify, char *userName)
{
	DB_Communicate_Head head;
	DBUser_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	int head_len = sizeof(DB_Communicate_Head);
	int recv_len = 0;
	int send_len = 0;
	if (mode < 0 || !userName || mode > 14 || !modify)
	{
		dlog(LOG_ERROR, "name=%p,mode=%d,%p\n", userName, mode, modify);
		return -1;
	}
	if (sock_fd == 0)
	{
		sock_fd = DB_connect_server(DB_USER_SERVER_PORT);
	}
	if (sock_fd <= 0)
	{
		return -1;
	}
	memset(&load, 0, sizeof(DBUser_Communicate_t));
	load.mode = mode;
	snprintf(load.modify, sizeof(load.modify), "%s", modify);
	snprintf(load.item.UserName, sizeof(load.item.UserName), "%s", userName);
	//拼装head
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DBUser_Communicate_t);
	head.msg_code = DB_MODIFY_USER_MSG;
	head.msg_result = 0;
	head.msg_type = 0;

	//发送head
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);
	if (ret < 0)
	{
		dlog(LOG_ERROR, "failed to send errpr=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}
	//发送需要处理的Load
	send_len = sizeof(DBUser_Communicate_t);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);
	if (ret < 0)
	{
		dlog(LOG_ERROR, "failed to send error =%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}
	//接收处理后的Head
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);
	if (recv_len != head_len || ret < 0)
	{
		dlog(LOG_ERROR, "failed to send errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}
	//检验处理结果
	if (1 != head.msg_result)
	{
		dlog(LOG_WARN, "DELETE failed errno=%d\n", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	RH_Close(__FILE__, (char *)__func__, sock_fd);
	return 0;
}

/*==============================================================================
	函数: <DB_insert_elem>
	功能: 插入新的elem
==============================================================================*/
int DBUser_insert_elem(DataBaseUser_t DbUserInfo)
{
	DB_Communicate_Head head;
	DBUser_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	int head_len = sizeof(DB_Communicate_Head);
	int send_len = 0;
	int recv_len = 0;
	if (strlen(DbUserInfo.Passwd) < 5)
	{
		dlog(LOG_ERROR, "DbUserInfo.UserName = %s,DbUserInfo.Passwd=%s\n\n", DbUserInfo.UserName, DbUserInfo.Passwd);
		DEBUG_INFO("DbUserInfo.UserName = %s,DbUserInfo.Passwd=%s", DbUserInfo.UserName, DbUserInfo.Passwd);
		return -1;
	}

	if (sock_fd == 0)
	{
		sock_fd = DB_connect_server(DB_USER_SERVER_PORT);
	}

	if (sock_fd <= 0)
	{
		DEBUG_INFO();
		return -1;
	}
	memset(&load, 0, sizeof(DBUser_Communicate_t));
	snprintf(load.item.UserName, sizeof(load.item.UserName), "%s", DbUserInfo.UserName);
	snprintf(load.item.Passwd, sizeof(load.item.Passwd), "%s", DbUserInfo.Passwd);
	load.item.Jurisdiction_all = DbUserInfo.Jurisdiction_all;
	load.item.Jurisdiction_record = DbUserInfo.Jurisdiction_record;
	load.item.Jurisdiction_file = DbUserInfo.Jurisdiction_file;
	load.item.Jurisdiction_upgrade = DbUserInfo.Jurisdiction_upgrade;
	load.item.Jurisdiction_director = DbUserInfo.Jurisdiction_director;
	snprintf(load.item.Remarks, sizeof(load.item.Remarks), "%s", DbUserInfo.Remarks);
	load.item.Reserve1 = DbUserInfo.Reserve1;
	load.item.Reserve2 = DbUserInfo.Reserve2;
	load.item.Reserve3 = DbUserInfo.Reserve3;
	snprintf(load.item.Reserve4, sizeof(load.item.Reserve4), "%s", DbUserInfo.Reserve4);
	snprintf(load.item.Reserve5, sizeof(load.item.Reserve5), "%s", DbUserInfo.Reserve5);
	snprintf(load.item.Reserve6, sizeof(load.item.Reserve6), "%s", DbUserInfo.Reserve6);
	// load.item.RcdTimeLenght = time_lenght;
	/* 拼装Head */
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DBUser_Communicate_t);
	head.msg_code = DB_INERST_USER_MSG;
	head.msg_result = 0;
	head.msg_type = 0;
	/*发送Head*/
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);

	if (ret < 0)
	{
		dlog(LOG_ERROR, "Failed to send !errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}
	/*发送需要处理的load*/
	send_len = sizeof(DBUser_Communicate_t);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);
	if (ret < 0)
	{
		dlog(LOG_ERROR, "Failed to send !errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}
	/*接收处理后的head*/
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);
	if (recv_len != head_len || ret < 0)
	{
		dlog(LOG_ERROR, "Failed to send !errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}
	/*检验处理结果*/
	if (1 != head.msg_result)
	{
		dlog(LOG_WARN, "insert failed!!!errno=%d\n", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}
    dlog(LOG_TRACE, "the head.msg_result is...... :[%d]", head.msg_result);
	RH_Close(__FILE__, (char *)__func__, sock_fd);
	return 0;
}

int DBUser_delete_elem(char *UserName)
{
	DB_Communicate_Head head;
	DBUser_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	int head_len = sizeof(DB_Communicate_Head);
	int send_len = 0;
	int recv_len = 0;

	if (!UserName)
	{
		dlog(LOG_ERROR, "UserName=%p\n", UserName);
		return -1;
	}

	if (sock_fd == 0)
	{
		sock_fd = DB_connect_server(DB_USER_SERVER_PORT);
	}

	if (sock_fd <= 0)
	{
		return -1;
	}

	/* 拼装Head */
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DBUser_Communicate_t);
	head.msg_code = DB_DELETE_USER_MSG;
	head.msg_result = 0;
	head.msg_type = 0;
	/*发送Head*/
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);

	if (ret < 0)
	{
		dlog(LOG_ERROR, "Failed to send !errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	/*发送load*/
	send_len = sizeof(DBUser_Communicate_t);
	memset(&load, 0, sizeof(DBUser_Communicate_t));
	snprintf(load.item.UserName, sizeof(load.item.UserName), "%s", UserName);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);

	if (ret < 0)
	{
		dlog(LOG_ERROR, "Failed to send !errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	/*接收处理后的head*/
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);

	if (recv_len != head_len || ret < 0)
	{
		dlog(LOG_ERROR, "Failed to send !errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}
	/*检验处理结果*/
	if (1 != head.msg_result)
	{
		dlog(LOG_WARN, "DELETE failed!!!errno=%d\n", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}
	RH_Close(__FILE__, (char *)__func__, sock_fd);
	return 0;
}