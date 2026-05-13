
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "nslog.h"
#include "share_socket.h"
#include "share_os.h"

#include <netinet/in.h>
#include <netinet/ip.h>

#include "share_socket.h"
#include "share_os.h"


int g_nslog_status = NS_CLOSE;
int g_debug_status = NS_CLOSE;


#define NSLOG_CONF_GLOBAL		("[global]\ndefault format = \"%D.%ms %V [%t:%p:%f:%U:%L] %m%n\"\nfile perms = 777\n[rules]\n")
#define RECV_UPCARD_NSLOG		("[global]\ndefault format = \"%m\"\nfile perms = 777\n[rules]\n")
#define UPCARD_NSLOG_GLOBAL		("[global]\ndefault format=\"%m\"\n[formats]\nsimple = \"<%d-%ms>%v[%f:%U:%L] %m\"\n[rules]\n")

//#define NSLOG_STATUS_CONFIG "/opt/dvr_rdk/ti816x_2.8/data/nslog_status.conf"
#define NSLOG_STATUS_CONFIG "/opt/rk/nslog/nslog_status.conf"

#if 0
static int nslog_create_configFile(char* nslog_conf,char* nslog_app,char* nslog_out_rule)
{

	char temp[512]={0};
	char temp2[512]={0};
	int  ret=0;
	FILE* fd=NULL;
	fd=fopen(nslog_conf,"w+");
	if(NULL == fd){
		printf("fopen %s failed!\n",nslog_conf);
		return -1;
	}
	printf("nslog_app=%s,nslog_out_rule=%s\n",nslog_app,nslog_out_rule);
	/*Write up-card nslog_config file Only*/
	if(0 == strncmp(nslog_app,UPENC_NSLOG_CNAME,strlen(UPENC_NSLOG_CNAME)) || 0 == strncmp(nslog_app,LIVE_ROOM_NSLOG_CNAME,strlen(LIVE_ROOM_NSLOG_CNAME))){
		printf("The AppName is Up-Card!\n");
		snprintf(temp,sizeof(temp),"%s.* $%s \"recordpath %%c %%D\";simple",nslog_app,nslog_out_rule);
		snprintf(temp2,sizeof(temp2),"%s",UPCARD_NSLOG_GLOBAL);
		printf("1------%s,%s-------",temp,temp2);

	/*Write up-card nslog_config file but recv_up-card nslog config file*/
	}else if(0 == strncmp(nslog_app,FTPSERVE_NSLOG_CNAME,strlen(FTPSERVE_NSLOG_CNAME)) || 0 == strncmp(nslog_app,FILEMANAGEMENT_CNAME,strlen(FILEMANAGEMENT_CNAME)) 
		|| 0 == strncmp(nslog_app,USERMANAGEMENT_NSLOG_CNAME,strlen(USERMANAGEMENT_NSLOG_CNAME)) || 0 == strncmp(nslog_app,STREAMDEAL_NSLOG_CNAME,strlen(STREAMDEAL_NSLOG_CNAME)) 
		|| 0 == strncmp(nslog_app,SD_UPGRADE_NSLOG_CNAME,strlen(SD_UPGRADE_NSLOG_CNAME)) ){
	
		snprintf(temp,sizeof(temp),"%s.DEBUG   %s",nslog_app,nslog_out_rule);
		snprintf(temp2,sizeof(temp2),"%s",NSLOG_CONF_GLOBAL);
		
		printf("2------%s,%s-------",temp,temp2);
	
	}
	/*recv up-card nslog_config only*/
	/*else if( 0 == strncmp(nslog_app,RECV_UPENC_NSLOG_CNAME,strlen(RECV_UPENC_NSLOG_CNAME)) || 0 == strncmp(nslog_app,RECV_LIVE_NSLOG_CNAME,strlen(RECV_LIVE_NSLOG_CNAME))
				||0 == strncmp(nslog_app,RECV_UPGRADE_NSLOG_CNAME,strlen(RECV_UPGRADE_NSLOG_CNAME))){
		snprintf(temp,sizeof(temp),"%s.*   %s",nslog_app,nslog_out_rule);
		snprintf(temp2,sizeof(temp2),"%s",RECV_UPCARD_NSLOG);
		printf("3------%s,%s-------",temp,temp2);
		
	}*/
	ret = fwrite(temp2,strlen(temp2),1,fd);
	if( ret  < 0 ){
		printf("fwrite [%s] failed!!",temp);
		fclose(fd);
		return -1;
	}
	ret = fwrite(temp,strlen(temp),1,fd);
	if( ret  < 0 ){
		printf("fwrite [%s] failed!!",temp);
		fclose(fd);
		return -1;
	}
	printf("%s%s\n",temp2,temp);
	fclose(fd);
	return 0;
}
#endif



