#include<stdio.h>
#include<stdlib.h>
#define OK 1
#define ERROR 0
#define OVERFLOW 0
#define LIST_INIT_SIZE 100
#define LISTINCREMENT 10
typedef struct
{
	int *elem;
	int length;
	int listsize;
}SqList;
int InitList_Sq( SqList &L )
{
	L.elem=(int *)malloc(LIST_INIT_SIZE*sizeof(int));
	if(!L.elem){
		exit(OVERFLOW);
	}
	L.length=0;
	L.listsize=LIST_INIT_SIZE;
	return OK;
}
