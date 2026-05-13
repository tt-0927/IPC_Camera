/*
 * @Descripttion: sqlite数据库操作基础库
 * @version: v1.0
 * @Author: fanghongshen
 * @Date: 2021-04-13 14:41:44
 * @LastEditors: fanghongshen
 * @LastEditTime: 2022-03-07 17:03:28
 */

#include "db_sqlite_control.h"
#include "dlog.h"
#include <unistd.h>

static int callback(void *pNotUsed, int argc, char **argv, char **pColName)
{
	return 0;
}

sqlite3 *db_sql_init(char *achFileName)
{
	int nRet = 0;
	sqlite3 *pSqlHandle = NULL;
	nRet = sqlite3_open(achFileName, &pSqlHandle);
	if (nRet < 0)
	{
		dlog_error("DB sqlite3_open failed::%s\n", sqlite3_errmsg(pSqlHandle));
		return NULL;
	}

	return pSqlHandle;
}

int db_sql_uninit(sqlite3 *pSqlHandle)
{
	sqlite3_close(pSqlHandle);
	return 0;
}

int db_backup_data(sqlite3 *pSqlHandle, int *pIsSave, char *pSqlFileBackup)
{
	int nRc;				 /* Function return code */
	sqlite3 *pFile;			 /* Database connection opened on db_backup_file */
	sqlite3_backup *pBackup; /* Backup object used to copy data */
	sqlite3 *pTo;			 /* Database to copy to (pFile or pInMemory) */
	sqlite3 *pFrom;			 /* Database to copy from (pFile or pInMemory) */
	int nHaveBackup = 0;

	if (0 == access(pSqlFileBackup, 0))
	{
		nHaveBackup = 1;
	}
	else
	{
		dlog_info("%s is not Exist.\n", pSqlFileBackup);
	}

	if (!nHaveBackup && 0 == *pIsSave)
	{
		dlog_info("No %s and main DB,Need create!!", pSqlFileBackup);
		return 0;
	}

	nRc = sqlite3_open(pSqlFileBackup, &pFile);
	if (nRc == SQLITE_OK)
	{
		pFrom = (*pIsSave ? pSqlHandle : pFile);
		pTo = (*pIsSave ? pFile : pSqlHandle);
		/*备份process*/
		pBackup = sqlite3_backup_init(pTo, "main", pFrom, "main"); //创建备份对象
		if (pBackup)
		{
			(void)sqlite3_backup_step(pBackup, -1); //拷贝数据
			(void)sqlite3_backup_finish(pBackup);	//释放资源
		}

		nRc = sqlite3_errcode(pTo); //若拷贝的过程中出现任何错误,该函数可以获取具体的错误码
									//*isSave = nHaveBackup;
	}

	dlog_info("backup sqlite3_open success nRc=%d", nRc);
	(void)sqlite3_close(pFile);
	return nRc;
}

int db_create_table(sqlite3 *pSqlHandle, char *achTableName, char *achSqlData)
{
	if (pSqlHandle == NULL || achTableName == NULL || achSqlData == NULL)
	{
		dlog_error("DB db_create_table  data is NULL\n");
		return -1;
	}
	// char achSqlcmd[256] = {0};
	// //sprintf(achSqlcmd,"PRAGMA table_info(%s)",achTableName);

	// char **ppResult = NULL;
	// char *pErrMsg = NULL;
	// int nMaxrow = 0;
	// int nMaxcolumn = 0;
	int nRet = 0;

	// nRet = sqlite3_get_table(pSqlHandle , achSqlcmd , &ppResult , &nMaxrow , &nMaxcolumn , &pErrMsg);
	// dlog_info"db_create_table[%s] nRet[%d]\n",achTableName,nRet);
	// if (nRet != SQLITE_OK)
	// {
	// 	if(pErrMsg)
	// 	{
	// 		dlog_info"DB sql table[%s] is no! error[%s]\n",achTableName,pErrMsg);
	// 		sqlite3_free(pErrMsg);
	// 	}

	// 	/*
	// 	创建新的表格
	// 	*/
	//     nRet = db_control_sql(pSqlHandle,achSqlData);

	// }
	nRet = db_control_sql(pSqlHandle, achSqlData);

	return nRet;
}

int db_control_sql(sqlite3 *pSqlHandle, char *achSqlData)
{
	if (pSqlHandle == NULL || achSqlData == NULL)
	{
		dlog_error("DB db_control_sql  is NULL\n");
		return -1;
	}
	char *pErrMsg = NULL;
	int nRet = 0;

	nRet = sqlite3_exec(pSqlHandle, "begin transaction", 0, 0, &pErrMsg);
	nRet = sqlite3_exec(pSqlHandle, achSqlData, callback, 0, &pErrMsg);
	if (nRet != 0)
	{
		//异常，回滚，撤销失败的动作
		dlog_error("Error,sqlite3_exec failed!! %s nRet=%d error[%s]\n", achSqlData, nRet, pErrMsg);
		sqlite3_exec(pSqlHandle, "rollback transaction", 0, 0, &pErrMsg);
		return -1;
	}
	dlog_debug("DB perform sql[%s] is succful!\n", achSqlData);
	//提交事务
	nRet = sqlite3_exec(pSqlHandle, "commit transaction", 0, 0, &pErrMsg);

	return 0;
}

int db_get_last_insert_id(sqlite3 *pSqlHandle)
{
	sqlite3_int64 lastInsertId = sqlite3_last_insert_rowid(pSqlHandle);
    return (int)lastInsertId;
}

int db_get_sqlData(sqlite3 *pSqlHandle, char *achSqlData, char ***ppOutData, int *nTotal, int *nLine)
{
	if (pSqlHandle == NULL || achSqlData == NULL)
	{
		dlog_error("db_get_sqlData is NULL\n");
		return -1;
	}

	int nRet = 0;
	int nRow = 0;
	int nColumn = 0;
	char *pErrMsg = NULL;
	nRet = sqlite3_get_table(pSqlHandle, achSqlData, ppOutData, &nRow, &nColumn, &pErrMsg);

	if (nRet != SQLITE_OK)
	{
		dlog_debug("sqlite3_get_table nRet[%d]\n", nRet);
		if (pErrMsg)
		{
			dlog_error("DB sql=%s  <---> Error=%s\n", achSqlData, pErrMsg);
			sqlite3_free(pErrMsg);
		}
	}
	*nTotal = nRow;
	*nLine = nColumn;

	// dlog_info"DB sql=%s  <---> total=%d\n",achSqlData, nRow);

	return nRet;
}

int db_puff_get_int(int *pNum, char *pData)
{
	if (pData != NULL)
	{
		*pNum = atoi(pData);
	}
	return 0;
}

int db_puff_get_char(char *pNum, int nLen, char *pData)
{
	if (pData != NULL)
	{
		snprintf(pNum, nLen, "%s", pData);
	}
	return 0;
}
