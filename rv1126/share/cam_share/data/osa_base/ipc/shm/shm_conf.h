

#ifndef _OS_IPC_SHM_CORE_SOURCE_CONF_INCLUDE_
#define _OS_IPC_SHM_CORE_SOURCE_CONF_INCLUDE_




typedef struct _SHM_CONF_INFO_
{
	Uint64 blockBuffSize;	//perser buff size
	Uint32 blockNum;		//block number
	Uint64 managedShmSize;	//shm total size
}shmConf_t;


/*
 * update shm conf
 * @param[in] handle: look shmConf_t
 * @param[in] realBuffSize: this data real size
 * @param[out] return:0-success,-1-failed
 * */
int shmConf_update(shmConf_t* handle,Uint64 realBuffSize);


#endif //_OS_IPC_SHM_CORE_SOURCE_CONF_INCLUDE_


