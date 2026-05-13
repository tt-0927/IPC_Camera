#include "share_dbBase.h"

#define DB_CLIENT_DEBUG 0

char *DB_array_RcdStartTime(char *queryName,
							int order,
							int curPage,
							int pageSize, int from, int to)
{
    dlog(LOG_TRACE, "start time");
	return NULL;
}

/*==============================================================================
	函数: <DB_array_FileSize>
	功能: 以FileSize排序
	参数:p_xml :-返回排序后的xml,使用后需要销毁
	返回值:0-成功   否则失败
==============================================================================*/
char *DB_array_FileSize(char *queryName,
						int order,
						int curPage,
						int pageSize, int from, int to)
{
	DB_Communicate_Head head;
	DB_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	char *xml_buf = NULL;
	int head_len = sizeof(DB_Communicate_Head);
	int recv_len = 0;
	int send_len = 0;

	if (sock_fd == 0)
	{
		sock_fd = DB_connect_server(DB_SERVER_PORT);
	}

	if (sock_fd <= 0)
	{
		return NULL;
	}

#if DB_CLIENT_DEBUG
    dlog(LOG_TRACE, "[%s],---sock_fd=%d", __func__, sock_fd);
#endif
	/*发送接收好的head*/
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = 0;
	head.msg_code = DB_ORDER_FILELIST_MSG;
	head.msg_result = 0;
	head.msg_type = 0;
	send_len = sizeof(DB_Communicate_Head);

	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "0:[%s],Failed to send !errno=%d<%s>", __func__, errno, strerror(errno));
#endif
		return NULL;
	}

	/*发送load*/
	memset(&load, 0, sizeof(DB_Communicate_t));
	load.mode = order; /*以文件大小排序*/
	load.to = to;
	load.from = from;
	load.CurPage = curPage;
	load.pageSize = pageSize;
	load.order = order;
	snprintf(load.find, sizeof(load.find), "%s", queryName);
	send_len = sizeof(DB_Communicate_t);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "01:[%s],Failed to send !errno=%d<%s>", __func__, errno, strerror(errno));
#endif
		return NULL;
	}

	/*接收处理后的head*/
	memset(&head, 0, sizeof(head));
	recv_len = 0;
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);

	if (recv_len != head_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "[%s]1:Failed to recv,recv_len=%d==(%d),ret=%d !errno=%d<%s>", __func__, recv_len, head_len, ret, errno, strerror(errno));
#endif
		return NULL;
	}

	/*检验处理信息*/
	if (1 != head.msg_result || head.load_len <= 0)
	{
        dlog(LOG_WARN, "Array failed!!!errno=%d,load_len=%d", head.msg_result, head.load_len);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "2:Array failed!!!errno=%d,load_len=%d", head.msg_result, head.load_len);
#endif
		return NULL;
	}

	/*接收处理的结果*/
	xml_buf = (char *)malloc(head.load_len + 1);

	if (!xml_buf)
	{
        dlog(LOG_WARN, "Array failed!!!errno=%d,xml_buf=%p", head.msg_result, xml_buf);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "2:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return NULL;
	}

	memset(xml_buf, 0, head.load_len + 1);
	recv_len = 0;
	ret = RH_TcpRcvBlockFd(sock_fd, xml_buf, head.load_len, &recv_len);

	if (recv_len != head.load_len || ret < 0)
	{
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "3:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		r_free(xml_buf);
		return NULL;
	}

	RH_Close(__FILE__, (char *)__func__, sock_fd);
	/*返回buffer*/
	return xml_buf;
}

/*==============================================================================
	函数: <DB_find_FileName>
	功能: 查找name
	参数:queryName :-查找内容
			   order  :-查找模式	，取值如下:
*	1-file name
*	2-file size
*	3-StartTime
*	4-notes
*	5-Record Time
*	6-CourseTeacher
*	7-Record Time
			   curPage:-传入页码
			   pageSize:页码大小
			   to:获取终止
			   from:获取起始
	返回值:null 则失败，否则成功
==============================================================================*/
char *DB_find_FileName(
	char *queryName,
	int mode,
	int curPage,
	int pageSize, int to, int from)
{
	DB_Communicate_Head head;
	DB_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	char *xml_buf = NULL;
	int head_len = sizeof(DB_Communicate_Head);
	int send_len = 0;
	int recv_len = 0;

	if (!queryName)
	{
        dlog(LOG_ERROR, "name=%p", queryName);
		return NULL;
	}

	if (sock_fd == 0)
	{
		sock_fd = DB_connect_server(DB_SERVER_PORT);
	}

	if (sock_fd <= 0)
	{
		return NULL;
	}

	/*拼装head*/
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DB_Communicate_t);

	head.msg_code = DB_ORDER_FILELIST_MSG; //获取文件列表
	if (mode <= 0)
	{
		mode = 1;
	}
	head.msg_result = 0;
	head.msg_type = 0;

	/*发送head*/
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "0:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return NULL;
	}

	/*发送load*/
	send_len = sizeof(DB_Communicate_t);
	memset(&load, 0, sizeof(DB_Communicate_t));
	load.mode = mode; // 1-FileName
	load.from = from;
	load.to = to;
	load.order = mode;
	load.pageSize = pageSize;
	load.CurPage = curPage;

	if (strlen(queryName) != 0)
	{
		snprintf(load.find, sizeof(load.find), "%s", queryName);
	}
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "1:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return NULL;
	}
	/*接收Head*/
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);
	if (recv_len != head_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "[%s]2:Failed to recv,recv_len=%d(==%d) !errno=%d<%s>", __func__, recv_len, head_len, errno, strerror(errno));
