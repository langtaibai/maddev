//continue语句作用是终止本次循环，执行下次循环
//下面例子for循环1~100，在循环过程中，当i的值为奇数时，执行continue语句结束本次循环，进入下一次循环，当i为奇数时，sum与i相加
public class Continue{
	public static void main(String[] args){
		int sum=0;
		for(int i=1;i<=100;i++){
			if(i%2==0){
				continue;
			}
			sum+=i;
		}
		System.out.println("sum"+sum);
	}	
}
