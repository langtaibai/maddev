//带参数返回
public class method2{
	public static void main(String[] args){
		int area=getArea(4,5);
		System.out.println("The area is "+area);
	}	
	public static int getArea(int x,int y){
		return x*y;
	}
}
