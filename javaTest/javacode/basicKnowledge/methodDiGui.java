//方法的递归就是一个方法的内部调用自身的过程，递归必须要有结束条件，否则就会陷入无限递归的状态，永远无法结束递归调用
public class methodDiGui{
	public static void main(String[] args){
		int sum=getSum(4);
		System.out.println("sum is "+sum);
	}	
	public static int getSum(int n){
		if(n==1){//判断条件
			return 1;
		}else{
			int temp=getSum(n-1);
			return temp+n;
		}
	}
}
