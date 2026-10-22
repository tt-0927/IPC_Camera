
#ifndef _OS_IPC_SHM_CORE_SOURCE_BASE_INCLUDE_
#define _OS_IPC_SHM_CORE_SOURCE_BASE_INCLUDE_


#include "os.h"
#include "os_atom.h"
#include "os_hashCode.h"

#define SHM_CHANNLE_NAME_LEN_MAX	(512)

typedef unsigned int atomicUint32_t;	//unsigned int 32bit,atom operation
typedef unsigned long long atomicUint64_t;	//unsigned int 64bit,atom operation


typedef struct _SHM_HEAD_INFO_
{
	atomicUint32_t wroteNum;		//current write block index
	atomicUint32_t referenceNum;	//process reference number
	atomicUint32_t needRemap;		//if need remap,0-flase,1-true
	atomicUint32_t blockBuffSize;	//this Message buff is size

}shmHead_t;

unsigned long long shmHead_get_blockBuffSize(shmHead_t *handle);

unsigned int shmHead_get_referenceCounts(shmHead_t *handle);

unsigned int shmHead_get_wroteNum(shmHead_t *handle);

void shmConf_set_needRemap(shmHead_t *handle);

int shmConf_get_needRemap(shmHead_t *handle);

void shmHead_increase_WroteNum(shmHead_t *handle);

void shmHead_reset_WroteNum(shmHead_t *handle);

void shmHead_increase_ReferenceCounts(shmHead_t *handle);

void shmHead_decrease_ReferenceCounts(shmHead_t *handle);



#endif //_OS_IPC_SHM_CORE_SOURCE_BASE_INCLUDE_

