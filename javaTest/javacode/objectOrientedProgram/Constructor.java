//构造方法是在实例化对象的同时直接赋值，构造方法需满足以下三点：方法名与类名相同；在方法名前面没有返回值类型的声明；在方法中不能使用return语句返回一个值
class Person{
	int age;
	public Person(int a){//构造一个有参数的构造方法
		age=a;
	}	
	public void speak(){
		System.out.println("I am"+age+"years old!");
	}
}
public class Constructor{
	public static void main(String[] args){
		Person person=new Person(20);//new Person(20)会在实例化对象的同时调用有参的构造方法，并传入参数20
		person.speak();
	}	
}