#endif
		return NULL;
	}
    dlog(LOG_DEBUG, "head.load_len=%d", head.load_len);
	/*检验头信息 */
	if (1 != head.msg_result || head.load_len <= 0)
	{
        dlog(LOG_WARN, "Array failed!!!errno=%d", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return NULL;
	}
	/*获取返回的xml*/
	xml_buf = (char *)malloc(head.load_len + 1);

	if (!xml_buf)
	{
        dlog(LOG_WARN, "Array failed!!!errno=%d,xml_buf=%p", head.msg_result, xml_buf);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "3:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return NULL;
	}
	recv_len = 0;
	memset(xml_buf, 0, head.load_len + 1);
	ret = RH_TcpRcvBlockFd(sock_fd, xml_buf, head.load_len, &recv_len);
	if (recv_len != head.load_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "4:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		r_free(xml_buf);
		return NULL;
	}
	RH_Close(__FILE__, (char *)__func__, sock_fd);
	/*返回获取成功的xml buffer*/
	return xml_buf;
}

/*==============================================================================
	函数: <DB_find_FileInfo>
	功能: 查找name
	参数: stQueryInfo:查询信息结构体
			   to:获取终止
			   from:获取起始
	返回值:null 则失败，否则成功
==============================================================================*/
char *DB_find_FileInfo(Ftp_QueryFile_t stQueryInfo, int to, int from)
{
	DB_Communicate_Head head;
	DB_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	char *xml_buf = NULL;
	int head_len = sizeof(DB_Communicate_Head);
	int send_len = 0;
	int recv_len = 0;

	if (!stQueryInfo.query_name)
	{
        dlog(LOG_ERROR, "stQueryInfo.query_name=%p", stQueryInfo.query_name);
		return NULL;
	}

	if (sock_fd == 0)
	{
		sock_fd = DB_connect_server(DB_SERVER_PORT);
	}

	if (sock_fd <= 0)
	{
		return NULL;
	}

	/*拼装head*/
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DB_Communicate_t);

	head.msg_code = DB_SELECT_INGO_FILE; //获取文件列表
	if (strlen(stQueryInfo.RecordTime) != 0 && strlen(stQueryInfo.RecordEndTime) != 0)
	{
		head.msg_code = DB_SEARCH_TIME_MSG; //按时间段查找
	}
	if (stQueryInfo.order <= 0)
	{
		stQueryInfo.order = 1;
	}
	head.msg_result = 0;
	head.msg_type = 0;

	/*发送head*/
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "0:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return NULL;
	}

	/*发送load*/
	send_len = sizeof(DB_Communicate_t);
	memset(&load, 0, sizeof(DB_Communicate_t));
	load.mode = stQueryInfo.infoType; // 1-FileName
	load.from = from;
	load.to = to;
	load.order = stQueryInfo.order;
	load.pageSize = stQueryInfo.page_size;
	load.CurPage = stQueryInfo.cur_page;
	load.flags = stQueryInfo.flags;
	if (strlen(stQueryInfo.RecordTime) != 0)
	{
		snprintf(load.recordtime, sizeof(load.recordtime), "%s", stQueryInfo.RecordTime);
	}
	if (strlen(stQueryInfo.query_name) != 0)
	{
		snprintf(load.find, sizeof(load.find), "%s", stQueryInfo.query_name);
	}
	if (strlen(stQueryInfo.RecordTime) != 0 && strlen(stQueryInfo.RecordEndTime) != 0)
	{
		snprintf(load.recordtime, sizeof(load.recordtime), "%s", stQueryInfo.RecordTime);
		snprintf(load.RecordEndTime, sizeof(load.RecordEndTime), "%s", stQueryInfo.RecordEndTime);
	}
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "1:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return NULL;
	}
	/*接收Head*/
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);
	if (recv_len != head_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "[%s]2:Failed to recv,recv_len=%d(==%d) !errno=%d<%s>", __func__, recv_len, head_len, errno, strerror(errno));
#endif
		return NULL;
	}
    dlog(LOG_DEBUG, "head.load_len=%d", head.load_len);
	/*检验头信息 */
	if (1 != head.msg_result || head.load_len <= 0)
	{
        dlog(LOG_WARN, "Array failed!!!errno=%d", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return NULL;
	}
	/*获取返回的xml*/
	xml_buf = (char *)malloc(head.load_len + 1);

	if (!xml_buf)
	{
        dlog(LOG_WARN, "Array failed!!!errno=%d,xml_buf=%p", head.msg_result, xml_buf);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "3:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return NULL;
	}
	recv_len = 0;
	memset(xml_buf, 0, head.load_len + 1);
	ret = RH_TcpRcvBlockFd(sock_fd, xml_buf, head.load_len, &recv_len);
	if (recv_len != head.load_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "4:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		r_free(xml_buf);
		return NULL;
	}
	RH_Close(__FILE__, (char *)__func__, sock_fd);
	/*返回获取成功的xml buffer*/
	return xml_buf;
}