/********
 * @decription: 文件不存在则创建文件，设置默认全部开启状态
 * @date: 2021-12-17
 * @author:liuhh
 ********/
#define MAX_SIZE 16
#define MAX_LEN 64
static int control_createConfig_file(void)
{
	int nRet = 0;
	char strBuf[MAX_SIZE][MAX_LEN] = {0};
	int nCount = 0;
	FILE *pf = NULL;
	pf = fopen(NSLOG_STATUS_CONFIG,"w+");
	if(pf == NULL)
	{
		printf("open nslog_status.conf failed!\n");
		return -1;
	}
	snprintf(strBuf[0],MAX_LEN,"[total_flag]:1\n");
	fwrite(strBuf[0],strlen(strBuf[0]),1,pf);

	snprintf(strBuf[1],MAX_LEN,"[daemon]:1\n");
	fwrite(strBuf[1],strlen(strBuf[1]),1,pf);
	snprintf(strBuf[2],MAX_LEN,"[filemanagement]:1\n");
	fwrite(strBuf[2],strlen(strBuf[2]),1,pf);
	snprintf(strBuf[3],MAX_LEN,"[ftp_upload]:1\n");
	fwrite(strBuf[3],strlen(strBuf[3]),1,pf);
	snprintf(strBuf[4],MAX_LEN,"[live]:1\n");
	fwrite(strBuf[4],strlen(strBuf[4]),1,pf);

	snprintf(strBuf[5],MAX_LEN,"[qt]:1\n");
	fwrite(strBuf[5],strlen(strBuf[5]),1,pf);
	snprintf(strBuf[6],MAX_LEN,"[record]:1\n");
	fwrite(strBuf[6],strlen(strBuf[6]),1,pf);
	snprintf(strBuf[7],MAX_LEN,"[rtmpserver]:1\n");
	fwrite(strBuf[7],strlen(strBuf[7]),1,pf);
	snprintf(strBuf[8],MAX_LEN,"[sdupgrade]:1\n");
	fwrite(strBuf[8],strlen(strBuf[8]),1,pf);
	snprintf(strBuf[9],MAX_LEN,"[stream_deal]:1\n");
	fwrite(strBuf[9],strlen(strBuf[9]),1,pf);

	snprintf(strBuf[10],MAX_LEN,"[ftpstandard]:1\n");
	fwrite(strBuf[10],strlen(strBuf[10]),1,pf);
	snprintf(strBuf[11],MAX_LEN,"[usermanagement]:1\n");
	fwrite(strBuf[11],strlen(strBuf[11]),1,pf);
	snprintf(strBuf[12],MAX_LEN,"[repair_file]:1\n");
	fwrite(strBuf[12],strlen(strBuf[12]),1,pf);

	snprintf(strBuf[13],MAX_LEN,"[debug_status]:1\n");
	fwrite(strBuf[13],strlen(strBuf[13]),1,pf);

	snprintf(strBuf[14],MAX_LEN,"[remote]:1\n");
	fwrite(strBuf[14],strlen(strBuf[14]),1,pf);

	snprintf(strBuf[9],MAX_LEN,"[control]:1\n");
	fwrite(strBuf[9],strlen(strBuf[15]),1,pf);

	fclose(pf);
	return nRet;
}

