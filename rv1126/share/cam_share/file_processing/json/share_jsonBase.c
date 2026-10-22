
#include <stdio.h>
#include <string.h>
#include "share_jsonBase.h"

#define MAX(x,y) (x>y?y:x)

cJSON* json_init_head(void)
{
	return cJSON_CreateObject();
}

BOOL json_delete_head(cJSON *jsonObjHead)
{
	if(jsonObjHead == NULL)
	{
		return FALSE;
	}
	cJSON_Delete(jsonObjHead);
	return TRUE;
}


BOOL json_add_double(cJSON *jsonObj,char *nodeName,double addValue)
{
	if(jsonObj == NULL || nodeName == NULL)
	{
			printf("json_set_double error!\n");
			return FALSE;
	}
	if(NULL == cJSON_AddNumberToObject(jsonObj,nodeName, addValue))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL json_add_int(cJSON *jsonObj,char *nodeName,int addValue)
{
	if(jsonObj == NULL || nodeName == NULL)
	{
			printf("json_set_double error!\n");
			return FALSE;
	}
	if(NULL == cJSON_AddNumberToObject(jsonObj,nodeName, addValue))
	{
		return FALSE;
	}
	return TRUE;
}


BOOL json_get_int(char *jsonItem,char *nodeName,int *Value)
{
	cJSON* jsonObjHead = NULL;
	if(jsonItem == NULL || nodeName == NULL || Value == NULL)
	{
			printf("json_get_int error!\n");
			return FALSE;
	}
	jsonObjHead = cJSON_Parse(jsonItem);
	if(jsonObjHead == NULL)
	{
			return FALSE;
	}
	if(cJSON_GetObjectItem(jsonObjHead,nodeName))
	{
		*Value = cJSON_GetObjectItem(jsonObjHead,nodeName)->valueint;
	}
	else
	{
		cJSON_Delete(jsonObjHead);
		return FALSE;
	}
	cJSON_Delete(jsonObjHead);

	#if 0

	jsonItemHead == cJSON_GetObjectItem(jsonObjHead,nodeName);
	if(jsonItemHead == NULL)
	{
				printf("line:%d\n",__LINE__);
		return FALSE;
	}
				printf("line:%d\n",__LINE__);
	*Value = jsonItemHead->valueint;
	printf("jsonItemHead->valueint = %d\n",jsonItemHead->valueint);
	cJSON_Delete(jsonItemHead);
	cJSON_Delete(jsonObjHead);	
	#endif
	return TRUE;
}

BOOL json_get_int1(char *jsonItem,char *nodeName,char *nodeName1,int *Value)
{
	cJSON* jsonObjHead = NULL;
	cJSON* jsonItemHead = NULL;
	if(jsonItem == NULL || nodeName == NULL || Value == NULL)
	{
		printf("json_get_int error!\n");
		return FALSE;
	}
	jsonObjHead = cJSON_Parse(jsonItem);
	if(jsonObjHead == NULL)
	{
		cJSON_Delete(jsonObjHead);
		return FALSE;
	}
	jsonItemHead = cJSON_GetObjectItem(jsonObjHead,nodeName);

	if(jsonItemHead == NULL)
	{
		cJSON_Delete(jsonObjHead);
		return FALSE;
	}	
	if(cJSON_GetObjectItem(jsonItemHead,nodeName1))
	{
		*Value = cJSON_GetObjectItem(jsonItemHead,nodeName1)->valueint;
	}
	else
	{
		cJSON_Delete(jsonObjHead);
		return FALSE;
	}

	cJSON_Delete(jsonObjHead);

	#if 0

	jsonItemHead == cJSON_GetObjectItem(jsonObjHead,nodeName);
	if(jsonItemHead == NULL)
	{
				printf("line:%d\n",__LINE__);
		return FALSE;
	}
				printf("line:%d\n",__LINE__);
	*Value = jsonItemHead->valueint;
	printf("jsonItemHead->valueint = %d\n",jsonItemHead->valueint);
	cJSON_Delete(jsonItemHead);
	cJSON_Delete(jsonObjHead);	
	#endif
	return TRUE;
}





BOOL json_add_char(cJSON *jsonObj,char *nodeName,char *string)
{
	if(jsonObj == NULL || nodeName == NULL || string == NULL)
	{
			printf("json_set_char error!\n");
			return FALSE;
	}
	if(NULL == cJSON_AddStringToObject(jsonObj,nodeName, string))
	{
		return FALSE;
	}
	return TRUE;
}



BOOL json_get_char(char *jsonItem,char *nodeName,char *Value,int nLen)
{
	cJSON* jsonObjHead = NULL;
	if(jsonItem == NULL || nodeName == NULL || Value == NULL)
	{
			printf("json_get_char error!\n");
			return FALSE;
	}
	jsonObjHead = cJSON_Parse(jsonItem);
	if(jsonObjHead == NULL)
	{
			return FALSE;
	}

	if(cJSON_GetObjectItem(jsonObjHead,nodeName))
	{
		if(cJSON_GetObjectItem(jsonObjHead,nodeName)->valuestring)
		{
			memcpy(Value,cJSON_GetObjectItem(jsonObjHead,nodeName)->valuestring,MAX(strlen(cJSON_GetObjectItem(jsonObjHead,nodeName)->valuestring),(size_t)nLen));
		}
		else
		{
			cJSON_Delete(jsonObjHead);
			return FALSE;
		}

	}
	else
	{
		cJSON_Delete(jsonObjHead);
		return FALSE;
	}

	cJSON_Delete(jsonObjHead);

	return TRUE;
}


BOOL json_get_char1(char *jsonItem,char *nodeName,char *nodeName1,char *Value,int nLen)
{
	cJSON* jsonObjHead = NULL;
	cJSON* jsonItemHead = NULL;
	cJSON* jsonNode = NULL;
	if(jsonItem == NULL || nodeName == NULL || Value == NULL)
	{
			printf("json_get_char error!\n");
			return FALSE;
	}
	jsonObjHead = cJSON_Parse(jsonItem);
	if(jsonObjHead == NULL)
	{
			return FALSE;
	}

	jsonItemHead = cJSON_GetObjectItem(jsonObjHead,nodeName);

	if(jsonItemHead == NULL)
	{
		cJSON_Delete(jsonObjHead);
		return FALSE;
	}

	if(cJSON_GetObjectItem(jsonItemHead,nodeName1))
	{
		if(cJSON_GetObjectItem(jsonItemHead,nodeName1)->valuestring)
		{
			memcpy(Value,cJSON_GetObjectItem(jsonItemHead,nodeName1)->valuestring,MAX(strlen(cJSON_GetObjectItem(jsonItemHead,nodeName1)->valuestring),(size_t)nLen));
		}

	}
	else
	{
		cJSON_Delete(jsonObjHead);
		return FALSE;
	}

	cJSON_Delete(jsonObjHead);

	return TRUE;
}


BOOL json_GetObjectItem_int(cJSON *jsonObj,char *nodeName,int *nData)
{
	if(jsonObj == NULL || nodeName == NULL)
	{
		printf("[analysis] GetObjectItem_int data is NULL\n");
		return FALSE;
	}

	if(cJSON_GetObjectItem(jsonObj,nodeName))
	{
		*nData = cJSON_GetObjectItem(jsonObj,nodeName)->valueint;
		return TRUE;
	}
	else
	{
		printf("[analysis] GetObjectItem_int nodeName is NULL [%s]\n",nodeName);
	}
		
	return FALSE;

}

BOOL json_GetObjectItem_string(cJSON *jsonObj,char *nodeName,char *pData,int nLen)
{
	if(jsonObj == NULL || nodeName == NULL)
	{
		printf("[analysis] GetObjectItem_int data is NULL\n");
		return FALSE;
	}

	if(cJSON_GetObjectItem(jsonObj,nodeName))
	{
		if(cJSON_GetObjectItem(jsonObj,nodeName)->valuestring)
		{
			strncpy(pData,cJSON_GetObjectItem(jsonObj,nodeName)->valuestring,nLen);
			return TRUE;
		}
	}
	else
	{
		printf("[analysis] GetObjectItem_int nodeName is NULL [%s]\n",nodeName);
	}
		
	return FALSE;

}


BOOL json_outOf_String(cJSON *jsonObj,char **outJson)
{
	if(NULL == jsonObj)
	{
		return FALSE;
	}
	*outJson=cJSON_Print(jsonObj);
	return TRUE;
}


