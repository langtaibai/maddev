#include<stdio.h>
#include<stdlib.h>
#define OK 1 
#define ERROR 0
#define OVERFLOW 0
typedef struct LNode{
	int data;
	struct LNode *next;
}LNode,*LinkList;
//建立一个空链表
int InitList_L(LinkList &L){
	L=(LinkList)malloc(sizeof(LNode));
	if(!L){
		exit(OVERFLOW);
	}
	L->next=NULL;
	return OK;
}

