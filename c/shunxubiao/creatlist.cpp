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
main()
{
	int i,n;
	SqList L;
	InitList_Sq(L);
	printf("\nInput the length of the list L:");
	scanf("%d",&n);
	L.length=n;
	CreateList_Sq(L);
	printf("Output the datas:");
	for(i=0;i<L.length;i++){
		printf("%d",L.elem[i]);
	
	}
	printf("\n");
	return 0;
}
