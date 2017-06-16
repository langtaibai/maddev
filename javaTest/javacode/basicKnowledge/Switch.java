//switch语句是针对某个表达式的值进行判断的
public class Switch{
	public static void main(String[] args){
		int week=1;
		switch(week){
			case 1:
				System.out.println("星期一");
				break;
			case 2:
                                System.out.println("星期二");
                                break;
			case 3:
                                System.out.println("星期三");
                                break;
			case 4:
                                System.out.println("星期四");
                                break;
			case 5:
                                System.out.println("星期五");
                                break;
			case 6:
			case 7:
                                System.out.println("周末");
                                break;
			default:
                                System.out.println("输入有误，请重新输入。。。。");
                                break;
			
		}
	}	
}
