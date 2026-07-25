#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include "sqlite3.h"
#include "cJSON.h"
#include "dlog.h"


#define MAX_SQL_LEN                 2048
#define DB_NAME               "faceRecognition.db"
#define DB_PATH               "/opt/bl/db"
#define DB_FEATURE_INFO       "featureInfo"

/*--------------------------------字段类型---------------------------------------------------------------*/
#define INT                 "int"
#define VARCHAR             "varchar(128)"

/*-------------------------------人脸识别表字段名-----------------------------------------------*/
#define FEATURESID            "featuresID"
#define NAME                  " Name"
#define POSX1                 " PosX1"
#define POSY1                 " PosY1"
#define POSX2                 " PosX2"
#define POSY2                 " PosY2"
#define DATA                  " Data"


/*
 *@description 打开数据库
 *@Author: suzhl
 *@param[in] dbName 数据库名称
 *@return 成功返回0，失败返回错误码
*/
int open_db(char *dbName);

/*
 *@description 关闭数据库
 *@Author: suzhl
 *@return 成功返回0，失败返回错误码
*/
int close_db();

/* 
  *@description 创建数据库表 
  *@Author: suzhl
  *@param[in] tableName 表名
  *@param[in] dbKey 表的主键 格式（主键名 数据类型,主键名2 数据类型），数据类型有test、integer、real、varchar(100)等
  *@return 成功返回0，失败返回错误码
*/
int create_db(char *tableName);

/* 
  *@description 添加字段 
  *@Author: suzhl
  *@param[in] tableName 表名
  *@param[in] columnName 字段名
  *@param[in] dataType 字段数据类型
  *@return 成功返回0，失败返回错误码
*/
int add_column_to_db(char *tableName,char *columnName,char *dataType);

/* 
  *@description 插入数据库 
  *@Author: suzhl
  *@param[in] tableName 表名
  *@param[in] jsonObj 数据内容
  *@return 成功返回0，失败返回错误码
*/
int insert_db(char *tableName, cJSON *jsonObj);

/* 
  *@description 删除数据库内容
  *@Author: suzhl
  *@param[in] tableName 表名
  *@param[in] cond 删除索引（例如 snprintf(cond, sizeof(cond), "id = 1"))
  *@return 成功返回0，失败返回错误码
*/
int delete_db(char *tableName, char *cond);

/* 
  *@description 查询数据库内容
  *@Author: suzhl
  *@param[in] tableName 表名
  *@param[in] field 查询内容(为NULL查询所有字段内容，查询内容例如 "type,teacher")
  *@param[in] cond 查询索引（例如 snprintf(cond, sizeof(cond), "id = 1"))
  *@return 返回查询到的数据库内容
*/
cJSON *query_db(char *tableName,char * field,char *cond);
cJSON *query_all_db(char *tableName);
int query_first_db(char *filed,char *tableName,char **data);
int query_first_flag_db(char *filed,char *tableName,char *cond,char **data);
/* 
  *@description 更新数据库内容
  *@Author: suzhl
  *@param[in] tableName 表名
  *@param[in] field 更新内容(例如 "flag=1")
  *@param[in] cond 查询索引（例如 snprintf(cond, sizeof(cond),"id = 1 AND upload_flag=0))
  *@return 成功返回0，失败返回错误码
*/
int update_db(char *tableName, char *field, char *cond);

int table_exists_callback(void *data, int argc, char **argv, char **azColName);

int db_init(char *dbName,char * tableName);

cJSON *query_all_db(char *tableName);