class Person(){
	//当一个对象在内存中被释放时，它的finalize()方法会被自动调用，一次可以在类中定义fianlize()方法来观察对象何时被释放
	public void finalize(){
		System.out.println("对象被作为垃圾回收。。。。。。。。");
	}	
	public class GarbageCollection(){
		public static void main(String[] args){
			Person p1=new Person();
			Person p2=new Person();
			//将变量置为空，让对象成为垃圾
			p1=null;
			p2=null;
			//调用方法进行垃圾回收
			System.gc();//这时候会调用finalize（）方法
			for(int i=0;i<1000000;i++){
				//延长程序运行时间
			}
		}
	}
}
