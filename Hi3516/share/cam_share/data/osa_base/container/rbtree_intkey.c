
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rbtree_intkey.h"



static int _rbtree_free_node(rbtreeNode_t* node)
{
	if(node == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}
	
	if(node->value)
	{
		free(node->value);
		node->value = NULL;
	}
	
	free(node);
	node = NULL;
	return 0;
}


rbtreeNode_t *rbtree_search(rbtreeHandle_t* handle, unsigned long long key)
{
	struct rb_root *root = &(handle->root);
    struct rb_node *node = root->rb_node;
 
    while (node) 
	{
		rbtreeNode_t *data = container_of(node, rbtreeNode_t, node);
	 
		if (key < data->key)
			node = node->rb_left;
		else if (key > data->key)
			node = node->rb_right;
		else
			return data;
    }
    
    return NULL;
}
 
int rbtree_insert(rbtreeHandle_t* handle, rbtreeNode_t *data)
{
	struct rb_root *root = &(handle->root);
    struct rb_node **tmp = &(root->rb_node), *parent = NULL;
 
    /* Figure out where to put new node */
    while (*tmp) 
	{
		rbtreeNode_t *this = container_of(*tmp, rbtreeNode_t, node);
	 
		parent = *tmp;
		if (data->key < this->key)
			tmp = &((*tmp)->rb_left);
		else if (data->key > this->key)
			tmp = &((*tmp)->rb_right);
		else 
			return -1;
    }
    
    /* Add new node and rebalance tree. */
    rb_link_node(&data->node, parent, tmp);
    rb_insert_color(&data->node, root);
    
    return 0;
}
 
void rbtree_delete(rbtreeHandle_t* handle, unsigned long long key)
{
	struct rb_root *root = &(handle->root);
    rbtreeNode_t *data = rbtree_search(handle, key);
    if (!data) 
	{ 
		fprintf(stderr, "Not found %lld.\n", key);
		return;
    }
    
    rb_erase(&data->node, root);
    _rbtree_free_node(data);
}
 
void print_rbtree(rbtreeHandle_t* handle)
{
	struct rb_root *root = &(handle->root);
    struct rb_node *node = NULL;
    
    for (node = rb_first(root); node; node = rb_next(node))
	printf("%lld ", rb_entry(node, rbtreeNode_t, node)->key);
    
    printf("\n");
}


rbtreeHandle_t* rbtree_init()
{
	
	rbtreeHandle_t* handle = (rbtreeHandle_t*)malloc(sizeof(rbtreeHandle_t));
	if(handle == NULL)
	{
		printf("malloc error!!\n");
		return NULL;
	}
	memset(handle,0,sizeof(rbtreeHandle_t));
	
	handle->root = RB_ROOT;
	
	return handle;
}


int rbtree_unInit(rbtreeHandle_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}
	
	struct rb_root *root = &(handle->root);
    struct rb_node *node = NULL;
    
    for (node = rb_first(root); node; node = rb_next(node))
	{
		printf("del key[%lld]\n", rb_entry(node, rbtreeNode_t, node)->key);
		rbtree_delete(handle,rb_entry(node, rbtreeNode_t, node)->key);
    }

	free(handle);
	
	return 0;
}


rbtreeNode_t* rbtree_firstNode(rbtreeHandle_t* handle)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return NULL;
	}

	struct rb_root *root = &(handle->root);
    struct rb_node *node = rb_first(root);
    if(node)
    {
    	return rb_entry(node, rbtreeNode_t, node);
    }
    return NULL;
}
rbtreeNode_t* rbtree_nextNode(rbtreeHandle_t* handle,rbtreeNode_t* node)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!\n");
		return NULL;
	}

	struct rb_root *root = &(handle->root);
	struct rb_node *rbnode = rb_next(&(node->node));
    if(rbnode)
    {
    	return rb_entry(rbnode, rbtreeNode_t, node);
    }
    return NULL;
}
rbtreeNode_t* rbtree_endNode(rbtreeHandle_t* handle)
{
	return NULL;
}


/*
 * 测试代码
 * */
int main_test()
{
	int ret = 0;
	rbtreeHandle_t *handle = NULL;
	rbtreeNode_t *node = NULL;
	handle = rbtree_init();
	
	int i = 0;
	for(i = 0;i < 10;i++)
	{
		node = (rbtreeNode_t*)malloc(sizeof(rbtreeNode_t));
		memset(node,0,sizeof(rbtreeNode_t));
		node->value = (char *)malloc(32);
		memset(node->value,0,32);
		sprintf(node->value,"11111%d",i);
		node->key = i;
		rbtree_insert(handle,node);
	}
	
	print_rbtree(handle);
	rbtree_delete(handle,2);
	print_rbtree(handle);
	rbtree_unInit(handle);
	
	return 0;
}











