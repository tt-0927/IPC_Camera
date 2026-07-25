#include "ctrl_db.h"


static sqlite3 *gDbHandle = NULL;/*数据库句柄*/
static char *zErrMsg = NULL;/*错误信息*/

/*
 *@description 打开数据库
 *@Author: suzhl
 *@param[in] dbName 数据库名称
 *@return 成功返回0，失败返回错误码
*/
int open_db(char *dbName) 
{
    int nRet = -1;
    if(gDbHandle == NULL) 
    {
        nRet = sqlite3_open(dbName, &gDbHandle); /*打开指定的数据库文件,如果不存在将创建一个同名的数据库文件*/  
        if(nRet != 0) 
        {  
            dlog(LOG_ERROR,"Can't open database [%s]\n",sqlite3_errmsg(gDbHandle));  
            sqlite3_close(gDbHandle);  
        }  
    }
    return nRet;
}
/*
 *@description 关闭数据库
 *@Author: suzhl
 *@return 成功返回0，失败返回错误码
*/
int close_db() 
{
    int nRet = -1;
    nRet = sqlite3_close(gDbHandle); /*关闭数据库*/
    if(nRet != 0)
    {
        dlog(LOG_ERROR,"Can't close database [%s]\n",sqlite3_errmsg(gDbHandle));
    }
    else
    {
        gDbHandle = NULL;
    }
    return nRet;
}
/* 
  *@description 创建数据库表 
  *@Author: suzhl
  *@param[in] tableName 表名
  *@param[in] dbKey 表的主键 格式（主键名 数据类型,主键名2 数据类型），数据类型有test、integer、real、varchar(100)等
  *@return 成功返回0，失败返回错误码
*/
int create_db(char *tableName) 
{
    char sql[MAX_SQL_LEN] = {0};
    int nRet = -1;
    if(gDbHandle == NULL)
    {
        return -1;
    }
    snprintf(sql, MAX_SQL_LEN, "CREATE TABLE IF NOT EXISTS %s (id INT PRIMARY KEY);", tableName);
    nRet = sqlite3_exec(gDbHandle, sql , 0 , 0 , &zErrMsg); 
    if(nRet != 0) 
    {
        dlog(LOG_ERROR,"创建数据库表失败[%s]\n",zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(gDbHandle);
    }
    return nRet;  
}
/* 
  *@description 添加字段 
  *@Author: suzhl
  *@param[in] tableName 表名
  *@param[in] columnName 字段名
  *@param[in] dataType 字段数据类型
  *@return 成功返回0，失败返回错误码
*/
int add_column_to_db(char *tableName,char *columnName,char *dataType)
{
    char sql[MAX_SQL_LEN] = {0};
    int nRet = -1;
    if(gDbHandle == NULL)
    {
        return nRet;
    }
    snprintf(sql, MAX_SQL_LEN, "PRAGMA table_info(%s);",columnName);
    nRet = sqlite3_exec(gDbHandle, sql , 0 , 0 , &zErrMsg);
    if (nRet != 0)
    {
        dlog(LOG_ERROR,"无法获取表结构 [%s]\n",zErrMsg);
        sqlite3_free(zErrMsg);
        return nRet;
    }
    else
    {
        int nHasField = 0;
        sqlite3_stmt *stmt;
        nRet = sqlite3_prepare_v2(gDbHandle, sql, -1, &stmt, 0);
        if (nRet == 0)
        {
            while (sqlite3_step(stmt) == SQLITE_ROW)
            {
                const unsigned char *fieldName = sqlite3_column_text(stmt, 1);
                if (fieldName && strcmp((const char*)fieldName, columnName) == 0)
                {
                    nHasField = 1;
                    break;
                }
            }
        }
        if (!nHasField)
        {
            snprintf(sql, MAX_SQL_LEN, "ALTER TABLE %s ADD COLUMN %s %s;",tableName,columnName,dataType);  
            nRet = sqlite3_exec(gDbHandle, sql , 0 , 0 , &zErrMsg); 
            if(nRet != 0) {
                //dlog(LOG_ERROR,"Can't add columnName [%s]\n",zErrMsg);
                sqlite3_free(zErrMsg);
                sqlite3_close(gDbHandle);
            }
        }
    }
    return nRet;
}



/* 
  *@description 插入数据库 
  *@Author: suzhl
  *@param[in] tableName 表名
  *@param[in] jsonObj 数据内容
  *@return 成功返回0，失败返回错误码
*/
int insert_db(char *tableName, cJSON *jsonObj) 
{
    char sql[MAX_SQL_LEN], keyStr[MAX_SQL_LEN] = {0}, valueStr[MAX_SQL_LEN] = {0};
    int nRet = -1;
    if(gDbHandle == NULL)
    {
        printf("imagePath=====nRet= %d\n", nRet);

        return nRet;

    }

    //遍历结构体json串,以键值对的方式去插入数据库
	for (int i = 0; i < cJSON_GetArraySize(jsonObj); i++)
    {
		cJSON* item = cJSON_GetArrayItem(jsonObj, i);

        if (item != NULL && cJSON_IsObject(item))
        {

            if (item->type == cJSON_Object)
            {
                cJSON *field = item->child;
                while (field != NULL) 
                {

                    if (cJSON_IsString(field)) 
                    {
                        snprintf(keyStr + strlen(keyStr), MAX_SQL_LEN, ",%s", field->string);
                        snprintf(valueStr + strlen(valueStr), MAX_SQL_LEN, ",\'%s\'", field->valuestring);
                    }
                    else if (cJSON_IsNumber(field))
                    {
                        snprintf(keyStr + strlen(keyStr), MAX_SQL_LEN, ",%s", field->string);
                        snprintf(valueStr +strlen(valueStr), MAX_SQL_LEN, ",%d", field->valueint);
                    }
                    field = field->next;
                }
               
            }
           
        }
        
    }
    snprintf(sql, MAX_SQL_LEN, "INSERT INTO %s (%s) VALUES(%s);", tableName, keyStr+1, valueStr+1);
    dlog(LOG_ERROR,"sql [%s]\n",sql);  
    nRet = sqlite3_exec(gDbHandle, sql , 0 , 0 , &zErrMsg);

    if (nRet != 0)
    {
        dlog(LOG_ERROR,"Can't insert database [%s]\n",zErrMsg);
    } 
    return nRet;
}
/* 
  *@description 删除数据库内容
  *@Author: suzhl
  *@param[in] tableName 表名
  *@param[in] cond 删除索引（例如 snprintf(cond, sizeof(cond), "id = 1"))
  *@return 成功返回0，失败返回错误码
*/
int delete_db(char *tableName, char *cond) 
{
    char sql[MAX_SQL_LEN] = {0};
    int nRet = -1;
    if(gDbHandle == NULL)
        return -1;
    if(cond == NULL)
        cond = "1=1";
    sprintf(sql, "DELETE FROM %s WHERE %s;", tableName, cond);
    nRet = sqlite3_exec(gDbHandle, sql , 0 , 0 , &zErrMsg );
    if(nRet != 0) {
        dlog(LOG_ERROR,"Can't delete database [%s]\n",zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(gDbHandle);
    }
    return nRet;   
}

/* 
  *@description 查询数据库内容
  *@Author: suzhl
  *@param[in] tableName 表名
  *@param[in] field 查询内容(为NULL查询所有字段内容，查询内容例如 "type,teacher")
  *@param[in] cond 查询索引（例如 snprintf(cond, sizeof(cond), "id = 1"))
  *@return 返回查询到的数据库内容
*/
cJSON *query_db(char *tableName,char * field,char *cond) 
{
    int nrow = 0, ncolumn = 0, i = 0, j = 0;  
    char **azResult; /*二维数组存放结果*/
    char sql[MAX_SQL_LEN] = {0};
    cJSON * array = NULL;
    if(gDbHandle == NULL)
        return NULL;
    /*查询数据*/  
    if(cond == NULL)
        cond = "1=1";
    if(field == NULL)
        snprintf(sql, MAX_SQL_LEN, "SELECT * FROM %s WHERE %s", tableName, cond);
    else
        snprintf(sql, MAX_SQL_LEN, "SELECT %s FROM %s WHERE %s", field, tableName, cond);
    sqlite3_get_table(gDbHandle, sql , &azResult , &nrow , &ncolumn , &zErrMsg);
    if(nrow > 0 && ncolumn > 0) {
        array = cJSON_CreateArray();
        for( i=1 ; i<nrow+1 ; i++ ) {
            cJSON * item =  cJSON_CreateObject();
            for( j=0 ; j<ncolumn; j++ )
                cJSON_AddItemToObject(item, azResult[j], cJSON_CreateString(azResult[i*ncolumn + j]));
            cJSON_AddItemToArray(array,item);
        }
        sqlite3_free_table(azResult);
    }
    return array;
}
/* 
  *@description 查询数据库所有内容
  *@Author: suzhl
  *@param[in] tableName 表名
  *@return 返回查询到的数据库内容
*/
cJSON *query_all_db(char *tableName)
{
    int nrow = 0, ncolumn = 0, i = 0, j = 0;/*行和列*/
    char **azResult;/*二维数组存放结果*/
    int nRet = -1;
    char sql[MAX_SQL_LEN] = {0};
    cJSON * array = NULL;
    snprintf(sql, MAX_SQL_LEN, "SELECT * FROM %s", tableName);
    dlog(LOG_ERROR,"查询的sql [%s]\n",sql);
    nRet = sqlite3_get_table(gDbHandle, sql, &azResult, &nrow, &ncolumn, &zErrMsg);
    if (nRet == 0)
    {
       if(nrow > 0 && ncolumn > 0)
        {
            array = cJSON_CreateArray();
            for( i=1 ; i<nrow+1 ; i++ ) {
            cJSON * item =  cJSON_CreateObject();
            for( j=0 ; j<ncolumn; j++ )
                cJSON_AddItemToObject(item, azResult[j], cJSON_CreateString(azResult[i*ncolumn + j]));
            cJSON_AddItemToArray(array,item);//向数组节点添加结构体节点
        }
        sqlite3_free_table(azResult);
        }
    } else
    {
       dlog(LOG_ERROR,"Can't find database [%s]\n",zErrMsg); 
    }
    return array;
}
/* 
  *@description 查询数据库最早的内容
  *@Author: suzhl
  *@param[in] tableName 表名
  *@param[out] dataJson 返回查询到的内容json串（dataJson需要在外部free掉）
  *@return 成功返回0，失败返回错误码
*/
int query_first_db(char *filed,char *tableName,char **data)
{
    int nrow = 0, ncolumn = 0, i = 0;
    int nRet = -1;
    char sql[MAX_SQL_LEN] = {0};
    char **azResult;/*二维数组存放结果*/
    cJSON * item = NULL;
    snprintf(sql, MAX_SQL_LEN, "SELECT * FROM %s WHERE %s = (SELECT MIN(%s) FROM %s)",tableName,filed,filed,tableName);
    nRet = sqlite3_get_table(gDbHandle, sql, &azResult, &nrow, &ncolumn, &zErrMsg);
    if (nRet == SQLITE_OK)
    {
        item = cJSON_CreateObject();
        if (nrow > 0 && ncolumn > 0)
        {
            for(i = 0;i < ncolumn;i++)
            {
                cJSON_AddStringToObject(item, azResult[i], azResult[ncolumn + i]);
            }
            sqlite3_free_table(azResult);
        } else{
            //dlog(LOG_ERROR,"获取最早的数据失败\n");
            sqlite3_free(zErrMsg); 
            return -1;
        }
    }  
    *data = cJSON_Print(item);
    return nRet;
}
/* 
  *@description 查询数据库最早的没过期内容
  *@Author: suzhl
  *@param[in] tableName 表名
  *@param[out] dataJson 返回查询到的内容json串（dataJson需要在外部free掉）
  *@return 成功返回0，失败返回错误码
*/
int query_first_flag_db(char *filed,char *tableName,char *cond,char **data)
{
    int nrow = 0, ncolumn = 0, i = 0;
    int nRet = -1;
    char sql[MAX_SQL_LEN] = {0};
    char **azResult;/*二维数组存放结果*/
    cJSON * item = NULL;
    if(cond == NULL)
        cond = "1=1";
    snprintf(sql, MAX_SQL_LEN, "SELECT * FROM %s WHERE %s = (SELECT MIN(%s) FROM %s WHERE %s)",tableName,filed,filed,tableName,cond);
    nRet = sqlite3_get_table(gDbHandle, sql, &azResult, &nrow, &ncolumn, &zErrMsg);
    if (nRet == SQLITE_OK)
    {
        item = cJSON_CreateObject();
        if (nrow > 0 && ncolumn > 0)
        {
            for(i = 0;i < ncolumn;i++)
            {
                cJSON_AddStringToObject(item, azResult[i], azResult[ncolumn + i]);
            }
            sqlite3_free_table(azResult);
        } else{
            //dlog(LOG_ERROR,"获取最早的数据失败\n");
            sqlite3_free(zErrMsg);
            return -1; 
        }
    }  
    *data = cJSON_Print(item);
    return nRet;
}
/* 
  *@description 更新数据库内容
  *@Author: suzhl
  *@param[in] tableName 表名
  *@param[in] field 更新内容(例如 "flag=1")
  *@param[in] cond 查询索引（例如 snprintf(cond, sizeof(cond),"id = 1 AND upload_flag=0))
  *@return 成功返回0，失败返回错误码
*/
int update_db(char *tableName, char *field, char *cond) 
{
    char sql[MAX_SQL_LEN] = {0};
    int nRet = -1;
    if(gDbHandle == NULL)
        return -1;
    if(cond == NULL)
        cond = "1=1";
    sprintf(sql, "UPDATE %s SET %s WHERE %s;", tableName, field, cond);
    nRet = sqlite3_exec(gDbHandle, sql , 0 , 0 , &zErrMsg );
    if(nRet != 0) 
    {
        dlog(LOG_ERROR,"Can't insert database [%s]\n",zErrMsg);
        sqlite3_free(zErrMsg);
        //sqlite3_close(gDbHandle);
    }
    return nRet;  
}
int table_exists_callback(void *data, int argc, char **argv, char **azColName) {
    int *table_exists = (int *)data;

    if (argc > 0) {
        *table_exists = 1;
    }

    return 0;
}

/*初始化数据库*/
int db_init(char *dbName,char * tableName)
{
    int nRet = -1;
    char sql[MAX_SQL_LEN] = {0};
    int tableExists = 0;
    nRet = open_db(dbName);
    if (0 != nRet)
    {
        printf("打开数据库失败===================\n");
        return nRet;
    }
    // snprintf(sql, MAX_SQL_LEN, "SELECT name FROM sqlite_master WHERE type='table' AND name='%s';", tableName);
    // nRet = sqlite3_exec(gDbHandle, sql , table_exists_callback , &tableExists , &zErrMsg ); 
    nRet = create_db(tableName);
    if (0 != nRet)
    {
        printf("表存在------------\n");
        return 0; 
    }
    else if (0 == nRet)
    {
        add_column_to_db(DB_FEATURE_INFO,FEATURESID,VARCHAR);
        add_column_to_db(DB_FEATURE_INFO,NAME,VARCHAR);
        add_column_to_db(DB_FEATURE_INFO,POSX1,VARCHAR);
        add_column_to_db(DB_FEATURE_INFO,POSY1,VARCHAR);
        add_column_to_db(DB_FEATURE_INFO,POSX2,VARCHAR);
        add_column_to_db(DB_FEATURE_INFO,POSY2,VARCHAR);
        add_column_to_db(DB_FEATURE_INFO,DATA,VARCHAR);

        nRet = 0;
    }

    return nRet;
}