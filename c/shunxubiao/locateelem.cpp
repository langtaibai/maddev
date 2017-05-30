#include<Sqlist.h>
int CreateList_Sq(SqList &L)
{
	int i;
	printf("Input the datas:");
	for(i=0;i<L.length;i++){
		scanf("%d",&L.elem[i]);
		return OK;
	}
		
}

int LocateElem_Sq(SqList L,int e,int (* compare)(int,int)){
	int i,*q;//表中第一个与e相等的位序号i
	i=1;
	p=L.elem;
	while(i<=L.length&&!(*compare(++p, e)){
		++i;	
	}
	if(i<=L.length){
		return i;
	}else{
		return 0;
	}
}
int compare(int a,int b){
	if(a==b)
		return 1;
	else
		return 0;

}
main()
{
	int i,n,e;
	SqList L;
	InitList_Sq(L);
	printf("\nInput the length of the list L:");
	scanf("%d",&n);
	L.length=n;
	CreateList_Sq(L);
	printf("Input the insert number:");
	scanf("%d",&e);
	i=LocateElem(L,e,compare);
	if(i){
		printf("%d",i);
	}else{
	printf("There is no search data in the L!\n");
	}
	printf("\n");
	return 0;
}
