
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include "condition_notifier.h"
#include "os_atom.h"

static int _condition_openOrCreate(conditionNotifier_t* handle);
static int _condition_openOnly(conditionNotifier_t* handle);
static int _condition_remove(conditionNotifier_t* handle);
static int _condition_reset(conditionNotifier_t* handle);


conditionNotifier_t* condition_notifier_init(key_t key)
{
	conditionNotifier_t* handle = (conditionNotifier_t*)malloc(sizeof(conditionNotifier_t));
	if(handle == NULL)
	{
		printf("malloc error!!\n");
		return NULL;
	}

	memset(handle,0,sizeof(conditionNotifier_t));

	handle->key = key;
	printf("condition notifier key: %d\n", handle->key);
	handle->shmSize = sizeof(Indicator);

	if (_condition_openOrCreate(handle) < 0)
	{
		printf("fail to init condition notifier.\n");
		AO_SET(&(handle->isShutdown),1);
		return NULL;
	}

	AO_GET(&(handle->indicator->nextSeq));
	handle->nextSeq = handle->indicator->nextSeq;
	printf("next_seq: %llu\n",handle->nextSeq);

	return handle;
}

int condition_notifier_unInit(conditionNotifier_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}
	condition_notifier_shutdown(handle);
	free(handle);
	handle = NULL;
	return 0;
}


int condition_notifier_shutdown(conditionNotifier_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}
	if (AO_SWAP(&(handle->isShutdown),1))
	{
		return 0;
	}

	usleep(100*1000);//sleep 100ms
	_condition_reset(handle);
	return 0;
}

int condition_notifier_notify(conditionNotifier_t* handle,const readAbleInfo_t info)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	Uint32 isShutDown = 0;
	AO_GET(&(handle->isShutdown));
	isShutDown = handle->isShutdown;
	if (isShutDown)
	{
		printf("notifier is shutdown.\n");
		return -1;
	}

	Uint64 seq = AO_F_ADD(&(handle->indicator->nextSeq),1);
	Uint64 idx = seq % kBufLength;
	memcpy(&(handle->indicator->infos[idx]),&info,sizeof(readAbleInfo_t));
	handle->indicator->seqs[idx] = seq;

	return 0;
}


int condition_notifier_listen(conditionNotifier_t* handle,int timeout_ms, readAbleInfo_t* info)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	Uint32 isShutDown = 0;
	AO_GET(&(handle->isShutdown));
	isShutDown = handle->isShutdown;
	if (isShutDown)
	{
		printf("notifier is shutdown.\n");
		return -1;
	}

	//int timeout_us = timeout_ms * 1000; //cpu hight
	int timeoutMs = timeout_ms;
	while (1)
	{
		AO_GET(&(handle->isShutdown));
		isShutDown = handle->isShutdown;
		if(isShutDown)
		{
			break;
		}

		AO_GET(&(handle->indicator->nextSeq));
		Uint64 seq = handle->indicator->nextSeq;
		if (seq != handle->nextSeq)
		{
			auto idx = handle->nextSeq % kBufLength;
			if (handle->indicator->seqs[idx] == handle->nextSeq)
			{
				*info = handle->indicator->infos[idx];
				++handle->nextSeq;
				return 0;
			} else
			{
				printf("seq[%llu] is writing, can not read now.\n",handle->nextSeq);
			}
		}

		if (timeoutMs > 0)
		{
			usleep(10*1000);	//sleep 1ms //sleep 50us
			timeoutMs -= 10;
		} else
		{
			//获取数据超时退出
			return -1;
		}
	}

	return -1;
}


static int _condition_openOrCreate(conditionNotifier_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	// create managed_shm_
	int retry = 0;
	int shmid = 0;
	while (retry < 2)
	{
		shmid = shmget(handle->key, handle->shmSize, 0644 | IPC_CREAT | IPC_EXCL);
		if (shmid != -1)
		{
			break;
		}

		if (EINVAL == errno)
		{
			printf("need larger space, recreate.\n");
			_condition_reset(handle);
			_condition_remove(handle);
			++retry;
		} else if (EEXIST == errno)
		{
			printf("shm already exist, open only.\n");
			return _condition_openOnly(handle);
		} else
		{
			break;
		}
	}

	if (shmid == -1)
	{
		printf("create shm failed, error code:%s\n", strerror(errno));
		return -1;
	}

	// attach managed_shm_
	handle->managedShm = shmat(shmid, NULL, 0);
	if ((handle->managedShm) == (void*)(-1))
	{
		printf("attach shm failed.\n");
		shmctl(shmid, IPC_RMID, 0);
		return -1;
	}

	// create indicator_
	handle->indicator = (Indicator*)handle->managedShm;
	if (handle->indicator == NULL)
	{
		printf("create indicator failed.\n");
		shmdt(handle->managedShm);
		handle->managedShm = NULL;
		shmctl(shmid, IPC_RMID, 0);
		return -1;
	}

	printf("open or create true.\n");
	return 0;
}

static int _condition_openOnly(conditionNotifier_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	// get managed_shm_
	int shmid = shmget(handle->key, 0, 0644);
	if (shmid == -1)
	{
		printf("get shm failed.\n");
		return -1;
	}

	// attach managed_shm_
	handle->managedShm = shmat(shmid, NULL, 0);
	if (handle->managedShm == (void*)(-1))
	{
		printf("attach shm failed.\n");
		return -1;
	}

	// get indicator_
	handle->indicator = (Indicator*)(handle->managedShm);
	if (handle->indicator == NULL)
	{
		printf("get indicator failed.\n");
		shmdt(handle->managedShm);
		handle->managedShm = NULL;
		return -1;
	}

	printf("open true.\n");
	return 0;
}


static int _condition_remove(conditionNotifier_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	int shmid = shmget(handle->key, 0, 0644);
	if (shmid == -1 || shmctl(shmid, IPC_RMID, 0) == -1)
	{
		printf("remove shm failed, error code: %s\n",strerror(errno));
		return -1;
	}
	printf("remove success.\n");

	return 0;
}

static int _condition_reset(conditionNotifier_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	handle->indicator = NULL;
	if (handle->managedShm != NULL)
	{
		shmdt(handle->managedShm);
		handle->managedShm = NULL;
	}
	return 0;
}


















