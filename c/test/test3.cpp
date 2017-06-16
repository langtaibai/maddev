#include<stdio.h>  
//    #include<malloc.h>  
    #include<stdlib.h>  
    #include<string.h>  
    #define OK 1  
    #define ERROR 0  
    #define OVERFLOW 0  
    #define MAXSIZE 100  
      
    typedef int Status; // 定义函数返回值类型  
      
    typedef struct  
    {  
        char num[10]; // 学号  
        char name[20]; // 姓名  
        double grade; // 成绩   
    }student;  
      
    typedef student  ElemType;  
      
    typedef struct  
    {  
        ElemType *elem; // 存储空间的基地址,elem为student类型指针
        int length; // 当前长度   
    }SqList;  
      
    Status InitList(SqList &L) // 构造空的顺序表 L   
    {     
        L.elem=(ElemType *)malloc(sizeof(ElemType)*MAXSIZE);  
        if(!L.elem)  exit(OVERFLOW);  
        L.length=0;  
        return OK;  
    }  
      
    ElemType GetElem(SqList &L,int i) // 访问顺序表，找到 i位置，返回给 e  
    {  
        return L.elem[i];  
    }  
      
    int Search(SqList &L,char str[]) // 根据名字查找，返回该同学在顺序表中的编号   
    {  
        for(int i=1;i<=L.length;i++)  
        {  
            if(strcmp(L.elem[i].name,str)==0)  
                return i;  
        }  
        return 0;  
    }  
      
    Status ListInsert(SqList &L,int i,ElemType e) // 在 i位置插入某个学生的信息   
    {  
        if((i<1)||(i>L.length+1)) return ERROR;  
        if(L.length==MAXSIZE)   return ERROR;  
        for(int j=L.length;j>=i;j--)  
        {  
            L.elem[j+1]=L.elem[j];  
        }  
        L.elem[i]=e;  
        ++L.length;  
        return OK;  
    }  
      
    Status ListDelete(SqList &L,int i) // 在顺序表中删除 i位置的学生信息   
    {  
        if((i<1)||(i>L.length))   return ERROR;  
        for(int j=i;j<=L.length;j++)  
        {  
            L.elem[j]=L.elem[j+1];  
        }  

        return OK;  
    }  
      
    void Input(ElemType *e)  
    {  
        printf("姓名:");  scanf("%s",e->name);  
        printf("学号:");  scanf("%s",e->num);  
        printf("成绩:");  scanf("%lf",&e->grade);  
        printf("输入完成\n\n");  
    }  
      
    void Output(ElemType *e)  
    {  
        printf("姓名:%-20s\n学号:%-10s\n成绩:%-10.2lf\n\n",e->name,e->num,e->grade);  
    }  
      
    int main()  
    {  
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
        while(1)  
        {  
            puts("请选择:");  
            scanf("%d",&choose);  
            if(choose==0)   break;  
            switch(choose)  
            {  
                case 1:  
                        if(InitList(L))  
                            printf("成功建立顺序表\n\n");  
                        else  
                            printf("顺序表建立失败\n\n");  
                        break;  
                case 2:  
                        printf("请输入要录入学生的人数（小于100）:");  
                        scanf("%d",&x);  
                        for(int i=1;i<=x;i++)  
                        {  
                            printf("第%d个学生:\n",i);  
                            Input(&L.elem[i]);  
                        }  
                        L.length=x;  
//                        puts("");  
                        break;  
                case 3:  
                        for(int i=1;i<=x;i++)  
                        {  
                            a=GetElem(L,i);  
                            Output(&a);  
                        }  
                        break;  
                case 4:  
                        char s[20];  
                        printf("请输入要查找的学生姓名:");  
                        scanf("%s",s);  
                        if(Search(L,s))  
                            Output(&L.elem[Search(L,s)]);  
                        else  
                            puts("对不起，查无此人");  
                        puts("");  
                        break;  
                case 5:  
                        printf("请输入要查询的位置:");  
                        int id1;  
                        scanf("%d",&id1);  
                        b=GetElem(L,id1);  
                        Output(&b);  
                        break;  
                case 6:  
                        printf ("请输入要插入的位置:");  
                        int id2;  
                        scanf("%d",&id2);  
                        printf("请输入学生信息:\n");  
                        Input(&c);  
                        if(ListInsert(L,id2,c))  
                        {  
                            x++;  
                            puts("插入成功");  
                            puts("");  
                        }  
                        else  
                        {  
                            puts("插入失败");  
                            puts("");     
                        }  
                        break;  
                case 7:  
                        printf("请输入要删除的位置:");  
                        int id3;  
                        scanf("%d",&id3);  
                        if(ListDelete(L,id3))  
                        {  
                            x--;  
                            puts("删除成功");  
                            puts("");  
                        }  
                        else  
                        {  
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
        system("pause");  
        return 0;  
    }  
