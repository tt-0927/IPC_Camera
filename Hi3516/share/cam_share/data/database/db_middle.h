#ifndef DB_MIDDLE_H

#define DB_MIDDLE_H
#include "record_communication.h"
#include "web_middle_cmd.h"
// #define DB_FILE_DIR "/opt/course/"
// #define DB_FILE_BAKDIR "/opt/course/Bak/"
// #define DB_THUMBPIC_DIR "/var/www/html/pic/vodImg"

#define DB_FILE_DIR "/opt/cam/"
#define DB_FILE_BAKDIR "/opt/cam/Bak/"
#define DB_THUMBPIC_DIR "/data/var/www/html/pic/vodImg"

#define DB_VOD_RANK "/opt/cam/dvr_rdk/ti816x_2.8/.config/user_data/vodRank.conf"

#define DB_SERVER_PORT (0x6021)		  // file
#define DB_USER_SERVER_PORT (0x6022)  // user
#define DB_LOG_SERVER_PORT (0x6023)	  // logSystem
#define DB_GROUP_SERVER_PORT (0x6024) // group

#define DB_DATA_MAX_LENGTH (64)

#define DB_IDENTIFIER 0XBBBBBBBB
#define DB_CREATE_XMLFILE_FLAG 0Xff
#define DB_XML_FILENAME ("/var/www/html/cgi-bin/file.xml")
#define DB_USER_LIST ""

#define MAX_DBFILE_NAME 256
enum DB_MSG_CODE
{
	DB_ORDER_FILELIST_MSG = 0x1001,		   //获取文件列表，按文件大小排序
	DB_SELECT_FILELIST_MSG = 0x1003,	   //查询文件
	DB_DELETE_FILELIST_MSG = 0x1004,	   //删除文件
	DB_INSERT_FILELIST_MSG = 0x1005,	   //插入文件列表
	DB_MODIFY_FILELIST_MSG = 0x1006,	   //修改文件列表
	DB_GET_FTPUPLOAD_MSG = 0x1007,		   //获取ftp上传状态
	DB_SET_FTPUPLOAD_MSG = 0x1008,		   //设置FTP上传状态
	DB_GET_DOWNLOADCNT_MSG = 0x1009,	   //获取下载数
	DB_SET_DOWNLOADCNT_MSG = 0x1010,	   //设置下载数
	DB_CREATE_XML_MSG = 0x1011,			   //生成xml文件
	DB_GET_ALL_FILENAME_MSG = 0x1012,	   //获取文件数据库中所有的文件名
	DB_GET_FILENAME_SUFFIX = 0x1013,	   //获取文件的后缀名,
	DB_GET_VOD_FILENAME_MSG_DIM = 0x1014,  //模糊搜索已发布的视频文件
	DB_GET_GROUP_FILE = 0x1015,			   //获取分组下的文件
	DB_SELECT_GROUP_SELECTFILE = 0x1016,   //搜索分组数据
	DB_SELECT_FILELIST_EXACT_MSG = 0x1017, //精确查找文件
	// DB_MODIFY_GROUP_NUMBER						= 		0x1016,		//修改所属分组
	DB_SELECT_INGO_FILE = 0X1018,	  //根据信息查询相关文件
	DB_MODIFY_COURTINFO_MSG = 0X1019, //修改庭审数据
	// USER DATABASE lx
	DB_DELETE_USER_MSG = 0x2015,  //删除用户
	DB_LIST_USER_MSG = 0x2016,	  //获取用户列表
	DB_SELECT_USER_MSG = 0x2017,  //查找
	DB_INERST_USER_MSG = 0x2018,  //增加用户
	DB_MODIFY_USER_MSG = 0x2019,  //修改用户信息
	DB_FIND_USER_PASSWD = 0x2020, //查找用户的密码
	DB_FIND_USER = 0x2020,
	DB_FIND_USER_GROUP = 0x2021, //根据组id找用户
	// LOG DATABASE lx
	DB_LIST_LOG_MSG = 0x3021, //查找log列表
	DB_ADD_LOG_MSG = 0x3022,  //增加Log列表
	// sDB_LIST_ACCURATE_SEARCH				= 		0x1013,		//精准查找
	//////////////////lx group//////////////////////////////////////
	DB_ADD_GROUP_MSG = 0x4023,	   //增加分组
	DB_FIND_GROUP_MSG = 0x4024,	   //查找分组
	DB_DELETE_GROUP_MSG = 0x4025,  //删除分组
	DB_MODIFY_GROUP_MSG = 0x4026,  //修改分组
	DB_FIND_GROUP_IDNAME = 0x4027, //获取组名
	DB_SELECT_GROUP_MSG = 0x4028,  //模糊查找
	DB_SEARCH_TIME_MSG = 0x4029,   //按时间段查找
	DB_INSERT_RECORD_MSG = 0x4030,	 //插入录制文件信息
	DB_SELECT_RECORD_MSG = 0x4031,   //获取录制文件信息
	DB_DELETE_RECORD_MSG = 0x4032,   //删除录制文件信息
	DB_UPDATE_RECORD_MSG = 0x4033,   //更新录制文件信息
	DB_INSERT_OPERATION_MSG = 0x4040,	 //插入操作日志信息
	DB_SELECT_OPERATION_MSG = 0x4041,   //获取操作日志信息
};

