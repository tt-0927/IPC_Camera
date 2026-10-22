
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<pthread.h>
#include<unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "dlog.h"
#include "file_base.h"

int g_SleepTime = 20000;//为了限速
typedef struct _FileListHandle
{
	char *pBuf;//读出的内容
	int nSize;//每次读出的大小
	FileRecvStatus IsFileEnd;//是否到达文件尾
	int nNum;//文件分为几次读出
}FileListHandle;

typedef struct _FileOprateHandle
{
	List_Handle_t pListHandle;
	pthread_mutex_t lock;
	FTP_FILE_STATUS FileStatus;
	int IsAutoStop;//是自动退出，或者异常，1代表自动
}FileOprateHandle;
int file_free_buf(char **pBuf)
{
	if(pBuf == NULL || *pBuf == NULL)
	{
		dlog(LOG_ERROR,"file_free_buf fail!\n");
		return -1;
	}
	free(*pBuf);
	*pBuf = NULL;
	return 0;

}

int file_write_data(const void *pBuf, int size, const char *fileName, const char *mode)
{
	FILE* filefd;

	if(fileName == NULL || pBuf == NULL)
	{
		return -1;
	}
	if ( NULL == (filefd = fopen(fileName, mode)))
	{
		dlog(LOG_ERROR,"fwrite");
		return -1;
	}
	fwrite(pBuf, 1, size, filefd);
	fclose(filefd);
	return 0;
}
int file_auto_fread(char **pBuf, char *fileName, int set, const char *mode)
{
	FILE* filefd;
	unsigned long long nLen = 0;
	unsigned long long nRetLen = -1;
	int nFreadSize = 0;
	if(fileName == NULL || pBuf == NULL)
	{
		return -1;
	}
	if ( NULL == (filefd = fopen(fileName, mode)))
	{
		dlog(LOG_ERROR,"fopen");
		return -1;
	}
	if(-1 == fseek(filefd, 0 ,SEEK_END)) /* 定位到文件末尾 */
	{
		dlog(LOG_ERROR,"fseek END");
		fclose(filefd);
		return -1;
	}
	nLen = ftell(filefd); /* 得到文件大小 */
	nRetLen = nLen - set;
	if (-1 == fseek(filefd, set, SEEK_SET))
	{
		dlog(LOG_ERROR,"fseek SET");
		fclose(filefd);
		return (-1);
	}
	*pBuf =(char*)malloc(nRetLen + 1);
	if(*pBuf == NULL)
	{
		fclose(filefd);
		return -1;
	}
	(*pBuf)[nRetLen] = '\0';
	nFreadSize = fread(*pBuf, 1, nRetLen, filefd);
	if(nRetLen != nFreadSize)
	{
		if(feof(filefd))
		{
			dlog(LOG_DEBUG,"the fileis end\n");
		}
		else
		{
			dlog(LOG_ERROR,"file_auto_fread");
			free(pBuf);
			nRetLen = -1;
		}
	}
	fclose(filefd);
	return nRetLen;
}
int file_reply_fread(char *pBuf, char *fileName, int set, int nSize, const char *mode)
{

	FILE* filefd;
	int nRetLen = -1;
	if(fileName == NULL || pBuf == NULL)
	{
		return -1;
	}
	if ( NULL == (filefd = fopen(fileName, mode)))
	{
		dlog(LOG_ERROR,"fopen");
		return -1;
	}
	if (-1 == fseek(filefd, set, SEEK_SET))
	{
		dlog(LOG_ERROR,"fseek SET");
		fclose(filefd);
		return (-1);
	}
	nRetLen = fread(pBuf, 1, nSize, filefd);
	if(nRetLen != nSize)
	{
		if(feof(filefd))
		{
			printf("the fileis end\n");
		}
		else
		{
			dlog(LOG_ERROR,"file_reply_fread");
			nRetLen = -1;
		}
	}
	fclose(filefd);
	return nRetLen;
}
unsigned long long file_get_len(char *fileName)
{
	FILE* filefd;
	unsigned long long  nLen = -1;
	struct stat FileInfo;
	if(fileName == NULL)
	{
		return -1;
	}
	if ( NULL == (filefd = fopen(fileName, "r")))
	{
		dlog(LOG_ERROR,"open");
		return -1;
	}
	if(stat(fileName, &FileInfo) < 0)
	{
		dlog(LOG_ERROR,"stat");
	}
	nLen = FileInfo.st_size;
	fclose(filefd);
	return nLen;

}
static int CleanList(List_Handle_t pListHandle)
{
	int nListSize = list_size(pListHandle);
	FileListHandle *pListData = NULL;
	int nclean = 0;
	if(nListSize > 0)
	{
		for(;nclean < nListSize; nclean++)
		{
			pListData = (FileListHandle*)list_pop_front(pListHandle);
			if(pListData == NULL || pListData->pBuf == NULL)
			{
				dlog(LOG_ERROR,"the clean list is fail,please check funtion\n");
				return -1;
			}
			else
			{
				free(pListData->pBuf);
				pListData->pBuf = NULL;
				free(pListData);
				pListData = NULL;
			}
		}
	}
	return 0;

}
int file_clean_list(File_Handle_t pFileHandle)
{
	List_Handle_t pListHandle;
	pthread_mutex_t *lock;
	FileOprateHandle* pFileOprateHandle = (FileOprateHandle*)pFileHandle;
	if(pFileOprateHandle == NULL || pFileOprateHandle->pListHandle == NULL)
	{
		dlog(LOG_ERROR,"file_clean_list is fail\n");
		return -1;
	}

	pListHandle = pFileOprateHandle->pListHandle;
	lock = &(pFileOprateHandle->lock);
	pthread_mutex_lock(lock);
	CleanList(pListHandle);
	pthread_mutex_unlock(lock);
	return 0;
}
RetErr_t file_push_list(char *fileName, unsigned long long set, int nSize, File_Handle_t pFileHandle)
{
	FILE* filefd = NULL;
	long long  nFileLen = 0;
	unsigned long long nNum = 0;//代表要读取多少次
	int nEndSize = 0;//最后一次读多少个字节
	RetErr_t nRet = RET_SUCCESS;
	int nFreadRet = 0;
	int nCircul = 0;
	int bErrFlag = 0;
	struct stat FileInfo;
	fpos_t post;
	FileListHandle* pFileListHandle = NULL;
	List_Handle_t pListHandle;
	pthread_mutex_t *lock;
	FileOprateHandle* pFileOprateHandle = (FileOprateHandle*)pFileHandle;

	if(fileName == NULL || set < 0 || nSize <= 0 || pFileOprateHandle == NULL ||
			pFileOprateHandle->pListHandle == NULL)
	{
		nRet =  RET_PARAMER_ERR;
		goto CLEAN;
	}
	printf("\033[33m""download fileName:%s set: %lld nSize:%d\n""\033[0m", fileName, set, nSize);
	pFileOprateHandle->IsAutoStop = 0;
	pListHandle = pFileOprateHandle->pListHandle;
	lock = &(pFileOprateHandle->lock);
	pFileOprateHandle->IsAutoStop = 0;

	if ( NULL == (filefd = fopen(fileName, "rb")))
	{
		dlog(LOG_ERROR,"file_push_list fopen");
		nRet=  RET_OPENFILE_FAILED;
		goto CLEAN;
	}
	if(stat(fileName, &FileInfo) < 0)
	{
		dlog(LOG_ERROR,"file_push_list stat");
		nRet = RET_UNKNOW_FAIL;
		goto CLEAN;
	}
	// post.__pos = set;
	if (-1 == fsetpos(filefd, &post))
	{
		dlog(LOG_ERROR,"file_push_list fseek SET");
		nRet = RET_UNKNOW_FAIL;
		goto CLEAN;
	}
	nFileLen = FileInfo.st_size - set;
	if(nFileLen < 0)
	{
		dlog(LOG_ERROR,"file_push_list set is error");
		nRet = RET_PARAMER_ERR;
		goto CLEAN;
	}
	if(nFileLen == 0)
	{
		dlog(LOG_ERROR,"file_push_list set is end");
		nRet = RET_FILE_END;
		goto CLEAN;
	}
	nNum = nFileLen / nSize;
	nEndSize =  nFileLen % nSize;
	if(nEndSize != 0)
	{
		nNum ++;
	}
	file_set_status(pFileHandle, FTP_START_DEL);
	for(nCircul = 0; nCircul < nNum; nCircul++)
	{
		if(file_get_status(pFileHandle) != FTP_START_DEL)
		{
			bErrFlag = 1;//外界异常了
			nRet = RET_UNKNOW_FAIL;
			break;
		}
		if(file_list_size(pFileHandle) >= LISTCASH)
		{
			nCircul--;
			usleep(20000);
			continue;
		}
		pFileListHandle = malloc(sizeof(FileListHandle));
		if(pFileListHandle == NULL)
		{
			nRet = RET_MEMORY_FAIL;
			goto CLEAN;
		}
		memset(pFileListHandle, 0 ,sizeof(FileListHandle));
		pFileListHandle->pBuf = malloc(nSize);
		if(pFileListHandle->pBuf == NULL)
		{
			nRet = RET_MEMORY_FAIL;
			goto CLEAN;
		}
		memset(pFileListHandle->pBuf, 0, nSize);


		nFreadRet = fread(pFileListHandle->pBuf, 1, nSize, filefd);

		pFileListHandle->nSize = nFreadRet;
		if(nFreadRet != nSize)
		{
			if(feof(filefd))
			{
				pFileListHandle->IsFileEnd = DATA_EOF;
				pFileListHandle->nSize = nEndSize;

				pthread_mutex_lock(lock);
				list_push_back(pListHandle, pFileListHandle);
				pthread_mutex_unlock(lock);

				printf("the file is end\n");
				break;
			}
			else
			{
				dlog(LOG_ERROR,"fread");
				nRet = RET_FREAD_FAIL;
				break;
			}
		}
		else if(nCircul == nNum - 1)
		{
			pFileListHandle->IsFileEnd = DATA_EOF;
			pthread_mutex_lock(lock);
			list_push_back(pListHandle, pFileListHandle);
			pthread_mutex_unlock(lock);
			break;
		}
		pthread_mutex_lock(lock);
		list_push_back(pListHandle, pFileListHandle);
		pthread_mutex_unlock(lock);


		usleep(g_SleepTime);
	}
CLEAN:
	dlog(LOG_DEBUG,"finish is file_push_list %d\n", bErrFlag);

	if(filefd)
	{
		fclose(filefd);
		filefd = NULL;
	}
	//通知可以反初始化了

	if(bErrFlag == 1 || nRet != RET_SUCCESS)
	{
		file_clean_list(pFileOprateHandle);
		file_set_status(pFileOprateHandle, FTP_Unit);
	}
	if(pFileOprateHandle)
	{
		pFileOprateHandle->IsAutoStop = 1;
	}


	return nRet;

}
RetErr_t file_pop_list(char * pBuf, int* nSize, File_Handle_t pFileHandle)
{
	FileListHandle* pListdata = NULL;
	RetErr_t nRet = RET_SUCCESS;

	List_Handle_t pListHandle;
	pthread_mutex_t *lock;
	FileOprateHandle* pFileOprateHandle = (FileOprateHandle*)pFileHandle;
	if(pFileOprateHandle == NULL || pFileOprateHandle->pListHandle == NULL || pBuf == NULL || nSize == NULL)
	{
		return RET_PARAMER_ERR;
	}

	pListHandle = pFileOprateHandle->pListHandle;
	lock = &(pFileOprateHandle->lock);

	pthread_mutex_lock(lock);
	pListdata = (FileListHandle*)list_pop_front(pListHandle);
	pthread_mutex_unlock(lock);
	if(pListdata == NULL || pListdata->pBuf == NULL)
	{
		return RET_CREATE_FAILED;
	}
	if(pListdata->IsFileEnd == DATA_EOF)
	{
		nRet = RET_FILE_END;
	}
	*nSize = pListdata->nSize;
	memcpy(pBuf, pListdata->pBuf, pListdata->nSize);
	free(pListdata->pBuf);
	free(pListdata);
	return nRet;

}
RetErr_t file_dataPushTo_list(char * pBuf, int nSize,FileRecvStatus eof, File_Handle_t pFileHandle)
{
	FileListHandle* pFileListHandle = NULL;
	RetErr_t nRet = RET_SUCCESS;

	List_Handle_t pListHandle;
	pthread_mutex_t *lock;
	FileOprateHandle* pFileOprateHandle = (FileOprateHandle*)pFileHandle;
	if(pFileOprateHandle == NULL || pFileOprateHandle->pListHandle == NULL || pBuf == NULL || nSize  <= 0)
	{
		nRet = RET_PARAMER_ERR;
		goto CLEAN;
	}

	pListHandle = pFileOprateHandle->pListHandle;
	lock = &(pFileOprateHandle->lock);

	pFileListHandle = malloc(sizeof(FileListHandle));
	if(pFileListHandle == NULL)
	{
		nRet = RET_MEMORY_FAIL;
		goto CLEAN;
	}
	memset(pFileListHandle, 0 ,sizeof(FileListHandle));
	pFileListHandle->pBuf = malloc(nSize);
	if(pFileListHandle->pBuf == NULL)
	{
		nRet = RET_MEMORY_FAIL;
		goto CLEAN;
	}
	memcpy(pFileListHandle->pBuf, pBuf, nSize);
	pFileListHandle->nSize = nSize;
	pFileListHandle->IsFileEnd = eof;

	pthread_mutex_lock(lock);
	list_push_back(pListHandle, pFileListHandle);
	pthread_mutex_unlock(lock);
CLEAN:
	if(pFileListHandle !=NULL && pFileListHandle->pBuf == NULL)
	{
		free(pFileListHandle);
	}
	return nRet;
}
RetErr_t file_listPopTo_file(char *fileName, unsigned long long set, File_Handle_t pFileHandle)
{

	FileListHandle* pListdata = NULL;
	RetErr_t nRet = RET_SUCCESS;
	int nFwriteRet = 0;
	int FileSize = 0;
	int bFlagDel = 0;
	FILE* filefd = NULL;
	int nSize=0;
	int nCount = 0;//共接收了多少次

	List_Handle_t pListHandle;
	pthread_mutex_t *lock;
	FileOprateHandle* pFileOprateHandle = (FileOprateHandle*)pFileHandle;
	printf("\033[33m""upload fileName:%s set: %lld \n""\033[0m", fileName, set);
	if(pFileOprateHandle == NULL || pFileOprateHandle->pListHandle == NULL ||fileName == NULL || set < 0)
	{
		nRet = RET_PARAMER_ERR;
		goto CLEAN;
	}

	pFileOprateHandle->IsAutoStop = 0;
	pListHandle = pFileOprateHandle->pListHandle;
	lock = &(pFileOprateHandle->lock);

	if ( NULL == (filefd = fopen(fileName, "wb")))
	{
		perror("fopen file_listPopTo_file");
		nRet=  RET_OPENFILE_FAILED;
		goto CLEAN;
	}
	while(1)
	{
		//attention============================
		if(file_get_status(pFileOprateHandle) != FTP_START_DEL)
		{
			printf("file_listPopTo_file is finish\n");
			bFlagDel = 1;
			nRet = RET_UNKNOW_FAIL;
			break;
		}
		FileSize = file_list_size(pFileOprateHandle);
		if(FileSize <= 0)
		{
			usleep(10000);
			continue;
		}
		pthread_mutex_lock(lock);
		pListdata = (FileListHandle*)list_pop_front(pListHandle);
		pthread_mutex_unlock(lock);
		nCount++;
		if(pListdata == NULL || pListdata->pBuf == NULL)
		{
			nRet = RET_CREATE_FAILED;
			break;
		}
		if(pListdata->IsFileEnd == DATA_FAIL)
		{
			nRet = RET_RECV_ERR;
			break;
		}
		nSize = pListdata->nSize;
		nFwriteRet = fwrite(pListdata->pBuf, 1, nSize, filefd);
		if(nFwriteRet !=  nSize)
		{
			dlog(LOG_ERROR,"file_pop_list fwrite");
			nRet = RET_FWRITE_FAIL;
			break;
		}
		if(pListdata->IsFileEnd == DATA_EOF)
		{
			dlog(LOG_DEBUG,"recv is finish nCount:%d\n", nCount);
			break;
		}
		free(pListdata->pBuf);
		free(pListdata);
		pListdata = NULL;
		usleep(g_SleepTime);

	}
CLEAN:
	if(pListdata)//异常处理
	{
		dlog(LOG_ERROR,"file_listPopTo_file is finish nRet:%d\n", nRet);
		if(pListdata->pBuf)
		{
			free(pListdata->pBuf);
		}
		free(pListdata);
		pListdata = NULL;
	}
	if(filefd)
	{
		fclose(filefd);
	}
	//通知可以反初始化了
	if(bFlagDel == 1)
		file_set_status(pFileOprateHandle, FTP_Unit);
	pFileOprateHandle->IsAutoStop = 1;

	return nRet;
}
int file_list_size(File_Handle_t pFileHandle)
{
	int nSize = 0;

	FileOprateHandle* pFileOprateHandle = (FileOprateHandle*)pFileHandle;
	pthread_mutex_t *lock;
	if(pFileOprateHandle == NULL || pFileOprateHandle->pListHandle == NULL)
	{
		dlog(LOG_ERROR,"file_unit is Fail\n");
		return -1;
	}

	lock = &(pFileOprateHandle->lock);
	pthread_mutex_lock(lock);
	nSize = list_size(pFileOprateHandle->pListHandle);
	pthread_mutex_unlock(lock);
	return nSize;
}

