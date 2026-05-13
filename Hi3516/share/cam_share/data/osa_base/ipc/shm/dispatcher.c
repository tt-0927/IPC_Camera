
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "dispatcher.h"
#include "os_hashCode.h"


static int _dispatcher_create(dispatcher_t* handle);
static int _dispatcher_read_message(dispatcher_t* handle,segment_t* segments_,Uint64 channel_id, Uint32 block_index);

dispatcher_t* dispatcher_init(char* notifierName,dispatcher_deal dealData,void *user)
{
	dispatcher_t* handle = (dispatcher_t*)malloc(sizeof(dispatcher_t));
	if(handle == NULL)
	{
		printf("malloc error!!\n");
		return NULL;
	}
	memset(handle,0,sizeof(dispatcher_t));

	handle->dealData = dealData;
	handle->user = user;

	handle->notiferId = (Uint64)BKDRHash(notifierName,strlen(notifierName));
	printf("name[%s] notiferId:%llu\n",notifierName,handle->notiferId);

	OS_mutexCreate(&(handle->segmentMapMutex));

	//create rb tree
	handle->segmentMap = rbtree_init();
	if(handle->segmentMap == NULL)
	{
		printf("rbtree_init error!!!\n");
		if(handle)
		{
			free(handle);
			handle = NULL;
		}
		return NULL;
	}

	//init
	_dispatcher_create(handle);

	return handle;
}

int dispatcher_unInit(dispatcher_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	handle->exit_ = 1;

	//等待线程退出
	OS_thrJoin(&(handle->thread_));

	//释放资源
	if(handle->segmentMap)
	{
		//遍历红黑树，释放每个节点的资源
		segmentContainer_t* segment = NULL;
		rbtreeNode_t* node = NULL;
		for(node = rbtree_firstNode(handle->segmentMap); \
					node != rbtree_endNode(handle->segmentMap);\
					node = rbtree_nextNode(handle->segmentMap,node))
		{
			segment = (segmentContainer_t*)node->value;
			if(segment)
			{
				if(segment->segments)
				{
					segment_unInit(segment->segments);
					segment->segments = NULL;
				}
			}
		}

		rbtree_unInit(handle->segmentMap);
		handle->segmentMap = NULL;
	}

	if(handle->notifier)
	{
		condition_notifier_unInit(handle->notifier);
		handle->notifier = NULL;
	}

	if(handle->subpacket.buff)
	{
		free(handle->subpacket.buff);
		handle->subpacket.buff = NULL;
	}

	OS_mutexDelete(&(handle->segmentMapMutex));

	free(handle);
	handle = NULL;

	return 0;
}

int _dispatcher_read_message(dispatcher_t* handle,segment_t* segments_,Uint64 channel_id, Uint32 block_index)
{
	int ret = 0;
	BlockBuff_t rb;
	callBackParam_t param;

	memset(&param,0,sizeof(callBackParam_t));
	memset(&rb,0,sizeof(readAbleInfo_t));
	rb.index = block_index;

	//get data
	ret = segment_acquire_blockToRead(segments_,&rb);
	if(ret < 0)
	{
		printf("fail to acquire block, channel[%llu]\n",channel_id);
		return -1;
	}

	//判断是否需要组包
	if(((rb.block->FUs)&(0x1)) == 0x1)
	{
		//重新组包
		if(((rb.block->FUs)&(0x1<<2)) == (0x1<<2))
		{
			//start pack
			if(handle->subpacket.buff)
			{
				free(handle->subpacket.buff);
				handle->subpacket.buff = NULL;
			}

			handle->subpacket.buff = (char*)malloc(rb.block->msgTotalSize);
			if(handle->subpacket.buff == NULL)
			{
				printf("malloc error!!\n");
				goto EXIT;
			}
			memset(handle->subpacket.buff,0,rb.block->msgTotalSize);
			handle->subpacket.totalSize = rb.block->msgTotalSize;
		}

		//memcpy
		if(handle->subpacket.buff)
		{
			memcpy(handle->subpacket.buff+rb.block->msgOffset,rb.buff,rb.block->msgSize);
		}else
		{
			//前面的包没获取到，组的包不完整，则将该包抛掉，直到下一个数据包
			goto EXIT;
		}

		//end pack
		if(((rb.block->FUs)&(0x1<<1)) == (0x1<<1))
		{
			//上抛数据
			param.data = handle->subpacket.buff;
			param.size = handle->subpacket.totalSize;
			param.user = handle->user;
			param.cmd = rb.block->cmd;
			param.channelName = segments_->channelName;
			if(handle->dealData)
			{
				handle->dealData(&param);
			}

			//释放内存
			if(handle->subpacket.buff)
			{
				free(handle->subpacket.buff);
				memset(&(handle->subpacket),0,sizeof(subpacketInfo_t));
				handle->subpacket.buff = NULL;
			}
		}

	}else
	{
		//完整的数据包，直接上抛数据
		param.data = rb.buff;
		param.size = rb.block->msgSize;
		param.user = handle->user;
		param.cmd = rb.block->cmd;
		param.channelName = segments_->channelName;
		if(handle->dealData)
		{
			handle->dealData(&param);
		}

	}

EXIT:

	//释放数据块
	segment_release_readBlock(segments_,rb);

	return 0;
}



