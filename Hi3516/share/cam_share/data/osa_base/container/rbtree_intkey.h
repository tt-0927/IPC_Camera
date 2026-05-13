
#ifndef _0S_CODE_SOURCE_CONTAINER_RBTREE_INT_KEY_INCLUDE_
#define _0S_CODE_SOURCE_CONTAINER_RBTREE_INT_KEY_INCLUDE_


#include "rbtree.h"

typedef struct _RBTREE_HANDLE_
{
	struct rb_root root;
	
}rbtreeHandle_t;


typedef struct _RBTREE_NODE_INFO_
{
	unsigned long long key;	//key
	void *value;			//value
	struct rb_node node;	//rbtree node
}rbtreeNode_t;



/*
 * 根据key值查找节点
 * @param[in] handle :rbtreeHandle_t 句柄
 * @param[in] key :key值
 * @param[out] return :success -> 0，failed -> -1
 * */
rbtreeNode_t *rbtree_search(rbtreeHandle_t* handle, unsigned long long key);

/*
 * 插入一个节点
 * @param[in] handle :rbtreeHandle_t 句柄
 * @param[in] data :rbtreeNode_t 节点
 * @param[out] return :success -> 0，failed -> -1
 * */
int rbtree_insert(rbtreeHandle_t* handle, rbtreeNode_t *data);

/*
 * 删除红黑树中的一个节点
 * @param[in] handle :rbtreeHandle_t 句柄
 * @param[out] return :success -> 0，failed -> -1
 * */
void rbtree_delete(rbtreeHandle_t* handle, unsigned long long key);

/*
 * 打印红黑树
 * @param[in] handle :rbtreeHandle_t 句柄
 * @param[out] return :success -> 0，failed -> -1
 * */
void print_rbtree(rbtreeHandle_t* handle);

/*
 * 初始化红黑树
 * @param[out] return :success -> rbtreeHandle_t句柄，failed -> NULL
 * */
rbtreeHandle_t* rbtree_init();

/*
 * 销毁红黑树
 * @param[in] handle :rbtreeHandle_t 句柄
 * @param[out] return :success -> 0，failed -> -1
 * */
int rbtree_unInit(rbtreeHandle_t* handle);

/*
 * 返回红黑树第一个节点
 * @param[in] handle :rbtreeHandle_t 句柄
 * @param[out] return :节点数据
 * */
rbtreeNode_t* rbtree_firstNode(rbtreeHandle_t* handle);

/*
 * 返回红黑树下一个节点
 * @param[in] handle :rbtreeHandle_t 句柄
 * @param[out] return :节点数据
 * */
rbtreeNode_t* rbtree_nextNode(rbtreeHandle_t* handle,rbtreeNode_t* node);

/*
 * 最后一个节点数据
 * @param[in] handle :rbtreeHandle_t 句柄
 * @param[out] return :节点数据
 * */
rbtreeNode_t* rbtree_endNode(rbtreeHandle_t* handle);



#endif //_0S_CODE_SOURCE_CONTAINER_RBTREE_INT_KEY_INCLUDE_

