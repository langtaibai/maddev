//Inner类有一个show()方法，在这方法中访问外部类的成员变量num，test()方法创建了内部类Inner的实例对象，并通过该对象调用show()方法来打印num值
class Outer{
	private int num=4;
	public void test(){//定义一个成员方法，成员方法访问内部类
		Inner inner=new Inner();
		inner.show();	
	}
//定义一个内部类
	class Inner{
		void show(){
			System.out.println("num="+num);
		}
	}	
}
public class InnerClass{
	public static void main(String[] args){
		Outer outer=new Outer();
		outer.test();
	}	
}
//如果外部类去访问内部类，则需要通过外部类去创建内部类对象
/*
public class InnerClass{
	public static void main(String[] args){
		Outer.Inner inner=new Outer().new Inner();//创建内部类
		inner.test();
	}	
//结果与上面相同但是如果内部类被声明为私有，外界将无法访问，就是将内部类Inner使用private来修饰，则编译就会报错
*/

//如果是静态内部类，则可以在不创建外部类的情况下被实例化,比如
/*
class Outer{
        private int num=4;
//定义一个静态内部类
        static class Inner{
                void show(){
                        System.out.println("num="+num);
                }
        }
}
public class InnerClass{
        public static void main(String[] args){
                Outer.Inner  inner=new Outer.Inner();//创建内部类对象
                inner.test();
        }
}
//注意只有静态内部类可以定义静态成员
*/

//最后是方法内部类，它只能在当前方法中被使用
/*
class Outer{
        private int num=4;
	public void test(){
       		 class Inner{
                	void show(){
                        	System.out.println("num="+num);
                	}
        	}
		Inner inner=new Inner();
		in.show();
	}
}
public class InnerClass{
        public static void main(String[] args){
                Outer outer=new Outer();//创建内部类对象
                outer.test();
        }
}




*/


