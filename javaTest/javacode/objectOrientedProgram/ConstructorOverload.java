//构造方法的重载像普通方法一样，只要每个构造方法的参数模型或参数个数不同即可，在创建对象是，可以通过调用不同的构造方法为不同的属性赋值
//注意以下两点：在一个类中如果定义了有参的构造方法，最好在定义一个无参的构造方法；private来构造方法时，不可以为外界调用，也就无法在类的外部创建该类的实例对象，因此为了实例化对象，构造方法常常使用public
class Person{
	String name;
	int age;
	//定义无参构造方法
	public Person(){
		this("芬芳"); //构造方法使用this只能调用其他的构造方法，不能在成员中使用，另外this调用只能放在第一行，且不能在一个类的两个构造方法使用this互相调用。
	}
	//定义两个有参的构造方法
	public Person(String name){
		this.name=name;//与下面相比this关键字用于在方法中访问对象的其他成员，在方法中说那个“age”是访问局部变量，this.age是访问成员变量。
	}
	public Person(String name,int age){
		this.name=name;
		this.age=age;
	}
	public void openMouth(){
		System.out.println("Open you mouth and say:");
	}
	public void speak(){
		this.openMouth(); //this关键字调用成员方法，注意这时直接用openMouth()也行
		System.out.println("我叫"+name+",我今年"+age+"岁！");
	}
	
}
public class ConstructorOverload{
	public static void main(String[] args){
		Person p1=new Person();
		Person p2=new Person("芬芳");
		Person p3=new Person("芬芳",5);
		p1.speak();
		p2.speak();
		p3.speak();
	} 	
	
}
