

#ifndef _OSA_LIST_BASE_H_
#define _OSA_LIST_BASE_H_

#include <os.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef  void* OS_listHndl;//链表操作句柄
typedef void* OS_listData_t;//用户数据
typedef void* OS_listNode_t;//链表节点

typedef struct _LIST_NODE_
{
	void* pData;
	struct _LIST_NODE_* next;
}OS_DataNode;


OS_listHndl   OS_listCreate();
OS_listData_t OS_listPopBack(OS_listHndl pHeadHandle);
OS_listData_t OS_listPopFront(OS_listHndl pHeadHandle);
OS_listNode_t OS_listPushBack(OS_listHndl pHeadHandle, void* pData);
OS_listNode_t OS_listPushFront(OS_listHndl pHeadHandle, void* pData);
OS_listNode_t OS_listBegin(OS_listHndl pHeadHandle);
OS_listNode_t OS_listNext(OS_listHndl pHeadHandle, OS_listNode_t pCurNode);
/* 返回NULL表示已经到达最后一个节点了 */
OS_listNode_t OS_listEnd(OS_listHndl pHeadHandle);

//只获取第一个节点/最后一个节点，不删除
OS_listData_t OS_listFront(OS_listHndl pHeadHandle);
OS_listData_t OS_listBack(OS_listHndl pHeadHandle);

int OS_listEarse(OS_listHndl pHeadHandle, void* pData);
int OS_listSize(OS_listHndl pHeadHandle);
int OS_listDestory(OS_listHndl pList_handle);
int OS_listClear(OS_listHndl pList_handle);



#ifdef __cplusplus
}
#endif

#endif //_OSA_LIST_BASE_H_

