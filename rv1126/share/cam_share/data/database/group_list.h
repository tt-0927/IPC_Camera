#ifndef __FILE_LIST_H__
#define __FILE_LIST_H__
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include <sys/time.h>
#include "db_middle.h"


typedef struct __GROUP_LIST_STRUCT__{

	DataBaseGroup_t groupInfo;
	struct __GROUP_LIST_STRUCT__ *next;
}groupList_t, *groupList_ptr;

/* NOTE:
返回地址的函数需要添加显式生命，否则会有32位与64位不兼容的问题，导致程序段错误
EasonLu ADD 2022-09-03 11:41:59
*/
groupList_ptr group_list_init();

int share_createGroupInfoList(groupList_ptr HEAD);
int group_insertNodeToList(groupList_ptr head, DataBaseGroup_t groupInfo);
int group_findGroupInfo(groupList_ptr head,DataBaseGroup_t *groupInfo);
void group_remove_list(groupList_ptr head) ;

#endif
