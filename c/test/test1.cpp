#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define OVERFLOW 0
#define OK 1
#define ERROR 0
#define LIST_INIT_SIZE 100 //存储空间初始分配量
typedef struct{
	char num[10];//学号
	char name[20];//名字
	double grade;//成绩
}student;
typedef int status;
typedef student ElemType ;

typedef struct{
	ElemType *elem;//存储基址
	int length;//当前表长
	int listsize;//当前分配存储容量
}SqList;
//构造一个空的顺序表
status INitList_Sq(SqList &L){
	L.elem=(ElemType *)malloc( LIST_INIT_SIZE*sizeof(ElemType));
	if(!L.elem){
		exit(OVERFLOW);//存储空间分配失败
	}
	L.length=0;
	L.listsize=LIST_INIT_SIZE;
	return OK;
}
void Input(ElemType *e){
	printf("姓名"); scanf("%s",e->name);
	printf("学号"); scanf("%s",e->num);	
	printf("成绩"); scanf("%lf",&e->grade);
	printf("请输入完成\n\n");
}
void Output(ElemType *e)
    {
        printf("姓名:%-20s\n学号:%-10s\n成绩:%-10.2lf\n\n",e->name,e->num,e->grade);
    }
//访问顺序表，找到i位置，返回给e
ElemType GetElem(SqList &L,int i){
	return L.elem[i];	
}
//根据名字查找，返回学生的编号
status Search(SqList &L,char str[]){
	for(int i=1;i<=L.length;i++){
		if(strcmp(L.elem[i].name,str)==0)
			return i;
	}
	return 0;	
}
//第i个位置插入某个学生信息
status ListInsert_Sq(SqList &L,int i,ElemType e){
	
	if((i<1)||(i>L.length+1)) return ERROR;
	if(L.length>=L.listsize)//当前空间已满，需要增加分配
	{
		return ERROR;
	}
	for(int j=L.length;j>=i;j--)
        {
            L.elem[j+1]=L.elem[j];
        }
        L.elem[i]=e;
        ++L.length;
        return OK;
}
//删除第i位置学生
status ListDelete_Sq(SqList &L,int i){
	if((i<1)||(i>L.length))   return ERROR;
        for(int j=i;j<=L.length;j++)
        {
            L.elem[j]=L.elem[j+1];
        }
	--L.length;
        return OK;
}

int main(){	
	SqList L;
	ElemType a,b,c,d;
	 printf("\n********************************\n\n");
        puts("1. 构造顺序表");
        puts("2. 录入学生信息");
        puts("3. 显示学生信息");
        puts("4. 输入姓名，查找该学生");
        puts("5. 显示某位置该学生信息");
        puts("6. 在指定位置插入学生信息");
        puts("7. 在指定位置删除学生信息");
        puts("8. 统计学生个数");
        puts("0. 退出");
        printf("\n********************************\n\n");
        int x,choose;
	while(1){
		puts("请选择");
		scanf("%d",&choose);
		if(choose==0){
			break;
		}
		switch(choose){
			case 1:
				if(INitList_Sq(L))
					printf("成功建立线性表\n\n");
				else
					printf("顺序建立线性表\n\n");
				break;
			case 2:
				printf("请输入要录入学生的人数，最大为100人");
				scanf("%d",&x);
				for(int i=1;i<=x;i++){
					printf("第%d个学生：\n",i);
					Input(&L.elem[i]);
				}
				L.length=x;
				break;
			case 3:
				for(int i=1;i<=x;i++){
					a=GetElem(L,i);
					Output(&a);
				}
				break;
			case 4:
				char s[20];
				printf("请输入要查找的学生姓名");
				scanf("%s",s);
				if(Search(L,s))
					Output(&L.elem[Search(L,s)]);
				else
					puts("对不起，查无此人");
				break;
			case 5:
				printf("请输入要查询位置");
				int id1;
				scanf("%d",&id1);
				b=GetElem(L,id1);
				Output(&b);
				break;
			case 6:
				printf("请输入插入位置");
				int id2;
				scanf("%d",&id2);
				printf("请输入学生信息：\n");
				Input(&c);
				if(ListInsert_Sq(L,id2,c)){
					x++;
					puts("插入成功");
				}else{
					puts("插入失败");
				}
			case 7:
				printf("请输入要删除的位置:");
                       		int id3;
                                scanf("%d",&id3);
                        	if(ListDelete_Sq(L,id3))
                        	{
                            		x--;
                           	        puts("删除成功");
                        	}else {
                           		 puts("删除失败");
                           		 puts("");
                        	}
                        	break;
			case 8:
				 printf("已录入的学生个数为:%d\n\n",L.length);
                	         break;
		}

	}

	printf("\n\n谢谢您的使用，请按任意键退出\n\n\n");
	return 0;
}