/*==============================================================================
	函数: <DB_get_all_filename>
	功能: 获取所有的文件名
	参数:DB_xml :-返回排序后的xml,使用后需要销毁
	queryName设置为NULL,则获取所有文件名,设置为组名则获取该组所有的文件名
	返回值:0-成功   否则失败
==============================================================================*/
int DB_get_all_filename(char *queryName, char **DB_xml)
{
	DB_Communicate_Head head;
	DB_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	char *xml_buf = NULL;
	int head_len = sizeof(DB_Communicate_Head);
	int recv_len = 0;
	int send_len = 0;

	if (sock_fd == 0)
	{
		sock_fd = DB_connect_server(DB_SERVER_PORT);
	}

	if (sock_fd <= 0)
	{
		return -1;
	}

	/*发送接收好的head*/
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = 0;
	head.msg_code = DB_GET_ALL_FILENAME_MSG; //获取所有的文件名
	head.msg_result = 0;
	head.msg_type = 0;
	send_len = sizeof(DB_Communicate_Head);

	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	/*发送load*/
	memset(&load, 0, sizeof(DB_Communicate_t));

	if (queryName == NULL)
	{
		load.mode = 0; //获取所有文件名
	}
	else
	{
		load.mode = 1;											 //根据组名查询文件名
		snprintf(load.find, sizeof(load.find), "%s", queryName); //只需要组名
	}

	send_len = sizeof(DB_Communicate_t);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);
	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	/*接收处理后的head*/
	memset(&head, 0, sizeof(head));
	recv_len = 0;
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);
	if (recv_len != head_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	/*检验处理信息*/
	if (1 != head.msg_result || head.load_len <= 0)
	{
        dlog(LOG_WARN, "check---Array failed!!!errno=%d,load_len=%d", head.msg_result, head.load_len);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	/*接收处理的结果*/
	xml_buf = (char *)malloc(head.load_len + 1);
	if (!xml_buf)
	{
        dlog(LOG_WARN, "Array failed!!!errno=%d,xml_buf=%p", head.msg_result, xml_buf);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	memset(xml_buf, 0, head.load_len + 1);
	recv_len = 0;
	ret = RH_TcpRcvBlockFd(sock_fd, xml_buf, head.load_len, &recv_len);

	if (recv_len != head.load_len || ret < 0)
	{
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		r_free(xml_buf);
		return -1;
	}

	RH_Close(__FILE__, (char *)__func__, sock_fd);

	*DB_xml = xml_buf;
	/*返回buffer*/
	return 0;
}

/*==============================================================================
	函数: <DB_get_vod_filename_dim>
	功能: 模糊搜索已发布点播的视频文件名
	参数:DB_xml :-返回排序后的xml,使用后需要销毁
	queryName设置为NULL,则获取所有文件名,设置为组名则获取该组所有的文件名
	返回值:0-成功   否则失败
==============================================================================*/
int DB_get_vod_filename_dim(char *queryName, char **DB_xml, char *fileName)
{
	DB_Communicate_Head head;
	DB_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	char *xml_buf = NULL;
	int head_len = sizeof(DB_Communicate_Head);
	int recv_len = 0;
	int send_len = 0;

	if (sock_fd == 0)
	{
		sock_fd = DB_connect_server(DB_SERVER_PORT);
	}

	if (sock_fd <= 0)
	{
		return -1;
	}

	/*发送接收好的head*/
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = 0;
	head.msg_code = DB_GET_VOD_FILENAME_MSG_DIM; //获取所有的文件名
	head.msg_result = 0;
	head.msg_type = 0;
	send_len = sizeof(DB_Communicate_Head);

	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	/*发送load*/
	memset(&load, 0, sizeof(DB_Communicate_t));

	if (queryName == NULL)
	{
		load.mode = 0; //获取所有文件名
	}
	else
	{
		load.mode = 1;											 //根据组名查询文件名
		snprintf(load.find, sizeof(load.find), "%s", queryName); //只需要组名
	}

	snprintf(load.modify, sizeof(load.modify), "%s", fileName); //模糊文件名

	send_len = sizeof(DB_Communicate_t);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);
	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	/*接收处理后的head*/
	memset(&head, 0, sizeof(head));
	recv_len = 0;
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);
	if (recv_len != head_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	/*检验处理信息*/
	if (1 != head.msg_result || head.load_len <= 0)
	{
        dlog(LOG_WARN, "check---Array failed!!!errno=%d,load_len=%d", head.msg_result, head.load_len);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	/*接收处理的结果*/
	xml_buf = (char *)malloc(head.load_len + 1);
	if (!xml_buf)
	{
        dlog(LOG_WARN, "Array failed!!!errno=%d,xml_buf=%p", head.msg_result, xml_buf);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	memset(xml_buf, 0, head.load_len + 1);
	recv_len = 0;
	ret = RH_TcpRcvBlockFd(sock_fd, xml_buf, head.load_len, &recv_len);

	if (recv_len != head.load_len || ret < 0)
	{
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		r_free(xml_buf);
		return -1;
	}

	RH_Close(__FILE__, (char *)__func__, sock_fd);

	*DB_xml = xml_buf;
	/*返回buffer*/
	return 0;
}

/*==============================================================================
	函数: <DB_get_filename_suffix>
	功能: 获取文件名的后缀名
	参数:suffix :-返回获取到的后缀名
	返回值:0-成功   否则失败
==============================================================================*/
int DB_get_filename_suffix(char *queryName, int *suffix)
{
	DB_Communicate_Head head;
	DB_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	char *xml_buf = NULL;
	int head_len = sizeof(DB_Communicate_Head);
	int recv_len = 0;
	int send_len = 0;

	if (sock_fd == 0)
	{
		sock_fd = DB_connect_server(DB_SERVER_PORT);
	}

	if (sock_fd <= 0)
	{
		return -1;
	}

	/*发送接收好的head*/
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = 0;
	head.msg_code = DB_GET_FILENAME_SUFFIX;
	head.msg_result = 0;
	head.msg_type = 0;
	send_len = sizeof(DB_Communicate_Head);

	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	/*发送load*/
	memset(&load, 0, sizeof(DB_Communicate_t));

	snprintf(load.find, sizeof(load.find), "%s", queryName); //只需要组名
	send_len = sizeof(DB_Communicate_t);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);
	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	/*接收处理后的head*/
	memset(&head, 0, sizeof(head));
	recv_len = 0;
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);
	if (recv_len != head_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to recv !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	/*检验处理信息*/
	if (1 != head.msg_result || head.load_len <= 0)
	{
        dlog(LOG_WARN, "check---Array failed!!!errno=%d,load_len=%d", head.msg_result, head.load_len);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	/*接收处理的结果*/
	xml_buf = (char *)malloc(head.load_len + 1);
	if (!xml_buf)
	{
        dlog(LOG_WARN, "Array failed!!!errno=%d,xml_buf=%p", head.msg_result, xml_buf);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	memset(xml_buf, 0, head.load_len + 1);
	recv_len = 0;
	ret = RH_TcpRcvBlockFd(sock_fd, xml_buf, head.load_len, &recv_len);

	if (recv_len != head.load_len || ret < 0)
	{
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		r_free(xml_buf);
		return -1;
	}

	RH_Close(__FILE__, (char *)__func__, sock_fd);

	if (strcmp(xml_buf, "") != 0)
	{
		*suffix = atoi(xml_buf);
	}

	return 0;
}

char *DB_find_FTPupload(int mode)
{
	DB_Communicate_Head head;
	//	DB_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	char *xml_buf = NULL;
	int head_len = sizeof(DB_Communicate_Head);
	int send_len = 0;
	int recv_len = 0;

	if (sock_fd == 0)
	{
		sock_fd = DB_connect_server(DB_SERVER_PORT);
	}

	if (sock_fd <= 0)
	{
		return NULL;
	}

	/*拼装head*/
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DB_Communicate_t);
	head.msg_code = DB_SELECT_FILELIST_MSG;
	head.msg_result = 0;
	head.msg_type = 0;

	/*发送head*/
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "0:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return NULL;
	}

	/*接收Head*/
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);

	if (recv_len != head_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "[%s]2:Failed to recv,recv_len=%d(==%d) !errno=%d<%s>", __func__, recv_len, head_len, errno, strerror(errno));
#endif
		return NULL;
	}

    dlog(LOG_DEBUG, "head.load_len=%d", head.load_len);

	/*检验头信息 */
	if (1 != head.msg_result || head.load_len <= 0)
	{
        dlog(LOG_WARN, "Array failed!!!errno=%d", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return NULL;
	}

	/*获取返回的xml*/
	xml_buf = (char *)malloc(head.load_len + 1);

	if (!xml_buf)
	{
        dlog(LOG_WARN, "Array failed!!!errno=%d,xml_buf=%p", head.msg_result, xml_buf);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "3:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return NULL;
	}

	recv_len = 0;
	memset(xml_buf, 0, head.load_len + 1);
	ret = RH_TcpRcvBlockFd(sock_fd, xml_buf, head.load_len, &recv_len);

	if (recv_len != head.load_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "4:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		r_free(xml_buf);
		return NULL;
	}

	RH_Close(__FILE__, (char *)__func__, sock_fd);
	/*返回获取成功的xml buffer*/
	return xml_buf;
}

/*==============================================================================
	函数: <DB_delete_elem>
	功能: 删除文件名为name的条目
	参数:name  :-需要的FileName
	返回值:0-成功   否则失败
==============================================================================*/
int DB_delete_elem(char *file_name)
{
	DB_Communicate_Head head;
	DB_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	int head_len = sizeof(DB_Communicate_Head);
	int send_len = 0;
	int recv_len = 0;

	if (!file_name)
	{
        dlog(LOG_ERROR, "file_name=%p", file_name);
		return -1;
	}

	if (sock_fd == 0)
	{
		sock_fd = DB_connect_server(DB_SERVER_PORT);
	}

	if (sock_fd <= 0)
	{
		return -1;
	}

	/* 拼装Head */
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DB_Communicate_t);
	head.msg_code = DB_DELETE_FILELIST_MSG;
	head.msg_result = 0;
	head.msg_type = 0;
	/*发送Head*/
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "0:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return -1;
	}

	/*发送load*/
	send_len = sizeof(DB_Communicate_t);
	memset(&load, 0, sizeof(DB_Communicate_t));
	snprintf(load.item.FileName, sizeof(load.item.FileName), "%s", file_name);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "[%s]1:Failed to recv !errno=%d<%s>", __func__, errno, strerror(errno));
