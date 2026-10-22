#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include "list_single.h"


typedef struct _HeadNode
{
	void* pData;
	int nSize;
	OS_DataNode* next;
}HeadNode;

typedef OS_DataNode* ptrDataNode;
typedef HeadNode* ptrHeadNode;

static void OS_ListMakeEmpty(ptrHeadNode pHead);
static int OS_ListIsEmpty(ptrHeadNode pHead);
static ptrDataNode OS_ListInsert(ptrDataNode prev, void *data);
static ptrDataNode OS_FindNodeTail(ptrHeadNode pHead);

OS_listHndl OS_listCreate()
{
	ptrHeadNode pHead = (ptrHeadNode)malloc(sizeof(HeadNode));
	if(NULL == pHead)
	{
		return NULL;
	}

	OS_ListMakeEmpty(pHead);
	return pHead;
}

static void OS_ListMakeEmpty(ptrHeadNode pHead)
{
	memset(pHead, 0 ,sizeof(*pHead));
}

static int OS_ListIsEmpty(ptrHeadNode pHead)
{
	return pHead->nSize;
}


static ptrDataNode OS_FindNodeTail(ptrHeadNode pHead)
{
	ptrDataNode pNode;
	if(pHead == NULL)
	{
		return NULL;
	}
	pNode = pHead->next;
	for(;pNode->next != NULL; pNode = pNode->next);
	return pNode;

}
static ptrDataNode OS_ListInsert(ptrDataNode prev, void *data)
{
	ptrDataNode pCurNode = (ptrDataNode)malloc(sizeof(*pCurNode));
	if(NULL == pCurNode)
	{
		return NULL;
	}
	pCurNode->pData = data;

	pCurNode->next = prev->next;
	prev->next = pCurNode;
	return pCurNode;
}
OS_listNode_t OS_listPushFront(OS_listHndl pHeadHandle, void* pData)
{
	ptrHeadNode pHead =(ptrHeadNode)pHeadHandle;
	ptrDataNode pCurNode;
	if(pHead == NULL)
	{
		return NULL;
	}
	pCurNode = (ptrDataNode)malloc(sizeof(*pCurNode));
	if(pCurNode == NULL)
	{
		return NULL;
	}
	pCurNode->pData = pData;

	pCurNode->next = pHead->next;
	pHead->next = pCurNode;
	(pHead->nSize)++;
	return pCurNode;
}
OS_listNode_t OS_listPushBack(OS_listHndl pHeadHandle, void* pData)
{
	ptrHeadNode pHead =(ptrHeadNode)pHeadHandle;
	if(pHead == NULL)
	{
		return NULL;
	}
	if(OS_ListIsEmpty(pHead) == 0)
	{
		return OS_listPushFront(pHead,  pData);
	}
	ptrDataNode pdataNode = OS_FindNodeTail(pHead);
	(pHead->nSize)++;
	return OS_ListInsert(pdataNode, pData);
}
OS_listData_t OS_listPopFront(OS_listHndl pHeadHandle)
{
	ptrHeadNode pHead =(ptrHeadNode)pHeadHandle;
	void *pdata = NULL;
	if(pHead == NULL)
	{
		return NULL;
	}
	if(OS_ListIsEmpty(pHead) != 0)
	{
		ptrDataNode pdataNode = pHead->next;
		pdata = pdataNode->pData;
		pHead->next = pdataNode->next;

		free(pdataNode);
		pdataNode = NULL;
		(pHead->nSize)--;
	}
	return pdata;
}
OS_listNode_t OS_listBegin(OS_listHndl pHeadHandle)
{
	ptrHeadNode pHead =(ptrHeadNode)pHeadHandle;

	if(pHead == NULL)
	{
		return NULL;
	}
	OS_listNode_t pNode= pHead->next;
	return pNode;
}

OS_listNode_t OS_listNext(OS_listHndl pHeadHandle, OS_listNode_t pCurNode)
{
	ptrDataNode pdataNode = pCurNode;
	if(pdataNode == NULL)
	{
		return NULL;
	}
	return pdataNode->next;
}

