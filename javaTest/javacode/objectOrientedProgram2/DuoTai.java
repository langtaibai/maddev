//多态的定义：指允许不同类的对象对同一消息做出响应.而重载不一样，重载是指同一个方法名（只指的是名字），有多个不同的版本
//多态存在的三个必要条件
//一、要有继承；
//二、要有重写；
//三、父类引用指向子类对象
interface Animal{ //定义一个接口
	void shout();	//定义抽象shout()方法
}
//定义Dog类实现Animal接口
class Cat implements Animal{
	public void shout(){
		System.out.println("汪汪");//重写shout方法
	}	
	public void sleep(){
		System.out.println("狗在睡觉");//定义一个新方法
	}
}
public class DuoTai{
	public static void main(String[] args){
		Animal an1=new Cat();//创建Cat对象，使用Animal类型的变量an1引用
		animalShout(an1);//调用animalShout()方法，将an1作为参数传入
	}
		//定义静态的animalShout()方法，接受一个Animal类型的参数
		public static void animalShout(Animal animal){
			animal.shout();
		}
		
}
//在多态的学习中，将子类对象当作父类类型来使用
// 如上例中Animal an1=new Cat();把Cat对象当做Animal类型来使用，注意此时不能用父类变量去调用子类中的新的定义方法，把上面例子改为
/*
	public static void animalShout(Animal animal){
                        animal.shout();
			animal.sleep();
                }
//就会报错找不到sleep()方法因为在Animal类中没有定义sleep()方法，面对这种情况，可以采用强制类型转换
	public static void animalShout(Animal animal){
		Cat cat=(Cat)animal;
		cat.shout();
		cat.sleep();
                }







*/
