import java.util.*;
public class Array{
	public static void main(String[] args){
		int[]  arr={1,3,4,2};
		System.out.println("排序前：");
		printArray(arr);//打印原数组	
		Arrays.sort(arr);//调用sort的排序方法
		System.out.println("排序后：");
		printArray(arr);//打印数组	
		int index=Arrays.binarySearch(arr,3);
		//binarySearch()方法只能对排序后的数组进行元素的查找，因为该方法是二分法查找
		System.out.println("数组排序后元素3的索引："+index);
		
		//Arrays的fill()方法可以将指定的值给数组中的每一个元素
	//	Arrays.fill(arr,8);
	//	Sngeystem.out.print("\n");

		//在不破坏原数组情况下使用数组中的部分元素，Arrars的copyOfRange()方法可以将指定范围的元素复制到新的数组，from表示
		//复制的的初始索引，to表示复制元素最后索引（不包括）
		int[] copy=Arrays.copyOfRange(arr,1,3);
		printArray(copy);
		
		//把数组以字符串的形式输出，这时就可以使用Arrays工具类的另一个方法toString(int[] arr),需要的是该方法并不是对Object类to			String()方法的重写，只是用于返回数组的字符串的形式
		String arrString=Arrays.toString(arr);
		System.out.println(arrString);
	}
	public static void printArray(int[] arr){//打印数组方法
		System.out.print("[");
		for(int i=0;i<arr.length;i++){
			if(i!=arr.length-1){
				System.out.print(arr[i]+",");
			}else{
				
				System.out.println(arr[i]+"]");
			}
		}
	}
}