OS_listNode_t OS_listEnd(OS_listHndl pHeadHandle)
{
	/* 返回NULL表示已经到达最后一个节点了 */
	return NULL;
}

OS_listData_t OS_listPopBack(OS_listHndl pHeadHandle)
{
	ptrHeadNode pHead =(ptrHeadNode)pHeadHandle;
	void *pdata = NULL;
	if(pHead == NULL)
	{
		return NULL;
	}
	if(OS_ListIsEmpty(pHead) != 0)
	{
		ptrDataNode pdataNode = OS_FindNodeTail(pHead);
		pdata = pdataNode->pData;
		free(pdataNode);
		pdataNode = NULL;
		(pHead->nSize)--;
	}
	return pdata;
}


OS_listData_t OS_listFront(OS_listHndl pHeadHandle)
{
	ptrHeadNode pHead =(ptrHeadNode)pHeadHandle;
	void *pdata = NULL;
	if(pHead == NULL)
	{
		return NULL;
	}
	if(OS_ListIsEmpty(pHead) != 0)
	{
		ptrDataNode pdataNode = pHead->next;
		pdata = pdataNode->pData;
	}
	return pdata;
}
OS_listData_t OS_listBack(OS_listHndl pHeadHandle)
{
	ptrHeadNode pHead =(ptrHeadNode)pHeadHandle;
	void *pdata = NULL;
	if(pHead == NULL)
	{
		return NULL;
	}
	if(OS_ListIsEmpty(pHead) != 0)
	{
		ptrDataNode pdataNode = OS_FindNodeTail(pHead);
		pdata = pdataNode->pData;
	}
	return pdata;
}



int OS_listDestory(OS_listHndl pListHandle)
{
	ptrHeadNode pHead =(ptrHeadNode)pListHandle;
	if(pHead == NULL)
	{
		return -1;
	}
	ptrDataNode pNode = pHead->next;
	ptrDataNode pNodeBak;
	while(pNode !=  NULL)
	{
		pNodeBak = pNode;
		pNode = pNode->next;
		free(pNodeBak);//销毁节点
	}
	free(pHead);//销毁头
	return 0;
}

int OS_listEarse(OS_listHndl pHeadHandle, void* pData)
{
	ptrHeadNode pHead =(ptrHeadNode)pHeadHandle;
	if(pHead == NULL)
	{
		return -1;
	}
	ptrDataNode pNode = pHead->next;
	ptrDataNode pNodeBak = NULL;
	ptrDataNode ptrPrvNode = NULL;
	int nCount = 0;
 	while(pNode !=  NULL)
	{
		pNodeBak = pNode;
		//删除特定节点
		if(pNode->pData == pData)
		{
			pNode = pNode->next;
			if(pNodeBak == pHead->next)
			{
				pHead->next = pNode;
			}
			else
			{
				if(ptrPrvNode)
				{
					ptrPrvNode->next = pNode;
				}
			}
			printf("OS_listEarse nCount:%d\n",nCount);
			free(pNodeBak);//销毁节点
			pHead->nSize--;
			break;
		}
		nCount++;
		ptrPrvNode = pNode;
		pNode = pNode->next;
	}
 	return 0;
}
int OS_listClear(OS_listHndl pListHandle)
{
	ptrHeadNode pHead = (ptrHeadNode)pListHandle;
	if(pHead == NULL)
	{
		return -1;
	}
	ptrDataNode pNode = pHead->next;
	ptrDataNode pNodeBak;
	while(pNode !=  NULL)
	{
		pNodeBak = pNode;
		pNode = pNode->next;
		free(pNodeBak);//销毁节点
	}
	OS_ListMakeEmpty(pHead);
	return 0;
}
int OS_listSize(OS_listHndl pHeadHandle)
{
	ptrHeadNode pHead =(ptrHeadNode)pHeadHandle;
	if(pHead == NULL)
	{
		return -1;
	}
	return pHead->nSize;
}






