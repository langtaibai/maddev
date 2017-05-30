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
//建立一个带头结点的含n个元素的双向循环链表L
int CreateList_DuL(DuLinkList &L,int n){
	DuLinkList p,q;
	int i;
	printf("Input the datas:");
	q=L;
	for(i=0;i<n;i++){
		p=(DuLinkList)malloc(sizeof(DuLNode));
		scanf("%d",&p->data);
		p->next=q->next;//新元素总是插入表尾
		q->next=p;
		p->prior=q;
		L->prior=p;//修改头结点的prior的值，指向新结点p
		q=p;//q指向新的尾结点
	}
		return OK;

}	
//遍历双向循环链表
int TraverseList_DuL(DuLinkList L){
	DuLinkList p;
	p=L->next;//p指向第一个结点
	while(p!=L){//还没到链表结尾  这地方注意如何还按照以前条件while（p）输出将是无线循环，因为这是循环链表头尾相连
		printf("%d",p->data);
		p=p->next;
	}
	return OK;
}
main(){
	int n;
	DuLinkList L;
	InitList_DuL(L);
	printf("Input the length of the list L:");
	scanf("%d",&n);
	CreateList_DuL(L,n);
	printf("Output the datas:");
	TraverseList_DuL(L);
	printf("\n");
}
