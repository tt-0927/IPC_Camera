#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "group_list.h"
//#include "db_middle.h"


groupList_ptr group_list_init()
{
	groupList_ptr head;
	head = malloc(sizeof(groupList_t));
	if(head == NULL)
	{
		perror("request list head failed\n");
		return NULL;
	}
	head->next = NULL;
	return head;
}

#if 1
int group_insertNodeToList(groupList_ptr head, DataBaseGroup_t groupInfo)
{
	groupList_ptr new_node;
	new_node = malloc(sizeof(groupList_t));
	if(new_node == NULL)
	{
		perror("request node failed\n");
		return -1;
	}
	memcpy(&new_node->groupInfo,&groupInfo,sizeof(DataBaseGroup_t));
	new_node->next = head->next;
	head->next = new_node;
	return 0;
}


void group_remove_list(groupList_ptr head) 
{
	groupList_ptr cur, lat;
	memset(&head->groupInfo,0,sizeof(DataBaseGroup_t));
	for(cur = head; cur != NULL; cur=lat)
	{
		lat = cur->next;
		free(cur);
	}
	return;
}

int group_findGroupInfo(groupList_ptr head,DataBaseGroup_t *groupInfo)
{
	groupList_ptr bef = NULL,cur = NULL;
	for(bef=head,cur=head->next; cur!= NULL; bef=cur, cur=cur->next)//经典用法
	{
		if(cur->groupInfo.ID == groupInfo->ID)
			break;
	}
	if(cur != NULL)
	{
		memset(groupInfo->GroupName,0,sizeof(groupInfo->GroupName));
		memcpy(groupInfo->GroupName,cur->groupInfo.GroupName,strlen(cur->groupInfo.GroupName)+1);
	}
	else
	{
		return -1;
	}

	return 0;
}


#endif