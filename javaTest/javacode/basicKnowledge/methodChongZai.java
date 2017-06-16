//方法重载就是在一个程序中定义多个名称相同的方法，但是参数的类型或个数必须不同，这就是方法的重载
public class methodChongZai{
	public static void main(String[] args){
	int sum1=add(1,3);
	int sum2=add(1,2,3);
	double sum3=add(1.2,2.3);
	System.out.println("sum1 is "+sum1);
	System.out.println("sum2 is "+sum2);
	System.out.println("sum3 is "+sum3);
	}
	public static int add(int x,int y){
		return x+y;
	}	
	public static int add(int x,int y,int z){
		return x+y+z;
	}	
	public static double add(double x,double y){
		return x+y;
	}	
}