#endif
		return -1;
	}

	/*接收处理后的head*/
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);

	if (recv_len != head_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "2:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return -1;
	}

	/*检验处理结果*/
	if (1 != head.msg_result)
	{
        dlog(LOG_WARN, "DELETE failed!!!errno=%d", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	RH_Close(__FILE__, (char *)__func__, sock_fd);
	return 0;
}

/*==============================================================================
	函数: <DB_insert_elem>
	功能: 插入新的elem
	参数:file_name  :-需要的FileName
			   moive_size:-电影模式的文件大小
			   res_size:-资源模式的文件大小
			   rce_start_time:-录制起始时间
			   time_lenght:-录制总时长
	返回值:0-成功   否则失败
==============================================================================*/
int DB_insert_elem(Record_CourseInfo_t course_info)
{
	DB_Communicate_Head head;
	DB_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	int head_len = sizeof(DB_Communicate_Head);
	int send_len = 0;
	int recv_len = 0;

	if (!course_info.dir_path || !course_info.start_time)
	{
        dlog(LOG_ERROR, "course_info.dir_path=%p-%lld,%d,%p,%s\n", course_info.dir_path, course_info.total_size, course_info.file_type, course_info.start_time, course_info.start_time);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "0:course_info.dir_path=%p-%lld,%lld,%p,%s", course_info.dir_path, course_info.total_size, course_info.file_type, course_info.start_time, course_info.start_time);
#endif
		return -1;
	}

	if (sock_fd == 0)
	{
		sock_fd = DB_connect_server(DB_SERVER_PORT);
	}

	if (sock_fd <= 0)
	{
		return -1;
	}

	memset(&load, 0, sizeof(DB_Communicate_t));
	snprintf(load.item.DirPath, sizeof(load.item.DirPath), "%s", course_info.dir_path);
	snprintf(load.item.FileName, sizeof(load.item.FileName), "%s", course_info.file_name);
	snprintf(load.item.RcdStartTime, sizeof(load.item.RcdStartTime), "%s", course_info.start_time);
	snprintf(load.item.RcdTimeLenght, sizeof(load.item.RcdTimeLenght), "%s", course_info.record_time);
	load.item.nFileType = course_info.file_type;
	load.item.MovieSize = course_info.total_size;
	load.item.nPerTime = course_info.per_time;
	load.item.nStartTime = course_info.nStartTime;
	snprintf(load.item.Reserve5, sizeof(load.item.Reserve5), "%s", course_info.recordId);
	load.item.RecordMode = course_info.RecordMode;
	// load.item.RcdTimeLenght = course_info.record_time;

	/* 拼装Head */
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DB_Communicate_t);
	head.msg_code = DB_INSERT_RECORD_MSG;
	head.msg_result = 0;
	head.msg_type = 0;
	/*发送Head*/
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "1:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return -1;
	}

	/*发送需要处理的load*/
	send_len = sizeof(DB_Communicate_t);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "2:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return -1;
	}

	/*接收处理后的head*/
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);

	if (recv_len != head_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "3:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return -1;
	}

	/*检验处理结果*/
	if (1 != head.msg_result)
	{
        dlog(LOG_WARN, "insert failed!!!errno=%d", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "4:insert failed!!!errno=%d", head.msg_result);
#endif
		return -1;
	}

	// printf("the head.msg_result is...... :[%d]",head.msg_result);
	RH_Close(__FILE__, (char *)__func__, sock_fd);
	return 0;
}

