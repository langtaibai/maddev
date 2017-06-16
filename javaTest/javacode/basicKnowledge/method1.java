public class fangfa1{
	public static void main(String[] args){
		printRectangle(4,5);
		printRectangle(2,3);
		printRectangle(7,8);
	}
		public static void printRectangle(int height,int width){
			int i,j;
			for(i=0;i<width;i++){
				for(j=0;j<height;j++){
					System.out.print("*");
				}
				System.out.print("\n");
			}
			System.out.print("\n");
		}
}
