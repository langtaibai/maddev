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
int GetElem_L(LinkList L,int i,int &e){
	LinkList p;
	int j=0;
	p=L;
	while(p&&j<i){
		p=p->next;
		++j;	
	}
	while(!p||j>i){
		return ERROR;
	}
	e=p->data;
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
	printf("Input the search location:");
	scanf("%d",&i);
	if(GetElem_L(L,i,e)){
		printf("The data in the location %d is %d\n",i,e);
	}else{
		printf("Can't find the right location!\n");
	}
	printf("Output the datas:");
	TraverseList_L(L);
	printf("\n");
}