/*==============================================================================
	函数: <DB_update_elem>
	功能: 更新
	参数:file_name  :-需要的FileName
			   moive_size:-电影模式的文件大小
			   res_size:-资源模式的文件大小
			   rce_start_time:-录制起始时间
			   time_lenght:-录制总时长
	返回值:0-成功   否则失败
==============================================================================*/
int DB_update_elem(Record_CourseInfo_t course_info)
{
	DB_Communicate_Head head;
	DB_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	int head_len = sizeof(DB_Communicate_Head);
	int send_len = 0;
	int recv_len = 0;

	if (!course_info.dir_path || !course_info.start_time)
	{
        dlog(LOG_ERROR, "course_info.dir_path=%p-%lld,%d,%p,%s\n", course_info.dir_path, course_info.total_size, course_info.file_type, course_info.start_time, course_info.start_time);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "0:course_info.dir_path=%p-%lld,%lld,%p,%s", course_info.dir_path, course_info.total_size, course_info.file_type, course_info.start_time, course_info.start_time);
#endif
		return -1;
	}

	if (sock_fd == 0)
	{
		sock_fd = DB_connect_server(DB_SERVER_PORT);
	}

	if (sock_fd <= 0)
	{
		return -1;
	}

	memset(&load, 0, sizeof(DB_Communicate_t));
	snprintf(load.item.FileName, sizeof(load.item.FileName), "%s", course_info.file_name);
	snprintf(load.item.RcdTimeLenght, sizeof(load.item.RcdTimeLenght), "%s", course_info.record_time);
	load.item.MovieSize = course_info.total_size;
	// load.item.RcdTimeLenght = course_info.record_time;

	/* 拼装Head */
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DB_Communicate_t);
	head.msg_code = DB_UPDATE_RECORD_MSG;
	head.msg_result = 0;
	head.msg_type = 0;
	/*发送Head*/
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "1:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return -1;
	}

	/*发送需要处理的load*/
	send_len = sizeof(DB_Communicate_t);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "2:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return -1;
	}

	/*接收处理后的head*/
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);

	if (recv_len != head_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "3:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return -1;
	}

	/*检验处理结果*/
	if (1 != head.msg_result)
	{
        dlog(LOG_WARN, "insert failed!!!errno=%d", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "4:insert failed!!!errno=%d", head.msg_result);
#endif
		return -1;
	}

	// printf("the head.msg_result is...... :[%d]",head.msg_result);
	RH_Close(__FILE__, (char *)__func__, sock_fd);
	return 0;
}

/*==============================================================================
	函数: <DB_modify_elem>
	功能: 修改elem
	参数:mode  :-
*	1-修改file_name
*	2-修改Notes
*	3-修改CourseTeacher
* 	4-修改CourseSubject
*	5-修改FTPupload
*	6-修改BackupFTPload
*	7-修改DownloadCnt
		modify:内容
			file_name:需要修改的文件名
	返回值:0-成功   否则失败
==============================================================================*/
int DB_modify_elem(int mode, char *modify, char *file_name)
{
	DB_Communicate_Head head;
	DB_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	int head_len = sizeof(DB_Communicate_Head);
	int recv_len = 0;
	int send_len = 0;
	//	char temp[1024]={0};

	if (!file_name || (mode < 0) || (mode > 20) || !modify)
	{
        dlog(LOG_ERROR, "name=%p,mode=%d,%p", file_name, mode, modify);
		return -1;
	}

	if (sock_fd == 0)
	{
		sock_fd = DB_connect_server(DB_SERVER_PORT);
	}

	if (sock_fd <= 0)
	{
		return -1;
	}

	memset(&load, 0, sizeof(DB_Communicate_t));
	load.mode = mode;
	/*	if( check_CHN(modify,r_strlen(modify)){
			code_convert("utf-8", "utf-32",modify, r_strlen(modify), temp, r_strlen(modify) * 4);
			snprintf(load.modify,sizeof(load.modify),"%s",temp);
		}else{*/
	snprintf(load.modify, sizeof(load.modify), "%s", modify);
	//}
	snprintf(load.item.FileName, sizeof(load.item.FileName), "%s", file_name);
	/* 拼装Head */
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DB_Communicate_t);
	head.msg_code = DB_MODIFY_FILELIST_MSG;
	head.msg_result = 0;
	head.msg_type = 0;
	/*发送Head*/
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	/*发送需要处理的load*/
	send_len = sizeof(DB_Communicate_t);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "0:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return -1;
	}

	/*接收处理后的head*/
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);

	if (recv_len != head_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "1:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return -1;
	}

	/*检验处理结果*/
	if (1 != head.msg_result)
	{
        dlog(LOG_WARN, "DELETE failed!!!errno=%d", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "3:DELETE failed!!!errno=%d", head.msg_result);
#endif
		return -1;
	}
	RH_Close(__FILE__, (char *)__func__, sock_fd);
	return 0;
}

