/*
NodeServer-log module
*/

#ifndef __NSLOG_H__
#define __NSLOG_H__
#include "zlog.h"


#ifdef __cplusplus
extern "C"
{
#endif

#define NS_NONE				"\033[m"
#define NS_RED				"\033[0;32;31m"
#define NS_LIGHT_RED		"\033[1;31m"
#define NS_GREEN			"\033[0;32;32m"
#define NS_LIGHT_GREEN		"\033[1;32m"
#define NS_BLUE				"\033[0;32;34m"
#define NS_LIGHT_BLUE		"\033[1;34m"
#define NS_DARY_GRAY		"\033[1;30m"
#define NS_CYAN				"\033[0;36m"
#define NS_LIGHT_CYAN		"\033[1;36m"
#define NS_PURPLE			"\033[0;35m"
#define NS_LIGHT_PURPLE		"\033[1;35m"
#define NS_BROWN			"\033[0;33m"
#define NS_YELLOW			"\033[1;33m"
#define NS_LIGHT_GRAY		"\033[0;37m"
#define NS_WHITE			"\033[1;37m"


#define NS_DEBUG 		ZLOG_LEVEL_DEBUG
#define NS_INFO  		ZLOG_LEVEL_INFO
#define NS_NOTICE	ZLOG_LEVEL_NOTICE
#define NS_WARN		ZLOG_LEVEL_WARN
#define NS_ERROR 	ZLOG_LEVEL_ERROR
#define NS_FATAL 		ZLOG_LEVEL_FATAL

#define NS_ALL 2
#define NS_OPEN 1
#define NS_CLOSE 0

/*配置文件的字段*/
#define TOTAL_FLAGS "[total_flag]"
#define DEBUG_FLAGS "[debug_status]"

#if 0
/*HD: Upgrade Zlog config*/
#define HD_UPGRADE_NSLOG_CONF		("/opt/dvr_rdk/ti816x_2.8/zlog_conf/HDupgrade_log.conf")
#define HD_UPGRADE_NSLOG_CNAME		("HDupgrade")
#define HD_UPGRADE_NSLOG_OUT		("HDupgrade_send")
#endif

#ifndef __USE_DEBUG
#define __USE_DEBUG

#define USE_DEBUG

#ifdef USE_DEBUG
#define DEBUG_LINE() printf("[%s:%s] line=%d\r\n",__FILE__, __func__, __LINE__)
#define DEBUG_ERR(fmt, args...) printf( NS_LIGHT_RED "[%s:%s:%d]" #fmt NS_NONE "\r\n",__FILE__,__func__, __LINE__, ##args)
#define DEBUG_INFO(fmt, args...) printf( NS_YELLOW "[%s:%s:%d]" #fmt NS_NONE "\r\n",__FILE__,__func__,__LINE__, ##args)
#else
#define DEBUG_LINE()
#define DEBUG_ERR(fmt, ...)
#define DEBUG_INFO(fmt, ...)
#endif

#endif



#if 0
/*HD:MAINEnc_App log config*/
#define HD_APP_NSLOG_CONF		("/opt/dvr_rdk/ti816x_2.8/zlog_conf/HDapp_log.conf")
#define HD_APP_NSLOG_CNAME		("HDApp")
#define  HD_APP_NSLOG_OUT		("\"/var/log/recserver/HDapp_log.%d(%u).log\", 30MB *2 ~ \"/var/log/recserver/HDapp_log.%d(%u).#2r.log\"")
#endif



/*SD:Enc_App log config*/
#define UPENC_NSLOG_CONF		("/opt/dvr_rdk/ti816x_2.8/zlog_conf/enc_log.conf")
#define UPENC_NSLOG_CNAME		("edukit")
#define UPENC_NSLOG_OUT			("\"/opt/course/log/edukit.%d(%u).log\", 30MB *2 ~ \"/opt/course/log/edukit.%d(%u).#2r.log\"")











//=====================-----------------------0660------------------------------========================================================================


/*SD:rtmpserver*/
#define RTMPSERVE_NSLOG_CONF		("/opt/rk/nslog/rtmpserver.conf")
#define RTMPSERVE_NSLOG	("rtmpserver")
#define RTMPSERVE_NSLOG_OUT		("\"/opt/course/log/rtmpserver.%d(%u).log\", 30MB *2 ~ \"/opt/course/log/rtmpserver.%d(%u).#2r.log\"")

/*FTP:ftp_standard*/
#define FTPSERVE_NSLOG_OUT		("\"/opt/course/log/ftpstandard.%d(%u).log\", 30MB *2 ~ \"/opt/course/log/ftpstandard.%d(%u).#2r.log\"")
#define FTPSERVE_NSLOG_CONF		("/opt/rk/nslog/ftpstandard.conf")
#define FTPSERVE_NSLOG		("ftpstandard")


/*SD:stream_deal log config*/
#define STREAMDEAL_NSLOG_OUT		("\"/opt/course/log/stream.%d(%u).log\", 30MB *2 ~ \"/opt/course/log/stream.%d(%u).#2r.log\"")
#define STREAMDEAL_NSLOG_CONF		("/opt/rk/nslog/stream.conf")
#define STREAMDEAL_NSLOG		("stream")

/*SD:upgrade log config*/
#define SD_UPGRADE_NSLOG_CONF		("/opt/rk/nslog/sd_upgrade.conf")
#define SD_UPGRADE_NSLOG		("sdupgrade")
#define SD_UPGRADE_NSLOG_OUT		("\"/opt/course/log/sdupgrade.%d(%u).log\", 30MB *2 ~ \"/opt/course/log/sdupgrade.%d(%u).#2r.log\"")

/*SD:file_management*/
#define FILEMANAGEMENT_OUT		("\"/opt/course/log/filemanagement.%d(%u).log\", 30MB *2 ~ \"/opt/course/log/filemanagement.%d(%u).#2r.log\"")
#define FILEMANAGEMENT_NSLOG_CONF		("/opt/rk/nslog/filemanagement.conf")
#define FILEMANAGEMENT_NSLOG	("filemanagement")

/*HD:Live log config*/
#define LIVE_ROOM_NSLOG_CONF		("/opt/rk/nslog/live.conf")
#define LIVE_ROOM_NSLOG		("live")
#ifdef SEND_LOG
#define LIVE_ROOM_NSLOG_OUT			("live_send")
#else
#define LIVE_ROOM_NSLOG_OUT			("\"/opt/course/log/live.%d(%u).log\", 30MB *2 ~ \"/opt/course/log/live.%d(%u).#2r.log\"")
#endif

/*control log config*/
#define CONTROL_NSLOG_OUT		("\"/opt/course/log/control.%d(%u).log\", 30MB *2 ~ \"/opt/course/log/control.%d(%u).#2r.log\"")
#define CONTROL_NSLOG_CONF		("/opt/rk/nslog/control.conf")
#define CONTROL_NSLOG		("control")

/*remote log config*/
#define REMOTE_NSLOG_OUT		("\"/opt/course/log/remote.%d(%u).log\", 30MB *2 ~ \"/opt/course/log/remote.%d(%u).#2r.log\"")
#define REMOTE_NSLOG_CONF		("/opt/rk/nslog/remote.conf")
#define REMOTE_NSLOG		("remote")

/*SD:Record log config*/
#define RECORD_NSLOG_CONF		("/opt/rk/nslog/record.conf")
#define RECORD_NSLOG		("record")
#define RECORD_NSLOG_OUT		("\"/opt/course/log/record.%d(%u).log\", 30MB *2 ~ \"/opt/course/log/record.%d(%u).#2r.log\"")

//SD:user_management
#define USERMANAGEMENT_NSLOG_CONF		("/opt/rk/nslog/usermanagement.conf")
#define USERMANAGEMENT_NSLOG	("usermanagement")
#define USERMANAGEMENT_NSLOG_OUT		("\"/opt/course/log/usermanagement.%d(%u).log\", 30MB *2 ~ \"/opt/course/log/usermanagement.%d(%u).#2r.log\"")



/*FTP:ftp_upload*/
#define FTPUPLOAD_NSLOG_CONF		("/opt/rk/nslog/ftp_upload.conf")
#define FTPUPLOAD_NSLOG		("ftp_upload")
#define FTPUPLOAD_NSLOG_OUT		("\"/opt/course/log/ftp_upload.%d(%u).log\", 30MB *2 ~ \"/opt/course/log/ftp_upload.%d(%u).#2r.log\"")

#define AI_NSLOG_CONF		("/opt/rk/nslog/ai.conf")
#define AI_NSLOG		("ai")
#define AI_NSLOG_OUT		("\"/opt/course/log/ai.%d(%u).log\", 30MB *2 ~ \"/opt/course/log/ai.%d(%u).#2r.log\"")

/*daemon*/
#define DAEMON_NSLOG_CONF		("/opt/rk/nslog/daemon.conf")
#define DAEMON_NSLOG		("daemon")
#define DAEMON_NSLOG_OUT		("\"/opt/course/log/daemon.%d(%u).log\", 30MB *2 ~ \"/opt/course/log/daemon.%d(%u).#2r.log\"")

/*repaire*/
#define REPAIRE_NSLOG_CONF		("/opt/rk/nslog/repair_file.conf")
#define REPAIRE_NSLOG			("repair_file")
#define REPAIRE_NSLOG_OUT		("\"/opt/course/log/repair_file.%d(%u).log\", 30MB *2 ~ \"/opt/course/log/repair_file.%d(%u).#2r.log\"")


#if 0
extern int ctrl_room_id;
#define nslog(nLevel, format, args...) \
	dzlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	nLevel, "[ROOM_ID:%d]" format, ctrl_room_id, ##args)
#endif
extern zlog_category_t *s_remote_handle;
extern int g_nslog_status;
extern int g_debug_status;

#if 0
#define nslog(nLevel, format, args...) \
	do{\
		if(NS_OPEN == g_nslog_status)\
		{ \
			switch(nLevel) {\
				case NS_WARN:\
					if( NULL == s_remote_handle){\
						dzlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
						nLevel, NS_YELLOW format NS_NONE, ##args); \
					}else{ zlog_warn(s_remote_handle, format,##args);}\
					break;\
				case NS_ERROR:\
				case NS_FATAL:\
					if( NULL == s_remote_handle){\
						dzlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
						nLevel, NS_RED format NS_NONE, ##args); \
					}else{ zlog_error(s_remote_handle, format,##args);}\
					break;\
				case NS_INFO:\
					if( NULL == s_remote_handle){\
						dzlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
						nLevel, NS_GREEN format NS_NONE, ##args); \
					}else{zlog_info(s_remote_handle, format,##args);}\
					break;\
				case NS_DEBUG:\
				default:\
					if( NULL == s_remote_handle){\
						dzlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
						nLevel, format, ##args); \
					}else{ zlog_info(s_remote_handle, format,##args);}\
					break;\
			}\
		}\
	}while(0)
#else
#define nslog(nLevel, format, args...) \
	do{\
		if(NS_OPEN == g_nslog_status)\
		{ \
			switch(nLevel) {\
				case NS_NOTICE:\
					dzlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__,nLevel, NS_LIGHT_PURPLE format NS_NONE, ##args); \
					if( NULL != s_remote_handle){zlog_notice(s_remote_handle, format,##args);}break;\
				case NS_WARN:\
					dzlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__,nLevel, NS_YELLOW format NS_NONE, ##args); \
					if( NULL != s_remote_handle){zlog_warn(s_remote_handle, format,##args);}break;\
				case NS_ERROR:\
				case NS_FATAL:\
					dzlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__,nLevel, NS_RED format NS_NONE, ##args); \
					if( NULL != s_remote_handle){zlog_error(s_remote_handle, format,##args);}break;\
				case NS_INFO:\
					dzlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__,nLevel, NS_GREEN format NS_NONE, ##args); \
					if( NULL != s_remote_handle){zlog_info(s_remote_handle, format,##args);}break;\
				case NS_DEBUG:\
				default:\
					dzlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__,nLevel, format, ##args); \
					if( NULL != s_remote_handle){zlog_info(s_remote_handle, format,##args);}break;\
			}\
		}\
		else\
		{\
			printf(format, ##args);\
		}\
	}while(0)
#endif

#define nslog_debug(format,args...) \
	{\
		if(NS_ALL == g_debug_status)\
		{\
			printf("%s %s DEBUG [%s:%s:%d]: ",__DATE__,__TIME__,__FILE__,__func__,__LINE__);\
			printf(format,##args);\
		}\
		else if(NS_OPEN == g_debug_status)\
		{\
			nslog(NS_ERROR, format, ##args);\
		}\
	}
/*nslog_btime(前)和nslog_etime(后)要配套使用调用顺序不能乱
 *作用是同时代码段的运行时间
 *ndiff_time是打印的差值时间
 * */
#define nslog_btime() {\
	int nTimeBegin=0; \
	nTimeBegin=OS_getSysTimeInMsec();

#define nslog_etime(ndiff_time, format, args...) \
	int nTimeEnd=0; \
	nTimeEnd=OS_getSysTimeInMsec(); \
	if(nTimeEnd - nTimeBegin >= ndiff_time)\
	{\
		if(NS_ALL == g_debug_status)\
		{\
			printf("%s %s RUN_TIME [%s:%s:%d]:run  time %d :",__DATE__,__TIME__,__FILE__,__func__,__LINE__,nTimeEnd - nTimeBegin);\
			printf(format,##args);\
		}\
		else if(NS_OPEN == g_debug_status)\
		{\
			nslog(NS_ERROR, "run time %d: "#format, nTimeEnd - nTimeBegin,##args);\
		}\
	}\
}

/* nslog_btime(后)和nslog_etime(前)要配套使用调用顺序不能乱
 *作用用来统计离开后进入函数的时间间隔
 *ndiff_time是打印的差值时间
 * */
#define nslog_estime(ndiff_time, format, args...) {\
	static int nsTimeEnd=0; \
	static int nsTimeBegin=0; \
	nsTimeEnd=OS_getSysTimeInMsec(); \
	if(nsTimeEnd - nsTimeBegin >= ndiff_time)\
	{\
		if(NS_ALL == g_debug_status)\
		{\
			printf("%s %s RUN_STATIC_TIME [%s:%s:%d]:run static time %d :",__DATE__,__TIME__,__FILE__,__func__,__LINE__,nsTimeEnd - nsTimeBegin);\
			printf(format,##args);\
		}\
		else if(NS_OPEN == g_debug_status)\
		{\
			nslog(NS_ERROR, "run static time %d: "#format, nsTimeEnd - nsTimeBegin,##args);\
		}\
	}
#define nslog_bstime() \
	nsTimeBegin=OS_getSysTimeInMsec();\
}

typedef struct nslog_conf_info_ {
	char conf_name[512];
	char rules_name[32];
	char output_type[512];
	char remote_conf_name[512];
	char remote_rules_name[32];
	char remote_output_type[512];
	char ip[16];
	unsigned short port;
	char strProcessName[64];              /*进程名字*/
}nslog_conf_info_t;

int NslogInit(nslog_conf_info_t *info);
void printf_pthread_create(char *file,char *func);
void printf_pthread_delete(char *file,char *func);

#define	NslogFini		zlog_fini

//add by zm
#ifndef PRINTF
#define PRINTF(X...)
#endif

#ifndef ERR_PRN
#define ERR_PRN(X...)
#endif

#ifndef WARN_PRN
#define WARN_PRN(X...)
#endif


#ifdef X86
#define nslog(nLevel, format, args...) \
	do{\
		switch(nLevel) {\
			case NS_WARN:\
			case NS_ERROR:\
			case NS_FATAL:\
			case NS_INFO:\
			case NS_DEBUG:\
			default:\
				printf(format, ##args);\
				break;\
		}\
	}while(0)
#endif

#ifdef __cplusplus
}
#endif

#endif //__NSLOG_H__

