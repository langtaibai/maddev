//当子类重写父类方法时，子类对象将无法访问父类被重写的方法，super关键字可以解决这个问题。例如可以访问父类的成员变量，成员方法以及构造方法

class Animal{
	String name="动物";
	void shout(){
		System.out.println("动物发出叫声");
	}	
}
class Dog extends Animal{
	String name="犬类";
	//重写父类的shout()方法
	void shout(){
		super.shout();//访问父类的成员方法,调用了父类被重写的方法
	}	
	void printName(){
		System.out.println("name="+super.name);
	}
}
//使用super调用父类的构造方法

/*
class Animal{
        public Animal(String name){
                System.out.println("我是一只"+name);
        }
}
class Dog extends Animal{
	public dog(){
                super("沙皮狗");//调用父类有参的构造函数
        }
}
public class SuperKeyWord{
	public static void main(String[] args){
		Dog dog=new Dog();
	}
}
//注意在实例化Dog对象时一定会调用Dog类构造方法，然后super()来调用父类构造方法
*/
