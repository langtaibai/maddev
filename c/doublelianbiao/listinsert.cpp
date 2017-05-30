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
int ListInsert_DuL(DuLinkList &L,int i,int &e){
	DuLinkList p,s;
	int j=1;
	p=L->next;
	while(p!=L&&j<i){
		p=p->next;//查找第i个节点
		++j;	
	}
	while(p==L||j>i){
		return ERROR;
	}
	s=(DuLinkList)malloc(sizeof(DuLNode));//申请新结点
	s->data=e;//s指向新元素e
	s->prior=p->prior;//开始插入,首先是×s的前驱指针
	p->prior->next=s;
	s->next=p;//×s的后继指针
	p->prior=s;
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
	printf("Input the insert location:");
	scanf("%d",&i);
	printf("Input the insert data:");
	scanf("%d",&e);
	if(ListInsert_DuL(L,i,e)){
		printf("Output the datas:");
		TraverseList_DuL(L);
	}else{
		printf("Can't insert the data!");
	}
	printf("\n");
}
