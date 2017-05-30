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
int LocateElem_L(LinkList L,int e){
	LinkList p;
	int j=0;
	p=L;
	while(p&&p->data!=e){
		p=p->next;
		++j;	
	}
	if(p){
		return j;
	}else{
		return OK;
	}

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
	printf("Input the search number:");
	scanf("%d",&e);
	i=LocateElem_L(L,e);
	if(i){
		printf("The search data is in the %dth location in the L\n",i);
	}else{
		printf("There is no search data in the L!\n");
	}
	printf("Output the datas:");
	TraverseList_L(L);
	printf("\n");
}
