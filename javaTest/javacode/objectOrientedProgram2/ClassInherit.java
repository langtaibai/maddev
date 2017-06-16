//类只支持单继承，一个类只能有一个父类
//多个类可以继承一个父类
//多层继承是可以的

//重写父类方法，需要注意的是在重写父类方法时，要和父类被重写的方法具有相同的方法名，参数列表，以及返回值类型；另外在重写父类方法时，不能使用比父类中被重写的方法更严格的访问权限，也就是如果父类是public，子类的方法就不能用private的

class Animal{
	void shout(){
		System.out.println("动物发出叫声");
	}	
}
class Dog extends Animal{
	void shout(){
		System.out.println("汪汪");
	}
}
public class ClassInherit{
	public static void main(String[] args){
		Dog dog=new Dog();
		dog.shout();
	}	
}
	
