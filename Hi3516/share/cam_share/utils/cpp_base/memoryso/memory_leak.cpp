#include<stdio.h>
#include <list>
#include<stdlib.h>
#include<pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "memory_leak.h"


#define CHECKDEBUG
#define WARNINGINFO "please check you code ok"
#define MALLOC_NUMNOFREE 100//申请了1000次还没释放
using namespace std;



typedef list<void*> List_Addr_Info_t;
typedef struct  Memory_leak_MallocInfo
{
	List_Addr_Info_t* list_addr;
	char callFuntionInfo[128];//从哪个地方进来申请的,包含文件第几行详细信息
	unsigned int mallocSize;//一共申请了多少，扣掉释放的
	unsigned int mallocnum;//一共申请了多少次，还有多少次没释放

}Memory_leak_MallocInfo_t;

typedef list<Memory_leak_MallocInfo_t*> List_MemoryLeat_Info_t;


class Memory_Leak_Check
{
public:
	Memory_Leak_Check();
	~Memory_Leak_Check();
public:
	void *sdk_malloc(int length, const char * filename,const char * function, int linenum);
	void sdk_free(void* addr, const char * filename,const char * function, int linenum);
private:
	pthread_mutex_t list_lock;
	List_MemoryLeat_Info_t listCheckHandle;
	 pid_t mypid;

};

Memory_Leak_Check* g_memory_handle = NULL;
Memory_Leak_Check::Memory_Leak_Check()
{
	mypid = getpid();
	pthread_mutex_init(&list_lock, NULL);
};
Memory_Leak_Check::~Memory_Leak_Check()
{

};
void * Memory_Leak_Check::sdk_malloc(int length, const char * filename,const char * function, int linenum)
{
	void *addr = malloc(length);
	Memory_leak_MallocInfo_t * memory_malloc_info = NULL;
	char callFuntionInfo[128] = {0};
	if(addr == NULL)
	{
		printf("Memory_Leak_Check malloc isfail %s\n", WARNINGINFO);
		return NULL;
	}
	sprintf(callFuntionInfo,"%s:%d", filename, linenum);
	pthread_mutex_lock(&list_lock);
	List_MemoryLeat_Info_t::iterator listfind = listCheckHandle.end();
	for(listfind = listCheckHandle.begin(); listfind != listCheckHandle.end(); listfind++)
	{

		if(0 == strcmp(callFuntionInfo, ((*(listfind))->callFuntionInfo)))
		{
			memory_malloc_info = *listfind;
			memory_malloc_info->mallocnum++;
			if(memory_malloc_info->mallocnum > MALLOC_NUMNOFREE)
			{
				// printf("pid:%d Memory_leak_MallocInfo:%s malloc is no free in %s",mypid, WARNINGINFO, callFuntionInfo);
			}
			(memory_malloc_info->list_addr)->push_back(addr);
			//memory_malloc_info->mallocSize+=length;
			if(memory_malloc_info->mallocnum > 5)
			{
				// printf("pid:%d Memory_leak_MallocInfo:filename:%s function:%s linenum:%d  mallocnum:%d size:%d\n",
						// mypid, filename, function, linenum,  memory_malloc_info->mallocnum, (memory_malloc_info->list_addr)->size());
			}

		}
	}


	if(memory_malloc_info == NULL)
	{
		memory_malloc_info = (Memory_leak_MallocInfo_t*)malloc(sizeof(Memory_leak_MallocInfo_t));
		if(memory_malloc_info == NULL)
		{
			printf("mypid:%d Memory_leak_MallocInfo:memory is no enough %s\n", mypid, WARNINGINFO);
			pthread_mutex_unlock(&list_lock);
			return addr;
		}
		memset(memory_malloc_info, 0, sizeof(Memory_leak_MallocInfo_t));

		memory_malloc_info->list_addr = new(List_Addr_Info_t);
		(memory_malloc_info->list_addr)->push_back(addr);

		memory_malloc_info->mallocnum++;
		//memory_malloc_info->mallocSize+=length;
		strncpy(memory_malloc_info->callFuntionInfo, callFuntionInfo, sizeof(memory_malloc_info->callFuntionInfo));

		listCheckHandle.push_back(memory_malloc_info);
		//printf("mypid:%d first Memory_leak_MallocInfo:filename:%s function:%s linenum:%d\n",mypid, filename, function, linenum);
	}
	pthread_mutex_unlock(&list_lock);




	return addr;

};
void  Memory_Leak_Check::sdk_free(void* addr, const char * filename,const char * function, int linenum)
{

	Memory_leak_MallocInfo_t * memory_malloc_info = NULL;
	char callFuntionInfo[128] = {0};
	int find = 0;
	if(addr == NULL)
	{
		printf("Memory_leak_MallocInfo Memory_Leak_Check malloc isfail %s\n", WARNINGINFO);
		return ;
	}
	sprintf(callFuntionInfo,"%s:%d", filename, linenum);

	pthread_mutex_lock(&list_lock);
	List_MemoryLeat_Info_t::iterator listfind = listCheckHandle.end();

	for(listfind = listCheckHandle.begin(); listfind != listCheckHandle.end(); listfind++)
	{

		memory_malloc_info = *listfind;
		List_Addr_Info_t::iterator listAddrfind = (memory_malloc_info->list_addr)->end();
		for(listAddrfind = (memory_malloc_info->list_addr)->begin(); listAddrfind != (memory_malloc_info->list_addr)->end(); listAddrfind++)
		{
			if(*listAddrfind == addr)
			{
				memory_malloc_info->mallocnum--;
				free(addr);
				(memory_malloc_info->list_addr)->erase(listAddrfind);
				find = 1;

//				printf("==============Memory_leak_MallocInfo free filename:%s function:%s linenum:%d mallocnum:%d"
//						"malloc callfunction:%s listsize:%d\n", filename, function, linenum, memory_malloc_info->mallocnum,
//						memory_malloc_info->callFuntionInfo, (memory_malloc_info->list_addr)->size());
				break;
			}
		}
		if(find == 1)
		{
			break;
		}


	}
	if(find == 0)
	{
		printf("mypid:%d %s free is error ==============Memory_leak_MallocInfo free filename:%s function:%s linenum:%d\n",
				mypid, WARNINGINFO, filename, function, linenum );
	}
	pthread_mutex_unlock(&list_lock);

}
void* memory_leak_malloc(int length, const char * filename,const char * function, int linenum)
{
#ifdef CHECKDEBUG
	if(g_memory_handle == NULL)
	{
		g_memory_handle =new Memory_Leak_Check();
	}
	return g_memory_handle->sdk_malloc(length, filename,function, linenum);
#else
	return malloc(length);
#endif


}
void memory_leak_free(void* addr, const char * filename,const char * function, int linenum)
{
#ifdef CHECKDEBUG
	if(g_memory_handle == NULL)
	{
		printf("memory_leak_free : WARNINGINFO:%s", WARNINGINFO);
		return ;
	}
	return g_memory_handle->sdk_free(addr, filename,function, linenum);
#else
	free(addr);
#endif

}

