/*
 * @Descripttion: sqlite数据库操作基础库。
 * @version: v1.0
 * @Author: fanghongshen
 * @Date: 2021-04-13 09:55:08
 * @LastEditors: fanghongshen
 * @LastEditTime: 2022-03-08 14:02:17
 */
#ifndef _DB_SQLITE_CONTROL_H_
#define _DB_SQLITE_CONTROL_H_
#include "sqlite3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @Function: db_sql_init
 * @Description: 数据库初始化
 * @Input:
 *      achFileName:文件名
 *      pSqlHandle:数据库操作指针
 * @Output: None
 * @Return:
 *      数据类型[int]:成功 0, 失败sqlite错误码
 * @Others:
 */
sqlite3 *db_sql_init(char *achFileName);

/**
 * @Function: db_sql_init
 * @Description: 数据库反初始化
 * @Input:
 *      pSqlHandle:数据库操作指针
 * @Output: None
 * @Return:
 *      数据类型[int]:成功 0, 失败sqlite错误码
 * @Others:
 */
int db_sql_uninit(sqlite3 *pSqlHandle);

/**
 * @Function: db_backup_data
 * @Description: 获取备份数据库
 * @Input:
 *      pSqlHandle:数据库操作指针
 *      pIsSave：文件是否已创建
 *      pSqlFileBackup：备份文件
 * @Output: None
 * @Return: void
 * @Others: None
 */
int db_backup_data(sqlite3 *pSqlHandle, int *pIsSave, char *pSqlFileBackup);

/**
 * @Function: db_create_table
 * @Description: 创建数据库表
 * @Input:
 *      pSqlHandle:数据库操作指针
 *      achTableName:表名
 *      achSqlData:完整sql语句
 * @Output: None
 * @Return:
 *      数据类型[int]:成功 0, 失败sqlite错误码
 * @Others: 已存在表则不创建
 */
int db_create_table(sqlite3 *pSqlHandle, char *achTableName, char *achSqlData);

/**
 * @Function: db_control_sql
 * @Description: 操作数据库
 * @Input:
 *      pSqlHandle:数据库操作指针
 *      achSqlData:完整sql语句
 * @Output: None
 * @Return:
 *      数据类型[int]:成功 0, 失败sqlite错误码
 * @Others:
 */
int db_control_sql(sqlite3 *pSqlHandle, char *achSqlData);
int db_get_last_insert_id(sqlite3 *pSqlHandle);
/**
 * @Function: db_get_sqlData
 * @Description: 获取数据库数据
 * @Input:
 *      pSqlHandle:数据库操作指针
 *      achSqlData: 完整sql语句
 * @Output:
 *      ppOutData:返回sql数据指针
 * @Return:
 *      数据类型[int]:成功 0, 失败sqlite错误码
 * @Others: None
 */
int db_get_sqlData(sqlite3 *pSqlHandle, char *achSqlData, char ***ppOutData, int *nTotal, int *nLine);

int db_puff_get_int(int *pNum, char *pData);
int db_puff_get_char(char *pNum, int nLen, char *pData);

typedef struct _FINDDATA_SQL_
{
    char achFindkey[64];  //查找字段值
    char achFindData[64]; //查找数据
    int nFindNum;         //查找数字
    int nCurPage;         //页数
    int nPageSize;        //当前页数量

} FinddataCond_S;

#endif