File_Handle_t file_init()
{
	FileOprateHandle* pFileHandle =(FileOprateHandle*)malloc(sizeof(FileOprateHandle));
	if(pFileHandle == NULL)
	{
		dlog(LOG_ERROR,"file_init is fail\n");
		return NULL;
	}
	pFileHandle->pListHandle = list_create();
	if(pFileHandle->pListHandle == NULL)
	{
		dlog(LOG_ERROR,"pFileHandle->pListHandle = list_create() is fail\n");
		return NULL;
	}
	pthread_mutex_init(&(pFileHandle->lock), NULL);
	pFileHandle->FileStatus = FTP_START_DEL;
	pFileHandle->IsAutoStop = 1;
	return pFileHandle;
}
int file_unit(File_Handle_t pFileHandle)
{
	FileOprateHandle* pFileOprateHandle = (FileOprateHandle*)pFileHandle;
	int nOutTime = 0;
	if(pFileOprateHandle == NULL || pFileOprateHandle->pListHandle == NULL)
	{
		dlog(LOG_ERROR,"file_unit is Fail\n");
		return -1;
	}
	while(file_get_status(pFileHandle) != FTP_Unit)
	{
		if(nOutTime++ > 2 || pFileOprateHandle->IsAutoStop == 1)
		{
			break;
		}
		sleep(1);
		continue;
	}
	file_clean_list(pFileOprateHandle);
	list_destory(pFileOprateHandle->pListHandle);
	pthread_mutex_destroy(&(pFileOprateHandle->lock));
	free(pFileOprateHandle);
	printf("file_unit is Sucess\n");
	return 0;
}
int file_set_status(File_Handle_t pFileHandle, FTP_FILE_STATUS FileStatus)
{
	FileOprateHandle* pFileOprateHandle = (FileOprateHandle*)pFileHandle;
	if(pFileOprateHandle == NULL || pFileOprateHandle->pListHandle == NULL)
	{
		dlog(LOG_ERROR,"file_set_status is Fail\n");
		return -1;
	}
	pthread_mutex_lock(&(pFileOprateHandle->lock));
	pFileOprateHandle->FileStatus = FileStatus;
	pthread_mutex_unlock(&(pFileOprateHandle->lock));
	return 0;
}
FTP_FILE_STATUS file_get_status(File_Handle_t pFileHandle)
{
	FileOprateHandle* pFileOprateHandle = (FileOprateHandle*)pFileHandle;
	FTP_FILE_STATUS TempStatus;
	if(pFileOprateHandle == NULL || pFileOprateHandle->pListHandle == NULL)
	{
		dlog(LOG_ERROR,"file_set_status is Fail\n");
		return -1;
	}
	pthread_mutex_lock(&(pFileOprateHandle->lock));
	TempStatus = pFileOprateHandle->FileStatus;
	pthread_mutex_unlock(&(pFileOprateHandle->lock));
	return TempStatus;
}