int DB_modify_fNode(Ftp_ModifyFile_t *modifyFile_info)
{
	int ret = -1;
	char IDbuf[16] = {0};
	if (modifyFile_info->mode == DB_MODIDY_FILE_NAME)
	{
		if (0 != (strcmp(modifyFile_info->file_name, modifyFile_info->new_fName)))
		{
			if (strcmp(modifyFile_info->new_fName, "") == 0)
			{

				return -1;
			}
			ret = DB_modify_elem(DB_MODIDY_FILE_NAME, modifyFile_info->new_fName, modifyFile_info->file_name);
			if (0 != ret)
			{
				return -1;
			}
		}
	}
	if (modifyFile_info->mode == DB_MODIFY_NOTES)
	{
		ret = DB_modify_elem(DB_MODIFY_NOTES, modifyFile_info->notes, modifyFile_info->file_name);
		// printf("********ret: %d", ret);
		if (0 != ret)
		{
			return -1;
		}
	}
	if (modifyFile_info->mode == DB_MODIDY_COURSE_TEACHER)
	{
		ret = DB_modify_elem(DB_MODIDY_COURSE_TEACHER, modifyFile_info->course_teacher, modifyFile_info->file_name);
		if (0 != ret)
		{
			return -1;
		}
	}
	if (modifyFile_info->mode == DB_MODIFY_COURSE_SUBJECT)
	{
		ret = DB_modify_elem(DB_MODIFY_COURSE_SUBJECT, modifyFile_info->course_subject, modifyFile_info->file_name);
		if (0 != ret)
		{
			return -1;
		}
	}
	if (modifyFile_info->mode == DB_MODIDY_GROUP_NUMBER)
	{
		sprintf(IDbuf, "%d", modifyFile_info->GroupID);
		ret = DB_modify_elem(DB_MODIDY_GROUP_NUMBER, IDbuf, modifyFile_info->file_name);
		if (0 != ret)
		{
			return -1;
		}
	}
	if (modifyFile_info->mode == DB_MODIDY_NANE_AND_NUMBER)
	{
		if (0 != (strcmp(modifyFile_info->file_name, modifyFile_info->new_fName)))
		{
			if (strcmp(modifyFile_info->new_fName, "") == 0)
			{

				return -1;
			}
			ret = DB_modify_elem(DB_MODIDY_FILE_NAME, modifyFile_info->new_fName, modifyFile_info->file_name);
			if (0 != ret)
			{
				return -1;
			}
		}
		sprintf(IDbuf, "%d", modifyFile_info->GroupID);
		ret = DB_modify_elem(DB_MODIDY_GROUP_NUMBER, IDbuf, modifyFile_info->new_fName);
		if (0 != ret)
		{
			return -1;
		}
	}

	if (modifyFile_info->mode == DB_MODIDY_VOD_NUMBER) //修改点播次数
	{
		sprintf(IDbuf, "%d", 1);
		ret = DB_modify_elem(DB_MODIDY_VOD_NUMBER, IDbuf, modifyFile_info->file_name);
		if (0 != ret)
		{
			return -1;
		}
	}
	if (modifyFile_info->mode == DB_MODIDY_VOD_ISPUBLISH) //是否发布视频
	{
		sprintf(IDbuf, "%d", modifyFile_info->ispublish);
	    dlog(LOG_TRACE, "modifyFile_info->ispublish=%d", modifyFile_info->ispublish);
		ret = DB_modify_elem(DB_MODIDY_VOD_ISPUBLISH, IDbuf, modifyFile_info->file_name);
		if (0 != ret)
		{
			return -1;
		}
	}
	if (modifyFile_info->mode == DB_MODIDY_FILE_SIZE)
	{
        dlog(LOG_DEBUG, "modifyFile_info->aFileSize %s,modifyFile_info->file_name %s", modifyFile_info->aFileSize, modifyFile_info->file_name);
		ret = DB_modify_elem(DB_MODIDY_FILE_SIZE, modifyFile_info->aFileSize, modifyFile_info->file_name);
		if (0 != ret)
		{
			return -1;
		}
	}
	if (modifyFile_info->mode == DB_MODIDY_FILE_GROUP_ONEKEY) //恢复出厂设置时，一键修改文件用户组
	{
        dlog(LOG_DEBUG, "modifyFile file group onekey");
		ret = DB_modify_elem(DB_MODIDY_FILE_GROUP_ONEKEY, "0", "0");
		if (0 != ret)
		{
			return -1;
		}
	}

	return 1;
}

int DB_modify_elem_court(Record_CourseInfo_t course_info)
{
	DB_Communicate_Head head;
	DB_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	int head_len = sizeof(DB_Communicate_Head);
	int recv_len = 0;
	int send_len = 0;
	//	char temp[1024]={0};

	if (sock_fd == 0)
	{
		sock_fd = DB_connect_server(DB_SERVER_PORT);
	}

	if (sock_fd <= 0)
	{
		return -1;
	}

	memset(&load, 0, sizeof(DB_Communicate_t));
	snprintf(load.item.FileName, sizeof(load.item.FileName), "%s", course_info.file_name);
	snprintf(load.item.RcdTimeLenght, sizeof(load.item.RcdTimeLenght), "%s", course_info.record_time);
	load.item.MovieSize = course_info.total_size;

	// memcpy(load.item.CoutrInfo,course_info.CourtInfo,sizeof(DB_Coutr_info_s));
	/* 拼装Head */
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DB_Communicate_t);
	head.msg_code = DB_MODIFY_COURTINFO_MSG;
	head.msg_result = 0;
	head.msg_type = 0;
	/*发送Head*/
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

	/*发送需要处理的load*/
	send_len = sizeof(DB_Communicate_t);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "0:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return -1;
	}

	/*接收处理后的head*/
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);

	if (recv_len != head_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "1:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return -1;
	}

	/*检验处理结果*/
	if (1 != head.msg_result)
	{
        dlog(LOG_WARN, "DELETE failed!!!errno=%d", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "3:DELETE failed!!!errno=%d", head.msg_result);
#endif
		return -1;
	}
	RH_Close(__FILE__, (char *)__func__, sock_fd);
	return 1;
}

