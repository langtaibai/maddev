//接口中方法都是抽象方法，因此不能通过实例话的方式来调用接口中的方法就需要定义一个类来实现
interface Animal{//定义了Animal接口
	int ID;
	void breath();
	void run();
}
class Dog implements Animal{
	//实现breath()方法
	public void breath(){
		System.out.println("狗在叫");
	}
	public void run(){
		System.out.println("狗在跑");
	}
}
public class Interface{
	public static void main(String[] args){
		Dog dog=new dog();//创建Dog类的实例对象
		dog.breath();
		dog.run();
	}	
}
//当一个类实现接口时，如果这个类是抽象类，则实现接口最后在那个的部分方法即可，否则需要实现接口中全部的所有方法
//类可以实现多个接口，多个接口要用都好隔开：class Bird implements Run,Fly{};接口实现多个接口也是
//一个类在继承另一个类的同时还可以实现接口，此时extends关键字必须位于implements关键字之前
//class Dog extends Canidate implements Animal{}
