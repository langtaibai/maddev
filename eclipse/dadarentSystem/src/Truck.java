
public class Truck extends Car {
	private int cargoCapacity;//载货量
    public int getCargoCapacity(){
            return cargoCapacity;
    }
    public void setCargoCapacity(int cargocapacity){
            this.cargoCapacity=cargocapacity;
    }
    public Truck(String name,double rent,int cargoCapacity){//构造方法可以在实例化对象时候九尾这个对象进行赋值
            this.name=name;//this.name是访问成员变量
            this.rent=rent;
            this.cargoCapacity=cargoCapacity;
    }
   
}