/*
 *	1-修改file_name
 *	2-修改Notes
 *	3-修改CourseTeacher
 * 	4-修改CourseSubject
 *	5-修改FTPupload
 *	6-修改BackupFTPload
 *	7-修改DownloadCnt
 */

enum DB_MODYFY_ITEM
{
	DB_MODIDY_FILE_NAME = 1,
	DB_MODIFY_NOTES = 2,
	DB_MODIDY_COURSE_TEACHER = 3,
	DB_MODIFY_COURSE_SUBJECT = 4,
	DB_MODIDY_FTP_UPLOAD = 5,
	DB_MODIFY_BACKUP_FTPLOAD = 6,
	DB_MODIDY_DOWNLOAD_CNT = 7,
	DB_MODIDY_FTPCLIENTUP_CNT = 8,
	DB_MODIDY_ISSUE_VOD = 9,
	DB_MODIDY_VOD_NUMBER = 10,		//视频点播的次数
	DB_MODIDY_GROUP_NUMBER = 11,	//分组id
	DB_MODIDY_NANE_AND_NUMBER = 12, //网页名字和分组一起修改
	DB_MODIDY_FTPSIZE_CNT = 13,		//修改慧课星数据库文件大小
	DB_MODIDY_VOD_ISPUBLISH = 14,
	DB_MODIDY_FILE_MD5 = 15,
	DB_MODIDY_FILE_SIZE = 16,		  //修改文件大小
	DB_MODIDY_FILE_GROUP_ONEKEY = 17, //一键修改文件用户组
	DB_MODIDY_FTPCLIENT2UP_CNT = 18	  //加一个ftp2 上传标志
};

enum DBUSER_MODYFY_ITEM
{
	DB_MODIDY_USER_User = 1,
	DB_MODIDY_USER_Passwd = 2,
	DB_MODIDY_USER_Jurisdiction_all = 3,
	DB_MODIDY_USER_Jurisdiction_record = 4,
	DB_MODIDY_USER_Jurisdiction_file = 5,
	DB_MODIDY_USER_Jurisdiction_upgrade = 6,
	DB_MODIDY_USER_Jurisdiction_director = 7,
	DB_MODIDY_USER_Remarks = 8,
	DB_MODIDY_USER_Reserve1 = 9,
	DB_MODIDY_USER_Reserve2 = 10,
	DB_MODIDY_USER_Reserve3 = 11,
	DB_MODIDY_USER_Reserve4 = 12,
	DB_MODIDY_USER_Reserve5 = 13,
	DB_MODIDY_USER_Reserve6 = 14
};
enum DBUSER_JURISDICTION
{
	JURISDICTION_ALL = 1,
	JURISDICTION_RECORD = 2,
	JURISDICTION_FILE = 3,
	JURISDICTION_UPGRADE = 4,
	JURISDICTION_DIRECTOR = 5,
};

enum DB_REMOTE_MODYFY_ITEM
{
	DB_MODIDY_IP = 1,
	DB_MODIFY_DEVICE_NAME = 2,
};

typedef struct _DB_Coutr_info_s
{
	char ReferenceID[256];		 //案号
	char CoutrTime[256];		 //开庭时间
	char CauseAction[256];		 //案由
	char CoutrAddr[256];		 //庭审地点
	char PresidingJudge[256];	 //审判长
	char Judge[256];			 //审判员
	char JudgeAssistant[256];	 //法官助理
	char CourtClerk[256];		 //书记员
	char Undertaker[256];		 //承办人
	char PassivityAboveMan[256]; //被上述人
	char ActiveAboveMan[256];	 //上述人
} DB_Coutr_info_s;

