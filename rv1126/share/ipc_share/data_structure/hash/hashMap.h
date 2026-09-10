
#ifndef _OS_CORE_SOURCE_CONTAINER_HASH_MAP_INCLUDE_
#define _OS_CORE_SOURCE_CONTAINER_HASH_MAP_INCLUDE_


#include "uthash.h"

typedef union
{
	unsigned long long i_key;
	char c_key[128];
}hashKey;

typedef struct _HASH_MAP_T_
{
	hashKey key;	/* 这个是用来做hash的key值 */
	void* value;	/* value值 */
	UT_hash_handle hh;
}hashMap_t;

typedef struct _HASH_MAP_HANDLE_
{
	hashMap_t* handle;
}hashMapHandle_t;

hashMapHandle_t* hashMap_init();

int hashMap_uninit(hashMapHandle_t* handle);

int hashMap_insert(hashMapHandle_t* handle,hashKey key,void* value);

/*
 * 删除节点
 * @[out] return :若存在该key，则返回该节点的value值，用户需要自行处理该值的内存，否则容易造成内存泄漏
 * */
void* hashMap_delete(hashMapHandle_t* handle,hashKey key);

void* hashMap_find(hashMapHandle_t* handle,hashKey key);

int hashMap_count(hashMapHandle_t* handle);

int hashMap_demo();

#endif //_OS_CORE_SOURCE_CONTAINER_HASH_MAP_INCLUDE_

