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
//若表中存在第i个结点删除并用e带回其值
int ListDelete_L(LinkList &L,int i,int &e){
	LinkList p,q;
	int j=0;
	p=L;
	while(p->next&&j<i-1){//查找第i个结点，若存在，p指向其直接前驱
		p=p->next;
		++j;	
	}
	while(!(p->next)||j>i-1){
		return ERROR;
	}
	q=p->next;//q指向第i个结点
	p->next=q->next;
	e=q->data;
	free(q);
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
	printf("Input the delete  location:");
	scanf("%d",&i);
	if(ListDelete_L(L,i,e)){
		printf("Output the datas:");
		TraverseList_L(L);
	}else{
		printf("Can't find the delete data!\n");
	}
	printf("\n");
}
