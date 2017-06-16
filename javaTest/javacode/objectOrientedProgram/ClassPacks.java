//类的封装就是定义一个时，将类中的属性私有化，即使用private关键字来修饰，私有属性只能在它所在类中被访问。为了能让外界访问私有属性，需提供一些public修饰的公有方法，其中包括用于获取属性值的getXxx()方法，和设置属性值的setXxx()方法
class Student{
	private String name;
	private int age;
	//下面是公有的getXxx()，setXxx()方法
	public String getName(){
		return name;
	}
	public void setName(String stuName){
		name=stuName;
	}
	public int  getAge(){
                return age;
        }
        public void setAge(int  stuAge){
                if(stuAge<=0){
			System.out.println("年龄不合法");
		}else{
			age=stuAge;
		}
        }
	public void introduce(){
		System.out.println("大家好，我叫"+name+",我今年"+age+"岁！");
	}
}
public class ClassPacks{
	public static void main(String[] args){
		Student stu=new Student();
		stu.setAge(-30);
		stu.setName("李芳");
		stu.introduce();
	}	
}
	


