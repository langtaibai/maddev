import java.util.Scanner;
public class Test {
	public static void main(String[] args){//定义一个类数组
        Car[] car={new PassengerCar("奥迪",500,4),new PassengerCar("马自达",400,4),new Truck("金龙",800,20),new Truck("松花江",400,4),new Truck("依维柯",1000,20)};
        System.out.println("欢迎使用答答租车系统");
        System.out.println("你是否要租车:1是 2否");
        int peopleCapacity=0;
        int cargoCapacity=0;
        int totalRent=0;
        Scanner scan=new Scanner(System.in);
        int input=scan.nextInt();//这两句是读取输入值
        if(input==1){
                System.out.println("你可租车的类型及其价格表：");
                System.out.print("序号\t\t汽车名称\t\t租金\t\t容量\t\t\n");
                int i=1;
                for(Car Cars:car){
                        if(Cars instanceof PassengerCar){
                        	System.out.println(" "+i+"\t\t"+Cars.getName()+"\t\t"+Cars.getRent()+"元/天\t"+((PassengerCar)Cars).getPeopleCapacity()+"人\t\t");

                        }else{
                        	System.out.println(" "+i+"\t\t"+Cars.getName()+"\t\t"+Cars.getRent()+"元/天\t"+((Truck)Cars).getCargoCapacity()+"吨\t\t");
                        }
                        ++i;
                }
                System.out.println("请输入你要租车的数量：");
                int count=scan.nextInt();
                for(int j=0;j<count;j++){
                	System.out.println("请输入您要选择的第"+(j+1)+"辆车的序号");
                	int chooseNum=scan.nextInt();
                	System.out.println("你选择的是第"+chooseNum+"号车型");
                	if(car[chooseNum-1] instanceof PassengerCar){
                			System.out.println(car[chooseNum-1].getName()+"\t\t"+car[chooseNum-1].getRent()+"元/天\t"+((PassengerCar)car[chooseNum-1]).getPeopleCapacity()+"人\t\t");
                			peopleCapacity+=((PassengerCar)car[chooseNum-1]).getPeopleCapacity();
                	}else{
                			System.out.println(car[chooseNum-1].getName()+"\t\t"+car[chooseNum-1].getRent()+"元/天\t"+((Truck)car[chooseNum-1]).getCargoCapacity()+"人\t\t");
                			cargoCapacity+=((Truck)car[chooseNum-1]).getCargoCapacity();
                	}
                	totalRent+=car[chooseNum-1].getRent();
                }
                System.out.println("请输入你租车的天数");
                int days=scan.nextInt();
                System.out.println("你租赁了"+count+"辆车");
                System.out.println("总载客量为"+peopleCapacity+"人");
                System.out.println("总载货量为"+cargoCapacity+"吨");
                System.out.println("总租金为"+totalRent*days+"元");
          }else{
        	System.out.println("再见");  
          }
	 } 
}