static void* thr_dispatcher(void* argv)
{
	dispatcher_t* handle = (dispatcher_t*)argv;
	readAbleInfo_t readable_info;
	Uint32 isShutDown = 0;
	rbtreeNode_t* node = NULL;
	segmentContainer_t* segment_ = NULL;

	while (handle->exit_ == 0)
	{
		AO_GET(&(handle->isShutdown));
		isShutDown = handle->isShutdown;
		if(isShutDown)
		{
			break;
		}

		if (condition_notifier_listen(handle->notifier,100,&readable_info) < 0)
		{
			continue;
		}

		Uint64 channel_id = readable_info.channelId;
		Uint32 block_index = readable_info.blockIndex;

		//get segment
		OS_mutexLock(&(handle->segmentMapMutex));
		node = rbtree_search(handle->segmentMap,channel_id);
		OS_mutexUnlock(&(handle->segmentMapMutex));

		if(node == NULL)
		{
			continue;
		}

		segment_ = (segmentContainer_t*)node->value;
		Uint32 previous_index = segment_->blockIndexes;
		if (block_index != 0 && previous_index != UINT32_MAX)
		{
			if (block_index == previous_index)
			{
				printf("Receive SAME index %u of channel %llu\n", block_index ,channel_id);
			} else if (block_index < previous_index)
			{
				printf("Receive PREVIOUS message. last: %u, now: %u\n", previous_index,block_index);
			} else if (block_index - previous_index > 1)
			{
				printf("Receive JUMP message. last:%u, now: %u\n",previous_index,block_index);
			}
		}
		segment_->blockIndexes = block_index;	//记录这次读到的block块

		//读取数据
		_dispatcher_read_message(handle,segment_->segments,channel_id, block_index);
	}

	return NULL;
}



static int _dispatcher_create(dispatcher_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	//create notifier
	handle->notifier = condition_notifier_init((key_t)handle->notiferId);
	if(handle->notifier == NULL)
	{
		printf("init condition notifoer error!!\n");
		return -1;
	}

	//create thead
	OS_thrCreate(&(handle->thread_),thr_dispatcher,OS_JOINABLE,OS_THR_STACK_SIZE_DEFAULT,(void*)handle);

	return 0;
}



int dispatcher_AddSegment_toListener(dispatcher_t* handle,char *channelName)
{
	if((handle == NULL) || (channelName == NULL))
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	rbtreeNode_t *node = NULL;
	Uint64 channel = (Uint64)BKDRHash(channelName,strlen(channelName));	//create hash
	printf("add segment to listen channel[%s] hash[%llu]\n",channelName,channel);

	segmentContainer_t* segment_ = (segmentContainer_t*)malloc(sizeof(segmentContainer_t));
	if(segment_ == NULL)
	{
		printf("malloc error!!\n");
		return -1;
	}
	memset(segment_,0,sizeof(segmentContainer_t));

	//新增segment
	segment_->segments = segment_init(channelName,READ_ONLY,0);
	if(segment_->segments == NULL)
	{
		printf("segment is init error!!\n");
		return -1;
	}
	segment_->blockIndexes = 0;

	node = (rbtreeNode_t*)malloc(sizeof(rbtreeNode_t));
	if(node == NULL)
	{
		printf("malloc error!!\n");
		segment_unInit(segment_->segments);
	}
	memset(node,0,sizeof(rbtreeNode_t));

	node->key = channel;
	node->value = segment_;

	//插入到红黑树中
	OS_mutexLock(&(handle->segmentMapMutex));
	rbtree_insert(handle->segmentMap,node);
	OS_mutexUnlock(&(handle->segmentMapMutex));

	return 0;
}

int dispatcher_DelSegment_toListener(dispatcher_t* handle,char *channelName)
{
	if((handle == NULL) || (channelName == NULL))
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	rbtreeNode_t *node = NULL;
	Uint64 channel = (Uint64)BKDRHash(channelName,strlen(channelName));	//create hash
	printf("del segment to listen channel[%s] hash[%llu]\n",channelName,channel);

	//查找节点下的数据并释放
	segmentContainer_t* segment = NULL;
	OS_mutexLock(&(handle->segmentMapMutex));
	node = rbtree_search(handle->segmentMap,channel);
	OS_mutexUnlock(&(handle->segmentMapMutex));

	if(node)
	{
		segment = (segmentContainer_t*)node->value;
		if(segment)
		{
			if(segment->segments)
			{
				segment_unInit(segment->segments);
				segment->segments = NULL;
			}
		}
	}

	//删除节点
	OS_mutexLock(&(handle->segmentMapMutex));
	rbtree_delete(handle->segmentMap,channel);
	OS_mutexUnlock(&(handle->segmentMapMutex));

	return 0;
}