typedef struct DataBase_Struct
{
	long long int MovieSize;		   //文件大小(byte)
	//long long int ResSize;		   // Res文件大小(byte)
	int nFileType;			   		   //文件类型
	long long int nStartTime;		   //录制开始时间(秒数)
	char RcdTimeLenght[128];		   //录制时长(s)	00:00:00
	char DirPath[MAX_DBFILE_NAME];	   //文件列表
	char FileName[MAX_DBFILE_NAME];	   //文件列表
	char RcdStartTime[128];			   //录制时间
	char Notes[128];				   //备注
	unsigned short DownloadCnt;		   //下载次数
	unsigned char FTPupload;		   //上传状态 3 -未上传,2-上传失败,1-上传中,0-上传完成
	unsigned char BackupFTPload;	   //备份FTP上传状态 0 -未上传,1-上传中,2-上传失败,3-上传完成
	unsigned int CourseCompleteStatus; //录制完成状态   0-异常课件 1-正常课件
	char CourseTeacher[128];		   //课件授课老师
	char CourseSubject[260];		   //课程名
	char achRoomName[256];			   /* 房间名*/
	int Reserve1;					   // FTP已使用
	int Reserve2;					   //修复字段，若是修复的视频，为1，否则为0
	int Reserve3;					   //用做分组
	int VodNumber;					   //
	//int Reserve4;					   //未使用 
	int nPerTime;					   //分片时长 
	char Reserve5[128];				   // recordId
	int Reserve6;
	int ispublish;			   //是否发布，1-发布
	int RecordMode;			   //录制模式 0 教育  1 庭审
	RepairState_E enRepairStatus;/* 记录录制状态  */
	DB_Coutr_info_s CoutrInfo; //庭审信息
} DataBase_t;

typedef struct RemoteDataBase_Struct
{
	char IP[64];					  //主机ip
	char DeviceName[MAX_DBFILE_NAME]; //设备名
	char CallStartTime[128];		  //呼叫时间
	char CallStatus[128];			  //被呼叫状态
	int CallNumber;					  //呼叫次数
	int Reserve1;					  //保留
	int Reserve2;					  //
	int Reserve3;					  //
	int Reserve4;					  //保留
	int Reserve5;
	int Reserve6;
} RemoteDataBase_t;

typedef struct
{
	char **result;
	int to;
	int from;
	int total;
	int column;
	int pageSize;
} DB_Find_t;

typedef struct DB_Communicate_Head
{
	int identifier; //标识
	int msg_code;	// DB_MSG_CODE
	int msg_result; //处理结果
	int msg_type;	// 0-去程，1-返程
	int load_len;	// msg负载长度
} DB_Communicate_Head;

typedef struct
{
	int from;					  // item起始
	int to;						  // item结束
	int mode;					  // item的项:比如查询的是文件名
	int order;					  //排序方式  0/1-时间降序   2-时间升序  3-大小降序  4-大小升序
	int CurPage;				  //当前页码
	int pageSize;				  //页码大小
	char find[MAX_DBFILE_NAME];	  //查找内容
	char modify[MAX_DBFILE_NAME]; //修改内容
	char recordtime[128];		  //查找时间
	int groupID;
	int flags;				 // 0-模糊查找     1-精确查找
	char RecordEndTime[128]; //无感平台，在 recordtime~RecordEndTime时间段查找文件内容
	DataBase_t item;
} DB_Communicate_t;

enum USER_JURISDICTION
{
	NO_JURISDICTION = 0,
	HAVE_JURISDICTION = 1,
};

typedef struct
{
	int from;		  // item起始
	int to;			  // item结束
	int mode;		  // item的项:比如查询的是文件名
	int CurPage;	  //当前页码
	int pageSize;	  //页码大小
	char find[128];	  //查找内容
	char modify[128]; //修改内容
	DataBaseUser_t item;
} DBUser_Communicate_t;

typedef struct __MODIFY_USER__
{
	char UserName[128];
	char Passwd[128];
	int mode;
	char Jurisdiction_all[16];
	char Jurisdiction_record[16];
	char Jurisdiction_file[16];
	char Jurisdiction_upgrade[16];
	char Jurisdiction_director[16];
	char Remarks[128];
	int GroupID;
} AuthorityModifyUser_t;

