#include <stdio.h>  
#include <stdlib.h>  
//定义结二叉树的构体  
typedef struct BTree                  
{  
    char  data;  
    struct BTree *lChild;  
    struct BTree *rChild;  
}BinTree;  
  
  
//二叉树的创建  
 BinTree* CreateTree(BinTree *T)  
{  
    char temp;  
   scanf(" %c", &temp); //注意这里前面为什么加一个空格，因为scanf输入形式是实际上输入的是:某个字符+Enter,Enter 产生的\n 也会停留在输入缓冲区中,下次调用 scanf %c 时就会直接读到它而不是等待你再次输入! 所以加入空格是为了消除上次输入留下的'/n'
	//getchar();  //这句也是可以消除上面的问题,当上面不加空格时，加上这句也可以,因为getchar是从输入缓冲区读取一个字符，所以读取是scanf输入留下的换行符‘/n’
    if(temp == '@'){
		T=NULL;
	}else{    
    		 T = (BinTree *)malloc(sizeof(BinTree));  
   		 T->data = temp;
		 printf("输入%c的左孩子:",temp);                        
   		 T->lChild = CreateTree(T->lChild);//递归创建左子数
		 printf("输入%c的右孩子:",temp);                    
   		 T->rChild = CreateTree(T->rChild); 
	     }                   
    return T;  
}  
  
//先序遍历二叉树  
void PreOrderTraverse(BinTree *T)  
{  
    if(T)  
    {  
        printf("%c", T->data);  
        PreOrderTraverse(T->lChild);  
        PreOrderTraverse(T->rChild);  
    }  
}  
  
  
//中序遍历二叉树  
void InOrderTraverse(BinTree *T)  
{  
    if(T)  
    {  
        InOrderTraverse(T->lChild);  
        printf("%c", T->data);  
        InOrderTraverse(T->rChild);  
    }  
}  
  
  
//后序遍历二叉树  
void PostOrderTraverse(BinTree *T)  
{  
    if(T)  
    {  
        PostOrderTraverse(T->lChild);  
        PostOrderTraverse(T->rChild );  
        printf("%c",T->data );  
   }  
}  
int main()  
{  
    BinTree *Tree;  
    printf("输入root:");
    Tree = CreateTree(Tree);  
    printf("=========分隔符============\n\n");  
    printf("二叉树的先序遍历：\n");  
    PreOrderTraverse(Tree);  
    printf("\n");  
    printf("二叉树的中序遍历：\n");  
    InOrderTraverse(Tree);  
    printf("\n");  
    printf("二叉树的后序遍历：\n");  
    PostOrderTraverse(Tree);  
    printf("\n");  
    printf("\n=========================\n");  
    return 0;  
}  