static int control_readNslog_status(const char *pProcessName)
{
	FILE *pf = NULL;
	char *pRet = NULL;
	char strBuf[1024] = {0};
	char strFlagbuf[250] = {0};
	int nFlags = 0;
	int nRet_len =0;
	int nRet = 0;
	static s_count = 0;
	pf = fopen(NSLOG_STATUS_CONFIG,"r");
	if(pf == NULL)
	{
		DEBUG_ERR("====== open file error! ======\n");
		return -1;
	}
	nRet_len = fread(strBuf,sizeof(strBuf),1,pf);
	/*筛选*/
	pRet = strstr(strBuf,pProcessName);
	if(pRet == NULL)
	{
		fclose(pf);
		pf = NULL;
		//printf("====== cannot find this process[%s]! GO TO CREATE CONFIG FIEL ======\n",pProcessName);
		nRet = control_createConfig_file();
		if(nRet < 0)
		{
			DEBUG_ERR("==== create nslog_status file error! ===\n");
			return nRet;
		}
		if(s_count > 0)
		{
			return -2;
		}
		s_count++;
		/*重新查找*/
		nFlags = control_readNslog_status(pProcessName);
		if(nFlags > 0)
		{
			return nFlags;
		}
		else
		{
			DEBUG_ERR("cannot find --");
			return -2;
		}
	}
	sscanf(pRet,"%*[^:]:%s",strFlagbuf);
	nFlags = atoi(strFlagbuf);

	if(pf != NULL)
	{
		fclose(pf);
		pf = NULL;
	}
	return nFlags;
}


static int read_nslog_status()
{
	FILE *fpread = NULL;
	char tmp[10];
	int nslogStatus = 0;
	int ret = 0;

	fpread = fopen(NSLOG_STATUS_CONFIG,"r");
	if(NULL == fpread)
	{
		printf("fopen  nslog config error!: %s\n", strerror(errno));
		ret = -1;
		goto EXIT_FLAG;
	}

	memset(tmp,0,10);

	ret = fread(tmp,1,1,fpread);
	if(ret <= 0)
	{
		ret = -1;
	}
	else
	{
		nslogStatus = atoi(tmp);
		ret = nslogStatus;
	}
EXIT_FLAG:

	if(NULL != fpread)
		fclose(fpread);

	return ret;
}

static int write_nslog_status(int nslogStatus)
{
	FILE *fpwrite = NULL;
	char tmp[10];
	int ret = 0;

	fpwrite = fopen(NSLOG_STATUS_CONFIG,"w");
	if(NULL == fpwrite)
	{
		printf("fopen  nslog config error!: %s\n", strerror(errno));
		ret = -1;
		goto EXIT_FLAG;
	}

	memset(tmp,0,10);

	snprintf(tmp, 10, "%d", nslogStatus);

	fwrite(tmp,strlen(tmp),1,fpwrite);
EXIT_FLAG:
	
	if(NULL != fpwrite)
		fclose(fpwrite);

	return ret;
}
#if 0
static int nslog_mk_rules_file(char *rule_file, char *rule_cname, char *out)
{
	if(!rule_file || !rule_cname) {
		printf("error : rule_file = %p, rule_cname = %p\n", rule_file, rule_cname);
		return -1;
	}

	FILE *fp = NULL;

	fp =	fopen(rule_file, "w+");

	if(NULL == fp) {
		printf("fopen  : %s\n", strerror(errno));
		return -1;
	}

	fprintf(fp, \
	        "[global]\n"\
	        "default format = \"%%D.%%ms%%V [%%t:%%p:%%f:%%U:%%L] %%m%%n\"\n"\
	        "[levels]\n"\
	        "INFO = 40, LOG_INFO\n"\
	        "[rules]\n"\
	        "%s.DEBUG %s\n", \
	        rule_cname, out);
	fclose(fp);
	printf("[nslog_mk_rules_file] end!!!\n");
	//system("sync");
	return 0;
}
#endif
static int s_remote_sockfd = -1;
zlog_category_t *s_remote_handle = NULL;


static int creatZlogSocket()
{
	int sock;
	sock = RH_Socket(__FILE__, (char *)__func__, AF_INET, SOCK_DGRAM, 0);
	s_remote_sockfd  = sock ;
	return sock;
}

static struct sockaddr_in recvAddr;
static  void setZlogSocketDst(char* ip, int port)
{
	recvAddr.sin_family = AF_INET;
	recvAddr.sin_port = htons(port);
	//inet_pton(AF_INET,ip, &(recvAddr.sin_addr.s_addr));
	recvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
}


