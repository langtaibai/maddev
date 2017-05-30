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

int ListInsert_Sq(SqList &L,int i, int e){
	int *newbase,*p,*q;
	if(i<1||i>L.length+1){
		return ERROR;
	}
	if(L.length>=L.listsize){
		newbase=(int *)realloc(L.elem,(L.length+LISTINCREMENT)*sizeof(int));
		if(!newbase){
		exit(OVERFLOW);
		}	
		L.elem=newbase;
		L.listsize+=LISTINCREMENT;
	}
	q=&L.elem[i-1];//insert site
	for(p=&(L.elem[L.length-1]);p>=q;--p){
	*(p+1)=*p;
	}
	*q=e;
	L.length++;
	return OK;
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
	printf("Input the insert location:");
	scanf("%d",&i);
	if(ListInsert_Sq(L,i,e)){
		printf("Output the datas:");
		for(i=0;i<L.length;i++){
			printf("%d",L.elem[i]);
		}
	}else{
	printf("Can't insert the data!");
	}
	printf("\n");
	return 0;
}
