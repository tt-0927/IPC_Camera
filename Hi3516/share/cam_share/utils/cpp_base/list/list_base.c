#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include "list_base.h"
struct node;


typedef struct _HeadNode
{
	void* pData;
	int nSize;
	struct node* next;
}HeadNode;

typedef DataNode* ptrDataNode;
typedef HeadNode* ptrHeadNode;

static void ListMakeEmpty(ptrHeadNode pHead);
static int ListIsEmpty(ptrHeadNode pHead);
static ptrDataNode ListInsert(ptrDataNode prev, void *data);
static ptrDataNode FindNodeTail(ptrHeadNode pHead);

List_Handle_t list_create()
{
	ptrHeadNode pHead = (ptrHeadNode)malloc(sizeof(*pHead));
	if(NULL == pHead)
	{
		return NULL;
	}

	ListMakeEmpty(pHead);
	return pHead;
}

static void ListMakeEmpty(ptrHeadNode pHead)
{
	memset(pHead, 0 ,sizeof(*pHead));
}

static int ListIsEmpty(ptrHeadNode pHead)
{
	return pHead->nSize;
}


static ptrDataNode FindNodeTail(ptrHeadNode pHead)

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
static ptrDataNode ListInsert(ptrDataNode prev, void *data)
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
List_CurNode_t list_push_front(List_Handle_t pHeadHandle, void* pData)
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
List_CurNode_t list_push_back(List_Handle_t pHeadHandle, void* pData)
{
	ptrHeadNode pHead =(ptrHeadNode)pHeadHandle;
	if(pHead == NULL)
	{
		return NULL;
	}
	if(ListIsEmpty(pHead) == 0)
	{
		return list_push_front(pHead,  pData);
	}
	ptrDataNode pdataNode = FindNodeTail(pHead);
	(pHead->nSize)++;
	return ListInsert(pdataNode, pData);
}
List_NodeData_t list_pop_front(List_Handle_t pHeadHandle)
{
	ptrHeadNode pHead =(ptrHeadNode)pHeadHandle;
	void *pdata = NULL;
	if(pHead == NULL)
	{
		return NULL;
	}
	if(ListIsEmpty(pHead) != 0)
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
List_CurNode_t list_getdata(List_CurNode_t pCurNode)
{
	ptrDataNode pdataNode = (ptrDataNode)pCurNode;
	void *pdata = NULL;
	pdata = pdataNode->pData;
	return pdata;
}
List_CurNode_t list_begin(List_Handle_t pHeadHandle)
{
	ptrHeadNode pHead =(ptrHeadNode)pHeadHandle;

	if(pHead == NULL)
	{
		return NULL;
	}
	List_CurNode_t pNode= pHead->next;
	return pNode;
}

List_CurNode_t list_next(List_Handle_t pHeadHandle, List_CurNode_t pCurNode)
{
	ptrDataNode pdataNode = pCurNode;
	if(pdataNode == NULL)
	{
		return NULL;
	}
	return pdataNode->next;
}

List_CurNode_t list_end(List_Handle_t pHeadHandle)
{
	return NULL;
}

List_NodeData_t list_pop_back(List_Handle_t pHeadHandle)
{
	ptrHeadNode pHead =(ptrHeadNode)pHeadHandle;
	void *pdata = NULL;
	if(pHead == NULL)
	{
		return NULL;
	}
	if(ListIsEmpty(pHead) != 0)
	{
		ptrDataNode pdataNode = FindNodeTail(pHead);
		pdata = pdataNode->pData;
		free(pdataNode);
		pdataNode = NULL;
		(pHead->nSize)--;
	}
	return pdata;
}
int list_destory(List_Handle_t pListHandle)
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

int list_earse_data(List_Handle_t pHeadHandle, void* pData)
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
			// printf("list_earse_data nCount:%d\n",nCount);
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
int list_clear(List_Handle_t pListHandle)
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
	ListMakeEmpty(pHead);
	return 0;
}
int list_size(List_Handle_t pHeadHandle)
{
	ptrHeadNode pHead =(ptrHeadNode)pHeadHandle;
	if(pHead == NULL)
	{
		return -1;
	}
	return pHead->nSize;
}


