
public class PassengerCar extends Car {
	private int  peopleCapacity;//载客量
    public PassengerCar(String name,double rent,int peopleCapacity){//构造方法可以在实例化对象时候直接这个对象进行赋值
           this.name=name;
           this.rent=rent;
           this.peopleCapacity=peopleCapacity;
   }

   public int getPeopleCapacity(){
           return peopleCapacity;
   }
   public void setPeopleCapacity(int  peoplecapacity){
           this.peopleCapacity=peoplecapacity;
   }
 
}
