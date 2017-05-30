#include<Sqlist.h>
//创建线性表
int CreateList_Sq(SqList &L){
	int i;
	for(i=0;i<L.length;i++){
		scanf("%d",&L.elem[i]);
	}	
	return OK;
}
int ListDelete_Sq(SqList &L,int i,int e){//删除表中第i个元素至变量
	int *p,*q;
	if(i<1||i>L.length){
		return ERROR;
	}
	q=&L.elem[i-1];//取第i个元素地址
	e=*q;
	p=L.elem+L.length-1;//表尾元素的位置
	for(++q;q>=p;q++){
		*(q-1)=*q;
	}
	--L.length;
	return OK;

}

main(){
	int i,n,e;
	SqList L;
	InitList_Sq(L);
	printf("\n Input the length of the List L:");
	scanf("%d",&n);
	L.length=n;
	CreateList_Sq(L);
	printf("Input the delete location");
	scanf("%d",&i);
	if(ListDelete_Sq(L,i,e)){
		printf("output the datas:");
		for(i=0;i<L.length;i++){
			printf("%d",L.elem[i]);
		}
	}else{
		printf("Can't find the delete data!");
	}
}
