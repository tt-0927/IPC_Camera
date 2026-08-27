#ifndef DList_H
#define DList_H

#ifdef __cplusplus
extern "C" {
#endif

typedef  void* Item;
typedef struct Node * PNode;
typedef PNode Position;
/*定义节点类型*/
typedef struct Node
{
	Item data;		/*数据域*/
	PNode previous; /*指向前驱*/
	PNode next;		/*指向后继*/
}Node;
/*定义链表类型*/
typedef struct
{
	PNode head;		/*指向头节点*/
	PNode tail;		/*指向尾节点*/
	int size;
}DList;

/*分配值为i的节点，并返回节点地址*/
Position MakeNode(Item i);

/*释放p所指的节点*/
void FreeNode(PNode p);

/*构造一个空的双向链表*/
DList* InitList();

/*摧毁一个双向链表*/
void DestroyList(DList *plist);

/*将一个链表置为空表，释放原链表节点空间*/
void ClearList(DList *plist);

/*返回头节点地址*/
Position GetHead(DList *plist);

/*返回尾节点地址*/
Position GetTail(DList *plist);

/*返回链表大小*/
int GetSize(DList *plist);

/*返回p的直接后继位置*/
Position GetNext(Position p);

/*返回p的直接前驱位置*/
Position GetPrevious(Position p);

/*将pnode所指节点插入第一个节点之前*/
PNode InsFirst(DList *plist,PNode pnode);

/*将链表第一个节点删除并返回其地址*/
PNode DelFirst(DList *plist);

/*获得节点的数据项*/
Item GetItem(Position p);

/*设置节点的数据项*/
void SetItem(Position p,Item i);

/*删除链表中的尾节点并返回其地址，改变链表的尾指针指向新的尾节点*/
PNode Remove(DList *plist);

/*删除链表中的节点，改变前后节点指针的指向*/
void DelNode(DList *plist,PNode s);

/*在链表中p位置之前插入新节点S*/
PNode InsBefore(DList *plist,Position p,PNode s);

/*在链表中p位置之后插入新节点s*/
PNode InsAfter(DList *plist,Position p,PNode s);

/*返回在链表中第i个节点的位置*/
PNode LocatePos(DList *plist,int i);

/*依次对链表中每个元素调用函数visit()*/
void ListTraverse(DList *plist,void (*visit)());

#ifdef __cplusplus
}
#endif

#endif


