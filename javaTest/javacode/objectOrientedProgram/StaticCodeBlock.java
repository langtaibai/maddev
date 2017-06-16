//一对大括号包围起来的若干行就是代码块，用static修饰的就是静态代码块，当类被加载时，代码块就会被执行，由于类只加载一次，因此代码块只执行一次，在程序中，常常用静态代码块对类的成员变量进行初始化
class Person{
	Static String country;
	static{
		country="China";
		System.out.println("Person类中的静态代码块执行了");
	}	
}
class StaticCodeBlock{
	public static void main(String[] args){
		Person p1=new Person();
		person p2=new Person();
	}	
}

