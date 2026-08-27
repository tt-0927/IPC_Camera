
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hashMap.h"

hashMapHandle_t* hashMap_init()
{
	hashMapHandle_t* handle = (hashMapHandle_t*)malloc(sizeof(hashMapHandle_t));
	memset(handle,0,sizeof(hashMapHandle_t));
	return handle;
}

int hashMap_uninit(hashMapHandle_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!!\n");
		return -1;
	}
	free(handle);
	return 0;
}

int hashMap_insert(hashMapHandle_t* handle,hashKey key,void* value)
{
	if(handle == NULL)
	{
		printf("this argument is null!!\n");
		return -1;
	}

	hashMap_t *tmp = NULL;
	//先查找这个key是否有对应的值
	HASH_FIND_INT(handle->handle, &key, tmp);
	if (tmp != NULL)
	{
		printf("The key exists in hash. \n");
		return -1;
	}

	hashMap_t* pkt = (hashMap_t *)malloc(sizeof(hashMap_t));
	memset(pkt,0,sizeof(hashMap_t));
	pkt->key = key;
	pkt->value = value;

	//insert
	HASH_ADD_INT(handle->handle, key, pkt);
	return 0;
}


void* hashMap_delete(hashMapHandle_t* handle,hashKey key)
{
	if(handle == NULL)
	{
		printf("this argument is null!!\n");
		return NULL;
	}

	hashMap_t *tmp = NULL;
	void* value = NULL;
	HASH_FIND_INT(handle->handle, &key, tmp);
	if (tmp == NULL)
	{
		printf ("find not item.\n");
		return NULL;
	}

	/*删除节点不会释放你的空间必须自己释放*/
	HASH_DEL(handle->handle, tmp);
	value = tmp->value;
	if(tmp)
	{
		free(tmp);
		tmp = NULL;
	}
	return value;
}

void* hashMap_find(hashMapHandle_t* handle,hashKey key)
{
	if(handle == NULL)
	{
		printf("this argument is null!!\n");
		return NULL;
	}

	hashMap_t *tmp = NULL;
	HASH_FIND_INT(handle->handle, &key, tmp);
	if (tmp == NULL)
	{
		return NULL;
	}
	return tmp->value;
}


int hashMap_count(hashMapHandle_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is null!!\n");
		return -1;
	}
	return HASH_COUNT(handle->handle);
}



int hashMap_demo()
{
	hashMapHandle_t* handle = hashMap_init();
	int ret = 0;
	int i = 0;
	hashKey key;

	printf("this cout:%d\n",hashMap_count(handle));

	//insert
	char* value = NULL;
	for(i = 0;i < 10;i++)
	{
		if(i == 9)
		{
			sprintf(key.c_key,"aa%d",i);
		}else
		{
			key.i_key = i;
		}
		char *valueName = (char*)malloc(128);
		memset(valueName,0,sizeof(char*)*128);
		sprintf(valueName,"value:%d",i);
		ret = hashMap_insert(handle,key,valueName);
		if(ret < 0)
		{
			printf("insert error key[%d]\n",i);
		}
		printf("insert value[%s]\n",valueName);
	}
	printf("this cout:%d\n",hashMap_count(handle));

	//find
	for(i = 0;i < 13;i++)
	{
		if(i == 9)
		{
			sprintf(key.c_key,"aa%d",i);
		}else
		{
			key.i_key = i;
		}
		value = hashMap_find(handle,key);
		if(value)
		{
			printf("find key[%lld] value[%s]\n",key.i_key,value);
		}
	}
	printf("this cout:%d\n",hashMap_count(handle));

	//delete
	for(i = 0;i < 13;i++)
	{
		if(i == 9)
		{
			sprintf(key.c_key,"aa%d",i);
		}else
		{
			key.i_key = i;
		}
		value = hashMap_delete(handle,key);
		if(value)
		{
			printf("delete success!! key[%d] value[%s] count[%d]\n",i,value,hashMap_count(handle));
		}
	}
	printf("this cout:%d\n",hashMap_count(handle));

	return 0;
}







