//static关键字可以修饰成员变量，成员方法以及代码块；
//static关键字来修饰成员变量，改变量称为静态变量，静态变量能够被一个类的所有实例对象共享，可以通过“类名.变量名”来访问，也可以通过类的实例对象进行调用
//注意一点就是static关键字只能修饰成员变量，不能修饰局部变量
class Student{
	static String schoolName;
}
public class StaticVariable{
	public static void main(String[] args){
		Student stu=new Student();
		Student.schoolName="ECUST";//为静态变量赋值
		System.out.println("My school is"+stu.schoolName);
	}  	
}