typedef struct Record_Court_Chinese_Info
{
	char CourtInfo[128];
	char CourtCase[128];
	char data[128];
	char ReferenceID[128];
	char CoutrTime[128];
	char CauseAction[128];
	char CoutrAddr[128];
	char PresidingJudge[128];
	char Judge[128];
	char JudgeAssistant[128];
	char CourtClerk[128];
	char Undertaker[128];
	char PassivityAboveMan[128];
	char ActiveAboveMan[128];
} Record_Court_Chinese_t;

enum DBGROUP_MODYFY_ITEM
{
	DB_MODIDY_GROUP_ID = 1,
	DB_MODIDY_GROUP_GROUP = 2,
	DB_MODIDY_GROUP_Reserve1 = 3,
	DB_MODIDY_GROUP_Reserve2 = 4,
	DB_MODIDY_GROUP_Reserve3 = 5,
	DB_MODIDY_GROUP_Reserve4 = 6,
	DB_MODIDY_GROUP_Reserve5 = 7,
	DB_MODIDY_GROUP_Reserve6 = 8
};

//==================log system==============================//

typedef struct DataBaseLog_Struct
{
	int Number;
	char Date[64];
	char Type[16]; // ERROR INFO WRAM
	char SoftName[16];
	char LogData[2048]; // log 数据
	int Reserve1;		//保留
	int Reserve2;
	int Reserve3;
	char Reserve4[64];
	char Reserve5[64];
	char Reserve6[64];
} DataBaseLog_t;

typedef struct
{
	int from;					  // item起始
	int to;						  // item结束
	int mode;					  // item的项:比如查询的是文件名
	int CurPage;				  //当前页码
	int pageSize;				  //页码大小
	char find[MAX_DBFILE_NAME];	  //查找内容
	char modify[MAX_DBFILE_NAME]; //修改内容
	DataBaseLog_t item;
} DBLog_Communicate_t;

typedef struct
{
	int from;					  // item起始
	int to;						  // item结束
	int mode;					  // item的项:比如查询的是文件名
	int CurPage;				  //当前页码
	int pageSize;				  //页码大小
	char find[MAX_DBFILE_NAME];	  //查找内容
	char modify[MAX_DBFILE_NAME]; //修改内容
	DataBaseGroup_t item;
} DBGroup_Communicate_t;

typedef struct
{
	char strDirPath[64];		//文件保存路径
	char strFileName[64];		//文件名称
	char strRecordStartSec[16];	//录制文件开始时间（秒数）
	char strRecordStartTime[32];
	char strRcdTimeLenght[32];
	char strFileType[4];
	char strMovieSize[16];
}DB_Record_Data_t, *pDB_Record_Data_t;

typedef struct
{
	int nTotalNumber;
	int nItemNumber;
	char data[];
}DB_Record_Param_t, *pDB_Record_Param_t;

typedef struct
{
	char strFileName[64];
}DB_Record_DeleteName_t, *pDB_Record_DeleteName_t;
typedef struct
{
	int nDeleteNumber;
	char data[];
}DB_Record_Delete_t, *pDB_Record_Delete_t;

typedef struct
{
	char strLogData[16];        //操作行为日期
	char strLogTime[16];        //操作行为时间
	char strTimeSec[16];        //操作行为时间(秒数)
	char strUser[16];           //操作用户
	char strTerminal[16];       //操作终端
	char strOperation[128];     //操作行为
	char strIPAddr[16];         //登录IP地址
	int nLanguage;				//日志中英文 中文：0 英文：1
}DB_Operation_Data_t, *pDB_Operation_Data_t;

typedef struct
{
	int nCurPage;
	int nPageSize;
	int nTotalNumber;
	int nItemNumber;
	int nLanguage;				//日志中英文 中文：0 英文：1
	char data[];
}DB_Operation_Param_t, *pDB_Operation_Param_t;


/*==============================================================================
	函数: <DB_array_RcdStartTime>
	功能: 以RcdStartTime排序
	参数:p_xml :-返回排序后的xml,使用后需要销毁
	返回值:0-成功   否则失败
==============================================================================*/
char *DB_array_RcdStartTime(char *queryName,
							int order,
							int curPage,
							int pageSize, int from, int to);

