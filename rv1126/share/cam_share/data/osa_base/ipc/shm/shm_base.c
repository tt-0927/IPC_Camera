


#include "shm_base.h"

unsigned long long shmHead_get_blockBuffSize(shmHead_t *handle)
{
	unsigned int blockBuffSize = 0;
	AO_GET(&(handle->blockBuffSize));
	blockBuffSize = handle->blockBuffSize;
	return blockBuffSize;
}

unsigned int shmHead_get_referenceCounts(shmHead_t *handle)
{
	unsigned int referenceNum = 0;
	AO_GET(&(handle->referenceNum));
	referenceNum = handle->referenceNum;
	return referenceNum;
}

unsigned int shmHead_get_wroteNum(shmHead_t *handle)
{
	unsigned int wroteNum = 0;
	AO_GET(&(handle->wroteNum));
	wroteNum = handle->wroteNum;
	return wroteNum;
}

void shmConf_set_needRemap(shmHead_t *handle)
{
	AO_SET(&(handle->needRemap),1);	//set true
}

int shmConf_get_needRemap(shmHead_t *handle)
{
	unsigned int needRemap = 0;
	AO_GET(&(handle->needRemap));
	needRemap = handle->needRemap;
	return needRemap;
}

void shmHead_increase_WroteNum(shmHead_t *handle)
{
	AO_ADD(&(handle->wroteNum),1);
}

void shmHead_reset_WroteNum(shmHead_t *handle)
{
	AO_SET(&(handle->wroteNum),0);
}


void shmHead_increase_ReferenceCounts(shmHead_t *handle)
{
	AO_ADD(&(handle->referenceNum),1);
}

void shmHead_decrease_ReferenceCounts(shmHead_t *handle)
{
	Uint32 currentReferenceCount = 0;
	AO_GET(&(handle->referenceNum));
	currentReferenceCount = handle->referenceNum;	//current reference num
	do {
		if (currentReferenceCount == 0)
		{
			return;
		}
	} while (!AO_CASB(&(handle->referenceNum),currentReferenceCount, currentReferenceCount - 1));
}
