#include "share_dbBase.h"
#include "public_define.h"
#include "db_middle.h"
#include "group_list.h"
int DBGroup_insert_elem(DataBaseGroup_t DbGroupInfo)
{
	DB_Communicate_Head head;
	DBGroup_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	int head_len = sizeof(DB_Communicate_Head);
	int send_len = 0;
	int recv_len = 0;
	if(strlen(DbGroupInfo.GroupName)<1) 
	{
		dlog(LOG_ERROR, "DbUserInfo.UserName = %s\n\n",DbGroupInfo.GroupName);
		DEBUG_INFO("DbUserInfo.UserName = %s",DbGroupInfo.GroupName);
		return -1;
	}

	if(sock_fd == 0) {
		sock_fd = DB_connect_server(DB_GROUP_SERVER_PORT);
	}
	if(sock_fd <= 0) {
		return -1;
	}
	memset(&load, 0, sizeof(DBGroup_Communicate_t));
	load.item.ID = DbGroupInfo.ID;
	snprintf(load.item.GroupName, sizeof(load.item.GroupName), "%s", DbGroupInfo.GroupName);
	load.item.Reserve1 = 0;
	load.item.Reserve2 = 0;
	load.item.Reserve3 = 0;
	snprintf(load.item.Reserve4, sizeof(load.item.Reserve4), "%s",DbGroupInfo.Reserve4);
	snprintf(load.item.Reserve5, sizeof(load.item.Reserve5), "%s",DbGroupInfo.Reserve5);
	snprintf(load.item.Reserve6, sizeof(load.item.Reserve6), "%s",DbGroupInfo.Reserve6);

	//load.item.RcdTimeLenght = time_lenght;
	/* 封装Head */
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DBGroup_Communicate_t);
	head.msg_code = DB_ADD_GROUP_MSG;
	head.msg_result = 0;
	head.msg_type = 0;
	/*发送Head*/
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);

	if(ret < 0) {
		dlog(LOG_ERROR, "Failed to send !errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}
	/*发送需要处理的load*/
	send_len = sizeof(DBGroup_Communicate_t);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);
	if(ret < 0) {
		dlog(LOG_ERROR, "Failed to send !errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}
	/*接收处理后的head*/
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);
	if(recv_len !=  head_len || ret < 0) {
		dlog(LOG_ERROR, "Failed to send !errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}
	/*接收处理结果*/
	if(1 != head.msg_result) {
		dlog(LOG_WARN, "insert failed!!!errno=%d\n", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}

    dlog(LOG_TRACE, "the head.msg_result is...... :[%d]",head.msg_result);
	RH_Close(__FILE__, (char *)__func__, sock_fd);
	return 0;
}




int DBGroup_delete_elem(char *DbGroupInfo)
{
	DB_Communicate_Head head;
	DBGroup_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	int head_len = sizeof(DB_Communicate_Head);
	int send_len = 0;
	int recv_len = 0;
	if(!DbGroupInfo) {
		dlog(LOG_ERROR, "file_name=%p\n", DbGroupInfo);
		return -1;
	}
	if(sock_fd == 0) {
		sock_fd = DB_connect_server(DB_GROUP_SERVER_PORT);
	}
	if(sock_fd <= 0) {
		return -1;
	}
	/* 封装Head */
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DBGroup_Communicate_t);
	head.msg_code = DB_DELETE_GROUP_MSG;
	head.msg_result = 0;
	head.msg_type = 0;
	/*发送Head*/
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);
	if(ret < 0) {
		dlog(LOG_ERROR, "Failed to send !errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}
	/*发送需要处理的load*/
	send_len = sizeof(DBGroup_Communicate_t);
	memset(&load, 0, sizeof(DBGroup_Communicate_t));
	snprintf(load.item.GroupName, sizeof(load.item.GroupName), "%s", DbGroupInfo);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);
	if(ret < 0) {
		dlog(LOG_ERROR, "Failed to send !errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}
	/*接收处理后的head*/
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);
	if(recv_len !=  head_len || ret < 0) {
		dlog(LOG_ERROR, "Failed to send !errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}
	/*接收处理结果*/
	if(1 != head.msg_result) {
		dlog(LOG_WARN, "DELETE failed!!!errno=%d\n", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}
	RH_Close(__FILE__, (char *)__func__, sock_fd);
	return 0;
}







int DBGroup_modify_elem(int mode,char *modify, char *DbGroupInfo)
{
	DB_Communicate_Head head;
	DBGroup_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	int head_len = sizeof(DB_Communicate_Head);
	int recv_len = 0;
	int send_len = 0;
	if(!DbGroupInfo || (mode < 0) || (mode > 10) || !modify) {
		dlog(LOG_ERROR, "name=%p,mode=%d,%p\n", DbGroupInfo, mode, modify);
		return -1;
	}
	if(sock_fd == 0) {
		sock_fd = DB_connect_server(DB_GROUP_SERVER_PORT);
	}
	if(sock_fd <= 0) {
		return -1;
	}
	memset(&load, 0, sizeof(DBGroup_Communicate_t));
	load.mode = mode;
	snprintf(load.modify, sizeof(load.modify), "%s", modify);
	snprintf(load.item.GroupName, sizeof(load.item.GroupName), "%s", DbGroupInfo);
	/* 封装Head */
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DBGroup_Communicate_t);
	head.msg_code = DB_MODIFY_GROUP_MSG;
	head.msg_result = 0;
	head.msg_type = 0;
	/*发送Head*/
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);
	if(ret < 0) {
		dlog(LOG_ERROR, "Failed to send !errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}
	/*发送需要处理的Load*/
	send_len = sizeof(DBGroup_Communicate_t);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);
	if(ret < 0) {
		dlog(LOG_ERROR, "Failed to send !errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}
	/*接收处理后的head*/
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);
	if(recv_len !=  head_len || ret < 0) {
		dlog(LOG_ERROR, "Failed to send !errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return -1;
	}
	/*接收处理结果*/
	if(1 != head.msg_result) {
		dlog(LOG_WARN, "DELETE failed!!!errno=%d\n", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
	    return -1;
	}
	RH_Close(__FILE__, (char *)__func__, sock_fd);
	return 0;
}



char *DBGroup_find_elem(char *DbGroupInfo,int mode,int curPage,int pageSize, int to, int from,int code)
{
	DB_Communicate_Head head;
	DBGroup_Communicate_t load;
	int sock_fd = 0;
	int ret = 0;
	char *xml_buf = NULL;
	int head_len = sizeof(DB_Communicate_Head);
	int send_len = 0;
	int recv_len = 0;
	if(!DbGroupInfo) {
		dlog(LOG_ERROR, "name=%p\n", DbGroupInfo);
		return NULL;
	}
	if(sock_fd == 0) {
		sock_fd = DB_connect_server(DB_GROUP_SERVER_PORT);
	}
	if(sock_fd <= 0) {
		return NULL;
	}
	/*封装Head*/
	memset(&head, 0, sizeof(head));
	head.identifier = DB_IDENTIFIER;
	head.load_len = sizeof(DBGroup_Communicate_t);
	head.msg_code = code;			
	if(mode <= 0)
	{
		mode = 1;
	}
	head.msg_result = 0;
	head.msg_type = 0;
	/*发送Head*/
	send_len = sizeof(DB_Communicate_Head);
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&head, &send_len);
	if(ret < 0) {
		dlog(LOG_ERROR, "Failed to send !errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return NULL;
	}
	/*发送需要处理的load*/
	send_len = sizeof(DBGroup_Communicate_t);
	memset(&load, 0, sizeof(DBGroup_Communicate_t));
	load.mode = mode; // 1-FileName
	load.from = from;
	load.to = to;
	//load.order = mode;
	load.pageSize = pageSize;
	load.CurPage = curPage;
	if(strlen(DbGroupInfo)!= 0){
		snprintf(load.find, sizeof(load.find), "%s", DbGroupInfo);
	}
	ret = RH_TcpSndBlockFd(sock_fd, (char *)&load, &send_len);
	if(ret < 0) {
		dlog(LOG_ERROR, "Failed to send !errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return NULL;
	}
	DEBUG_INFO();
	/*接收Head*/
	memset(&head, 0, sizeof(head));
	ret = RH_TcpRcvBlockFd(sock_fd, (char *)&head, head_len, &recv_len);
	if(recv_len !=  head_len || ret < 0) {
		dlog(LOG_ERROR, "Failed to send !errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		DEBUG_INFO("recv_len=%d,head_len=%d,ret=%d\n",recv_len,head_len,ret);
		return NULL;
	}
	dlog(LOG_DEBUG, "head.load_len=%d\n", head.load_len);
	/*校验头信息 */
	if(1 != head.msg_result ||  head.load_len <= 0) 
	{
		
	  DEBUG_INFO();
		dlog(LOG_WARN, "Array failed!!!errno=%d\n", head.msg_result);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		return NULL;
	}
	/*获取返回的xml*/
	xml_buf = (char *)malloc(head.load_len + 1);			
	if(!xml_buf) {
		dlog(LOG_WARN, "Array failed!!!errno=%d,xml_buf=%p\n", head.msg_result, xml_buf);
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		DEBUG_INFO();
		return NULL;
	}
	recv_len = 0;
	memset(xml_buf, 0, head.load_len + 1);
	ret = RH_TcpRcvBlockFd(sock_fd, xml_buf, head.load_len, &recv_len);
	if(recv_len != head.load_len || ret < 0) {
		dlog(LOG_ERROR, "Failed to send !errno=%d<%s>\n", errno, strerror(errno));
		RH_Close(__FILE__, (char *)__func__, sock_fd);
		DEBUG_INFO();
		r_free(xml_buf);
		return NULL;
	}
	DEBUG_INFO();
	RH_Close(__FILE__, (char *)__func__, sock_fd);
	/*返回获取成功的xml buffer*/
	return xml_buf;
}




int process_baseIdFindGroupName(int GroupID,char *GroupName)
{
	char nameBuf[128] = {0};
	char groupName[256] = {0};
	sprintf(groupName,"%d",GroupID);
	char *xmlBuf = DBGroup_find_elem(groupName,1,1,100,sizeof(int)*8,0,DB_FIND_GROUP_IDNAME);
	if(xml_get_charNode1("/RequestMsg/MsgBody/GroupInfo/GroupName/",nameBuf,xmlBuf,128) == FALSE)
	{
		r_free(xmlBuf);
		dlog(LOG_ERROR, "/RequestMsg/MsgBody/GroupInfo/GroupName/ is error\n");
		return -40;
	} 
	memcpy(GroupName,nameBuf,strlen(nameBuf)+1);
	r_free(xmlBuf);
	return 0;
}

int process_baseGroupNameFindId(int *GroupID,char *GroupName)
{
	int result = -1;
    dlog(LOG_TRACE, "GroupName=%s",GroupName);
	char *xmlbuf = DBGroup_find_elem(GroupName,1,1, 100,100, 0,DB_FIND_GROUP_MSG);
    dlog(LOG_TRACE, "xmlbuf=%s",xmlbuf);
	if(xml_get_intNode1("/ResponseMsg/MsgHead/Result/",&result,xmlbuf) == FALSE)
	{
		dlog(LOG_ERROR, "/ResponseMsg/MsgHead/Result/ is error\n");
		return -40;
	}
	
	if(xml_get_intNode1("/RequestMsg/MsgBody/GroupInfo/ID/",GroupID,xmlbuf) == FALSE)
	{
		r_free(xmlbuf);
		dlog(LOG_ERROR, "/RequestMsg/MsgBody/GroupInfo/ID/ is error\n");
		return -40;
	}

    dlog(LOG_TRACE, "ID=%d",*GroupID);
	r_free(xmlbuf);
	return 0;
}

#if 1

int share_createGroupInfoList(groupList_ptr HEAD)
{
	int result = -1;
	int from = 0, to = 0;
	char *begin = NULL;
	char *end = NULL;
	char *nodeBegin = NULL;
	char *nodeEnd = NULL;
	char nodeData[256] = {0};
	char bodyData[256] = {0};
	DataBaseGroup_t groupInfo = {0};
	char *xmlbuf = DBGroup_find_elem("",1,1, 100,100, 0,DB_SELECT_GROUP_MSG);
	if(xmlbuf == NULL)
	{
        dlog(LOG_ERROR, "DBGroup_find_elem falied");
		return -1;
	}
	else
	{
		if(xml_get_intNode1("/ResponseMsg/MsgHead/Result/",&result,xmlbuf) == FALSE)
		{
            dlog(LOG_ERROR, "/ResponseMsg/MsgHead/Result/ is error");
			return -40;
		}
	}
	if(result < 0) result = 0;
	end = xmlbuf;
	while(result)
	{
		if(end != NULL) begin = strstr(end,"<GroupInfo>");
		if(begin != NULL)	end = strstr(begin,"</GroupInfo>");
		if(begin == NULL || end == NULL)
		{
			break;
		}
		else
		{
			memset(bodyData,0,sizeof(bodyData));
			memcpy(bodyData,begin,end-begin-1);
			nodeBegin = strstr(bodyData,"<ID>");
			nodeEnd = strstr(bodyData,"</ID>");
			memset(nodeData,0,sizeof(nodeData));
			memcpy(nodeData,nodeBegin,nodeEnd-nodeBegin);
			
			sscanf(nodeData,"%*[^0-9]%[0-9]",nodeData);
			memset(&groupInfo,0,sizeof(DataBaseGroup_t));
			groupInfo.ID = atoi(nodeData);

			nodeBegin = strstr(bodyData,"<GroupName>");
			nodeEnd = strstr(bodyData,"</GroupName>");
			memset(nodeData,0,sizeof(nodeData));
			memcpy(nodeData,nodeBegin,nodeEnd-nodeBegin);
			nodeBegin = strstr(nodeData,">");
			memcpy(groupInfo.GroupName,nodeBegin+1,strlen(nodeBegin+1)+1);
			group_insertNodeToList(HEAD,groupInfo);			
		}
	}
	r_free(xmlbuf);	
	return 0;	

}

#endif