/*==============================================================================
	函数: <DB_array_FileSize>
	功能: 以FileSize排序
	参数:p_xml :-返回排序后的xml,使用后需要销毁
	返回值:0-成功   否则失败
==============================================================================*/
char *DB_array_FileSize(char *queryName,
						int order,
						int curPage,
						int pageSize, int from, int to);

/*==============================================================================
	函数: <DB_array_RcdTimeLenght>
	功能: 以录制时间长度排序
	参数:p_xml :-返回排序后的xml,使用后需要销毁
	返回值:0-成功   否则失败
==============================================================================*/
char *DB_array_RcdTimeLenght(char *queryName,
							 int order,
							 int curPage,
							 int pageSize, int from, int to);

/*==============================================================================
	函数: <DB_find_FileName>
	功能: 查找name
	参数:p_xml :-返回查找后的xml,-使用后需要销毁-失败返回NULL
			   name  :-需要查找的字段
	返回值:0-成功   否则失败
==============================================================================*/
char *DB_find_FileName(
	char *queryName,
	int order,
	int curPage,
	int pageSize, int to, int from);

/*==============================================================================
	函数: <DB_find_FileInfo>
	功能: 查找name
	参数:p_xml :-返回查找后的xml,-使用后需要销毁-失败返回NULL
			   stQueryInfo  :-需要查找的字段
	返回值:0-成功   否则失败
==============================================================================*/
char *DB_find_FileInfo(Ftp_QueryFile_t stQueryInfo, int to, int from);

/*==============================================================================
	函数: <DB_delete_elem>
	功能: 删除文件名为name的条目
	参数:name  :-需要的FileName
	返回值:0-成功   否则失败
==============================================================================*/
int DB_delete_elem(char *file_name);

/*==============================================================================
	函数: <DB_insert_elem>
	功能: 插入新的elem
	参数:file_name  :-需要的FileName
			   notes:-备注信息
			   moive_size:-电影模式的文件大小
			   res_size:-资源模式的文件大小
			   rce_start_time:-录制起始时间
			   time_lenght:-录制总时长
	返回值:0-成功   否则失败
==============================================================================*/
int DB_insert_elem(Record_CourseInfo_t course_info);
int DB_update_elem(Record_CourseInfo_t course_info);
int DB_modify_fNode(Ftp_ModifyFile_t *modifyFile_info);
int process_baseIdFindGroupName(int GroupID, char *GroupName);

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
int DB_modify_elem(int mode, char *modify, char *file_name);
int DB_modify_elem_court(Record_CourseInfo_t course_info);

int DB_get_all_filename(char *queryName, char **DB_xml);
int DB_get_vod_filename_dim(char *queryName, char **DB_xml, char *fileName);

int DB_get_filename_suffix(char *queryName, int *suffix);
// 0660
char *His_management_finduser(char *userName, int code);
int His_management_findGroup(char *userName, int code);
char *His_management_finduserAll(char *userName, DBUser_Communicate_t userInfo, int code);
int DBUser_insert_elem(DataBaseUser_t DbUserInfo);
int DBUser_delete_elem(char *UserName);
// int DBUser_modify_elem(AuthorityModifyUser_t modifyInfo);
int DBUser_modify_elem(int mode, char *modify, char *userName);
char *His_get_UserInfo(char *UserName);

/////////////////////////log
char *His_management_findLog(char *logSelect, DBLog_Communicate_t logInfo, int code);

/////////////////////////group
int DBGroup_insert_elem(DataBaseGroup_t DbGroupInfo);
int DBGroup_delete_elem(char *DbGroupInfo);
int DBGroup_modify_elem(int mode, char *modify, char *DbGroupInfo);
char *DBGroup_find_elem(char *DbGroupInfo, int mode, int curPage, int pageSize, int to, int from, int code);
char *DB_find_groupFileName(char *queryName, int mode, int curPage, int pageSize, int to, int from);
char *DB_select_groupFile(int mode);
char *DB_select_groupFileName(char *queryName, int mode, int curPage, int pageSize, int to, int from, int groupID);
char *DB_findExect_FileName(char *queryName, int mode, int curPage, int pageSize, int to, int from);
int process_baseGroupNameFindId(int *GroupID, char *GroupName);

int DBFTP_insert_elem(char *file_name, char *notes, long long int movie_size, long long int res_size, const char *rce_start_time, char *time_lenght, char *teacher, char *course);

#endif
