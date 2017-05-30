#include<LinkList.h>
int CreateList_L(LinkList &L,int n){
	LinkList p,q;
	int i;
	printf("Input the datas:");
	q=L;
	for(i=0;i<n;i++){
		p=(LinkList)malloc(sizeof(LNode));
		scanf("%d",&p->data);
		p->next=q->next;
//		p->next=L->next;
		q->next=p;
		q=p;
	}
		return OK;

}
int ListInsert_L(LinkList &L,int i,int &e){
	LinkList p,s;
	int j=0;
	p=L;
	while(p&&j<i-1){
		p=p->next;//查找第i-1个节点
		++j;	
	}
	while(!p||j>i){
		return ERROR;
	}
	s=(LinkList)malloc(sizeof(LNode));//申请新结点
	s->data=e;
	s->next=p->next;
	p->next=s;
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
	int i,n,e;
	LinkList L;
	InitList_L(L);
	printf("Input the length of the list L:");
	scanf("%d",&n);
	CreateList_L(L,n);
	printf("Input the insert location:");
	scanf("%d",&i);
	printf("Input the insert data:");
	scanf("%d",&e);
	if(ListInsert_L(L,i,e)){
		printf("Output the datas:");
		TraverseList_L(L);
	}else{
		printf("Can't insert the data!");
	}
	printf("\n");
}