char *DB_find_groupFileName(char *queryName, int mode, int curPage, int pageSize, int to, int from)
{
	DB_Communicate_Head head;
	DB_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	char *xml_buf = NULL;
	int head_len = sizeof(DB_Communicate_Head);
	int send_len = 0;
	int recv_len = 0;

	if (!queryName)
	{
        dlog(LOG_ERROR, "name=%p", queryName);
		return NULL;
	}

	if (sock_fd == 0)
	{
		sock_fd = DB_connect_server(DB_SERVER_PORT);
	}

	if (sock_fd <= 0)
	{
		return NULL;
	}

	/*拼装head*/
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DB_Communicate_t);

	head.msg_code = DB_GET_GROUP_FILE; //获取文件列表
	if (mode <= 0)
	{
		mode = 1;
	}
	head.msg_result = 0;
	head.msg_type = 0;

	/*发送head*/
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "0:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return NULL;
	}

	/*发送load*/
	send_len = sizeof(DB_Communicate_t);
	memset(&load, 0, sizeof(DB_Communicate_t));
	load.mode = mode; // 1-FileName
	load.from = from;
	load.to = to;
	load.order = mode;
	load.pageSize = pageSize;
	load.CurPage = curPage;

	if (strlen(queryName) != 0)
	{
		snprintf(load.find, sizeof(load.find), "%s", queryName);
	}
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "1:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return NULL;
	}
	/*接收Head*/
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);
	if (recv_len != head_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "[%s]2:Failed to recv,recv_len=%d(==%d) !errno=%d<%s>", __func__, recv_len, head_len, errno, strerror(errno));
#endif
		return NULL;
	}
    dlog(LOG_DEBUG, "head.load_len=%d", head.load_len);
	/*检验头信息 */
	if (1 != head.msg_result || head.load_len <= 0)
	{
        dlog(LOG_WARN, "Array failed!!!errno=%d", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return NULL;
	}
	/*获取返回的xml*/
	xml_buf = (char *)malloc(head.load_len + 1);

	if (!xml_buf)
	{
        dlog(LOG_WARN, "Array failed!!!errno=%d,xml_buf=%p", head.msg_result, xml_buf);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "3:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return NULL;
	}
	recv_len = 0;
	memset(xml_buf, 0, head.load_len + 1);
	ret = RH_TcpRcvBlockFd(sock_fd, xml_buf, head.load_len, &recv_len);
	if (recv_len != head.load_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "4:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		r_free(xml_buf);
		return NULL;
	}
	RH_Close(__FILE__, (char *)__func__, sock_fd);
	/*返回获取成功的xml buffer*/
	return xml_buf;
}

char *DB_select_groupFileName(char *queryName, int mode, int curPage, int pageSize, int to, int from, int groupID)
{
	DB_Communicate_Head head;
	DB_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	char *xml_buf = NULL;
	int head_len = sizeof(DB_Communicate_Head);
	int send_len = 0;
	int recv_len = 0;

	if (!queryName)
	{
        dlog(LOG_ERROR, "name=%p", queryName);
		return NULL;
	}

	if (sock_fd == 0)
	{
		sock_fd = DB_connect_server(DB_SERVER_PORT);
	}

	if (sock_fd <= 0)
	{
		return NULL;
	}

	/*拼装head*/
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DB_Communicate_t);

	head.msg_code = DB_SELECT_GROUP_SELECTFILE; //获取文件列表
	if (mode <= 0)
	{
		mode = 1;
	}
	head.msg_result = 0;
	head.msg_type = 0;

	/*发送head*/
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "0:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return NULL;
	}

	/*发送load*/
	send_len = sizeof(DB_Communicate_t);
	memset(&load, 0, sizeof(DB_Communicate_t));
	load.mode = mode; // 1-FileName
	load.from = from;
	load.to = to;
	load.order = mode;
	load.pageSize = pageSize;
	load.CurPage = curPage;
	load.groupID = groupID;

	if (strlen(queryName) != 0)
	{
		snprintf(load.find, sizeof(load.find), "%s", queryName);
	}
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "1:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return NULL;
	}
	/*接收Head*/
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);
	if (recv_len != head_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "[%s]2:Failed to recv,recv_len=%d(==%d) !errno=%d<%s>", __func__, recv_len, head_len, errno, strerror(errno));
#endif
		return NULL;
	}
    dlog(LOG_DEBUG, "head.load_len=%d", head.load_len);
	/*检验头信息 */
	if (1 != head.msg_result || head.load_len <= 0)
	{
        dlog(LOG_WARN, "Array failed!!!errno=%d", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return NULL;
	}
	/*获取返回的xml*/
	xml_buf = (char *)malloc(head.load_len + 1);

	if (!xml_buf)
	{
        dlog(LOG_WARN, "Array failed!!!errno=%d,xml_buf=%p", head.msg_result, xml_buf);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "3:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return NULL;
	}
	recv_len = 0;
	memset(xml_buf, 0, head.load_len + 1);
	ret = RH_TcpRcvBlockFd(sock_fd, xml_buf, head.load_len, &recv_len);
	if (recv_len != head.load_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "4:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		r_free(xml_buf);
		return NULL;
	}
	RH_Close(__FILE__, (char *)__func__, sock_fd);
	/*返回获取成功的xml buffer*/
	return xml_buf;
}

