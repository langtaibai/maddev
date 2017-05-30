#include<stdio.h>
#include<stdlib.h>
#define OK 1
#define ERROR 0
#define OVERFLOW 0
typedef struct DuLNode{
        int data;
        struct DuLNode *prior;
        struct DuLNode *next;
}DuLNode,*DuLinkList;
//建立一个只含头结点的空双向循环链表
int InitList_DuL(DuLinkList &L){
        L=(DuLinkList)malloc(sizeof(DuLNode));
        if(!L){
                exit(OVERFLOW);
        }
        L->prior=L;
        L->next=L;
        return OK;
}
int CreateList_DuL(DuLinkList &L,int n){
	DuLinkList p,q;
	int i;
	printf("Input the datas:");
	q=L;
	for(i=0;i<n;i++){
		p=(DuLinkList)malloc(sizeof(DuLNode));
		scanf("%d",&p->data);
		p->next=q->next;
//		p->next=L->next;
		q->next=p;
		p->prior=q;
		L->prior=p;
		q=p;
	}
		return OK;

}
int ListDelete_DuL(DuLinkList &L,int i,int &e){
	DuLinkList p;
	int j=1;
	p=L->next;
	while(p!=L&&j<i){
		p=p->next;//查找第i个节点
		++j;	
	}
	while(p==L||j>i){
		return ERROR;
	}
	e=p->data;
	p->prior->next=p->next;
	p->next->prior=p->prior;
	free(p);
	return OK;
	

}	
int TraverseList_DuL(DuLinkList L){
	DuLinkList p;
	p=L->next;
	while(p!=L){
		printf("%d",p->data);
		p=p->next;
	}
	return OK;
}
main(){
	int i,n,e;
	DuLinkList L;
	InitList_DuL(L);
	printf("Input the length of the list L:");
	scanf("%d",&n);
	CreateList_DuL(L,n);
	printf("Input the delete location:");
	scanf("%d",&i);
	if(ListDelete_DuL(L,i,e)){
		printf("Output the datas:");
		TraverseList_DuL(L);
	}else{
		printf("Can't delete the data!");
	}
	printf("\n");
}
