#include<LinkList.h>
int CreateList_L(LinkList &L,int n){
	LinkList p,q;
	int i;
	printf("Input the datas in increasing order:");
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
int MergeList_L(LinkList &La,LinkList &Lb,LinkList &Lc){
	LinkList pa,pb,pc;
	pa=La->next;//初始化pa的初值指向表La的第一个结点
	pb=Lb->next;
	Lc=pc=La;//用La的头结点作为Lc的头结点，pc的初值指向Lc的头结点
	while(pa && pb){ //当两个表非空，依次取出两表中较小的结点插入到Lc表的最后
		if(pa->data<=pb->data){
			pc->next=pa;pc=pa;pa=pa->next;
		}else{
			pc->next=pb;pc=pb;pb=pb->next;
		}	
	
	}
	pc->next=pa?pa:pb;//插入剩余结点
	free(Lb);
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
	LinkList La,Lb,Lc;
	InitList_L(La);
	InitList_L(Lb);
	printf("Input the length of the list La:");
	scanf("%d",&n);
	CreateList_L(La,n);
	printf("Input the length of the list Lb:");
	scanf("%d",&n);
	CreateList_L(Lb,n);
	MergeList_L(La,Lb,Lc);
	printf("Output the data in Lc:");
	TraverseList_L(Lc);
	printf("\n");
}
