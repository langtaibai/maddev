//在switch语句使用break语句，作用是终止某个case并跳出switch语句；
//在循环语句时作用是跳出循环语句，执行下面代码
//但在嵌套循环时，它只能跳出内层循环，如果想使用break语句跳出外层循环则需要对外层循环添加标记
public class Break{
	public static void main(String[] args){
		int i,j;
		itcast:for(i=0;i<9;i++){
                        for(j=0;j<=i;j++){
                               if(i>4){
					break itcast;
				}
				 System.out.print("*");
                        }
                        System.out.print("\n");
                }

	}
		

}
//外层for循环加了itcast。当i>4时，使用break itcast语句跳出外层循环，因此程序只打印了4行“×”
