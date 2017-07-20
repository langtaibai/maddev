#include<stdio.h>
//#include<string.h>
//void (*pf)(int *,int *);
void  swap(int * p1,int *p2){
	//int i=0;
	//i=strcmp(p1,p2);
	//if(i>=0){
	//	return p1;
	//}else{
	//	return p2;
	//}
	int temp;
	temp=*p1;
	*p1=*p2;
	*p2=temp;
} 
int  main(){
	
	int i=1,j=2;
	void(*pf)(int *,int *);
	pf=&swap;
	pf(&i,&j);
	printf("%d %d",i,j);
	return 0;
}