static int output(zlog_msg_t *msg)
{
	if(NS_OPEN != g_nslog_status)
		return -1;
	int alreadSendLen = 0, leth = 0;
	int sock = s_remote_sockfd;
	if(sock < 0) 
	{
		return -1;
	}
	if(strstr(msg->buf,"NOTICE") == NULL)
	{
		
		return 0;
	}
	while(alreadSendLen < (msg->len)) 
	{
		leth = sendto(sock,msg->buf + alreadSendLen,msg->len - alreadSendLen, 0,(struct sockaddr *)&recvAddr, sizeof(recvAddr));
		if(leth < 0) 
		{
			return -1;
		}
		alreadSendLen += leth;
	}
	//DEBUG_INFO("[%s][%d]:%s", msg->path,msg->len,msg->buf);
	return alreadSendLen;
}


static int creatZlogUDPSend(nslog_conf_info_t *info)
{
	zlog_set_record(info->remote_output_type, output);
	creatZlogSocket();
	setZlogSocketDst(info->ip, info->port);
	return 0;
}

#if 0
int init_remote_zlog(nslog_conf_info_t *remote_info)
{
	int32_t ret = 0;
	zlog_category_t *handle = NULL;
	ret = zlog_init(remote_info->remote_conf_name);
	if (ret) 
	{
		return -1;
	}
	ret = creatZlogUDPSend(remote_info);
	if(ret < 0)
	{
		return -1;
	}
	handle = zlog_get_category(remote_info->remote_rules_name);
	if(handle == NULL) 
	{
		zlog_fini();
		return -1;
	}
	s_remote_handle = handle;	
	return 0;
}

#else
int init_remote_zlog(nslog_conf_info_t *remote_info)
{
	int32_t ret = 0;
	zlog_category_t *handle = NULL;
	ret = dzlog_init(remote_info->remote_conf_name,remote_info->remote_output_type);
	if (ret)
	{
		return ret;
	}
	ret = creatZlogUDPSend(remote_info);
	if(ret < 0)
	{
		return -1;
	}
	return 0;
}
#endif
int NslogInit(nslog_conf_info_t *info)
{
	int rc = -1;
	char freadBuf[64] = {0};
	rc = order_getInfo("df -h|grep /opt/course|awk '{print $1}'",freadBuf, sizeof(freadBuf));
	if(rc <= 0)
	{	
		printf("NsLogInit error\n");
		return -1;
	}
#if 0
    /*当硬件节点不是/dev/sda的时候，会没有打印，去除此判断*/
	sscanf(freadBuf,"%[^ ]",freadBuf);
	char *node = strstr(freadBuf,"\n");
	*node = '\0';	
	
	if(strcmp(freadBuf,"/dev/sda") != 0)
	{	
		return -2;
	}
#endif

		/*取nslog 状态，打开或关闭*/
	// g_nslog_status = read_nslog_status();
	g_nslog_status = control_readNslog_status(TOTAL_FLAGS);
	if(NS_CLOSE == g_nslog_status  )
	{
		DEBUG_INFO("NSlog is NS_CLOSE\n");
		return 0;
	}
	rc = init_remote_zlog(info);
	if(rc != 0)
	{
		DEBUG_ERR("init_remote_zlog ERROR[%d]",rc);
		return rc;
	}
	g_nslog_status = NS_OPEN;
	write_nslog_status(g_nslog_status);
	DEBUG_INFO(g_nslog_status=NS_OPEN);
	return 0;
}

#if 1
static int g_pthread_num = 0 ;
void printf_pthread_create(char *file, char *func)
{
	if(g_pthread_num > 0xffff) {
		g_pthread_num = 0;
	}
	nslog(NS_DEBUG, "pthread %d create in [%s:%s]\n", g_pthread_num++, file, func);
	return ;
}

static int g_pthread_del_num = 0 ;
void printf_pthread_delete(char *file, char *func)
{
	if(g_pthread_del_num > 0xffff) {
		g_pthread_del_num = 0;
	}
	nslog(NS_DEBUG, "pthread %d delete in [%s:%s]\n", g_pthread_del_num++, file, func);
	return ;
}
#endif