char *DB_findExect_FileName(char *queryName, int mode, int curPage, int pageSize, int to, int from)
{
	DB_Communicate_Head head;
	DB_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	char *xml_buf = NULL;
	int head_len = sizeof(DB_Communicate_Head);
	int send_len = 0;
	int recv_len = 0;

	if (!queryName)
	{
        dlog(LOG_ERROR, "name=%p", queryName);
		return NULL;
	}

	if (sock_fd == 0)
	{
		sock_fd = DB_connect_server(DB_SERVER_PORT);
	}

	if (sock_fd <= 0)
	{
		return NULL;
	}

	/*拼装head*/
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DB_Communicate_t);

	head.msg_code = DB_SELECT_FILELIST_EXACT_MSG; //获取文件列表
	if (mode <= 0)
	{
		mode = 1;
	}
	head.msg_result = 0;
	head.msg_type = 0;

	/*发送head*/
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "0:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return NULL;
	}

	/*发送load*/
	send_len = sizeof(DB_Communicate_t);
	memset(&load, 0, sizeof(DB_Communicate_t));
	load.mode = mode; // 1-FileName
	load.from = from;
	load.to = to;
	load.order = mode;
	load.pageSize = pageSize;
	load.CurPage = curPage;

	if (strlen(queryName) != 0)
	{
		snprintf(load.find, sizeof(load.find), "%s", queryName);
	}
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "1:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return NULL;
	}
	/*接收Head*/
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);
	if (recv_len != head_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "[%s]2:Failed to recv,recv_len=%d(==%d) !errno=%d<%s>", __func__, recv_len, head_len, errno, strerror(errno));
#endif
		return NULL;
	}
    dlog(LOG_DEBUG, "head.load_len=%d", head.load_len);
	/*检验头信息 */
	if (1 != head.msg_result || head.load_len <= 0)
	{
        dlog(LOG_WARN, "Array failed!!!errno=%d", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return NULL;
	}
	/*获取返回的xml*/
	xml_buf = (char *)malloc(head.load_len + 1);

	if (!xml_buf)
	{
        dlog(LOG_WARN, "Array failed!!!errno=%d,xml_buf=%p", head.msg_result, xml_buf);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "3:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return NULL;
	}
	recv_len = 0;
	memset(xml_buf, 0, head.load_len + 1);
	ret = RH_TcpRcvBlockFd(sock_fd, xml_buf, head.load_len, &recv_len);
	if (recv_len != head.load_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "4:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		r_free(xml_buf);
		return NULL;
	}
	RH_Close(__FILE__, (char *)__func__, sock_fd);
	/*返回获取成功的xml buffer*/
	return xml_buf;
}

int DBFTP_insert_elem(char *file_name, char *notes, long long int movie_size, long long int res_size, const char *rce_start_time, char *time_lenght, char *teacher, char *course)
{
	DB_Communicate_Head head;
	DB_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	int head_len = sizeof(DB_Communicate_Head);
	int send_len = 0;
	int recv_len = 0;

	if (!file_name || 0 == movie_size || !rce_start_time || rce_start_time <= 0)
	{
        dlog(LOG_ERROR, "file_name=%p-%lld,%lld,%p,%s\n", file_name, movie_size, res_size, rce_start_time, rce_start_time);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "0:file_name=%p-%lld,%lld,%p,%s", file_name, movie_size, res_size, rce_start_time, rce_start_time);
#endif
		return -1;
	}

	if (sock_fd == 0)
	{
		sock_fd = DB_connect_server(DB_SERVER_PORT);
	}

	if (sock_fd <= 0)
	{
		return -1;
	}

	memset(&load, 0, sizeof(DB_Communicate_t));
	snprintf(load.item.FileName, sizeof(load.item.FileName), "%s", file_name);
	snprintf(load.item.Notes, sizeof(load.item.Notes), "%s", notes);
	snprintf(load.item.RcdStartTime, sizeof(load.item.RcdStartTime), "%s", rce_start_time);
	snprintf(load.item.CourseTeacher, sizeof(load.item.CourseTeacher), "%s", teacher);
	snprintf(load.item.CourseSubject, sizeof(load.item.CourseSubject), "%s", course);
	snprintf(load.item.RcdTimeLenght, sizeof(load.item.RcdTimeLenght), "%s", time_lenght);
	load.item.MovieSize = movie_size;
	//load.item.ResSize = res_size;
	//load.item.Reserve4 = res_size;
	// load.item.Reserve5 = 999;   //慧课星标志
	load.item.FTPupload = 0;
	load.item.BackupFTPload = 3;
	load.item.Reserve1 = 1;
	load.item.CourseCompleteStatus = 1;
	// load.item.RcdTimeLenght = time_lenght;

	/* 拼装Head */
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DB_Communicate_t);
	head.msg_code = DB_INSERT_FILELIST_MSG;
	head.msg_result = 0;
	head.msg_type = 0;
	/*发送Head*/
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "1:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return -1;
	}

	/*发送需要处理的load*/
	send_len = sizeof(DB_Communicate_t);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);

	if (ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "2:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return -1;
	}

	/*接收处理后的head*/
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);

	if (recv_len != head_len || ret < 0)
	{
        dlog(LOG_ERROR, "Failed to send !errno=%d<%s>", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "3:Failed to recv !errno=%d<%s>", errno, strerror(errno));
#endif
		return -1;
	}

	/*检验处理结果*/
	if (1 != head.msg_result)
	{
        dlog(LOG_WARN, "insert failed!!!errno=%d", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
#if DB_CLIENT_DEBUG
	    dlog(LOG_TRACE, "4:insert failed!!!errno=%d", head.msg_result);
#endif
		return -1;
	}

    dlog(LOG_TRACE, "the head.msg_result is...... :[%d]", head.msg_result);
	RH_Close(__FILE__, (char *)__func__, sock_fd);
	return 0;
}