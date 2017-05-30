#include<LinkList.h>
int CreateList_L(LinkList &L,int n){
	LinkList p;
	int i;
	printf("Input the datas:");
//	q=L;
	for(i=0;i<n;i++){
		p=(LinkList)malloc(sizeof(LNode));
		scanf("%d",&p->data);
//		p->next=q->next;
		p->next=L->next;
		L->next=p;
	}
		return OK;

}	
int TraverseList_L(LinkList L){
	LinkList p;
	p=L->next;
	while(p){
		printf("%d",p->data);
		p=p->next;
	}
	return OK;
}
main(){
	int n;
	LinkList L;
	InitList_L(L);
	printf("Input the length of the list L:");
	scanf("%d",&n);
	CreateList_L(L,n);
	printf("Output the datas:");
	TraverseList_L(L);
}
