//用static修饰的方法就是静态方法，静态方法可以在不创建对象的情况下就可以调用，静态方法可以使用“类名.方法名”来使用
//静态方法只能访问static修饰的成员，因为没被static修饰的成员需要先创建对象才能访问，而静态方法在被调用时可以不创建任何对象
class Person{
	public static void sayHello(){
		System.out.println("Hello");
	}
}
public class StaticMethod{
	public static void main(String[] args){
		Person.sayHello();//调用静态方法
	}	
}
