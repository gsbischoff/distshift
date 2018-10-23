import java.io.File;
import java.io.PrintWriter;
import java.io.FileNotFoundException;
import java.util.Scanner;
public class IntegralPlot
{
	public static int[] LoadPoints(String filename, int N) throws FileNotFoundException
	{
		double[] a = StdAudio.read(filename+".wav");

		int[] intArray = new int[a.length];
		for(int i=0; i<intArray.length; ++i)
    		intArray[i] = (int) (a[i] * ((double) Short.MAX_VALUE));

		int[] ampCount = new int[65536];

		for(int i = 0; i < intArray.length-1; ++i)
			++ampCount[(int)((intArray[i] + 65536 + intArray[i+1])/2)];

		return ampCount;
	}
	public static void DrawAxis(double ymin, double ymax, int xmin, int xmax)
	{
		// 2 ^ 16 = zoom 1
		// to get tick space divide by (65536/new range) or multiply by new range and divde 2^16
		int tickSpacing = (xmax - xmin) / 16;
		double[] x = new double[4];
		double[] y = new double[4];
		x[0] = xmin; y[0] = -1;
		x[1] = xmax; y[1] = -1;
		x[2] = xmax; y[2] = ymax;
		x[3] = xmin; y[3] = ymax;

		//int start = 32768;// = ((int)(xmin / tickSpacing)) * tickSpacing;

		StdDraw.setPenColor(StdDraw.CYAN);
		StdDraw.line((65536 / 2), 0, (65536 / 2), ymax);
		StdDraw.line(xmin, -1, xmax, -1);
		StdDraw.setPenColor(StdDraw.BLACK);
		StdDraw.polygon(x, y);
		for(int k = 0; k <= xmax; k += tickSpacing)
		{
			if(k < xmin)
				continue;
			StdDraw.text(k, (ymin/2), k+"");
			StdDraw.line(k, -1, k, (ymin/4));
		}
	}

	//Plot the count of each amplitude within the range of [xmin,xmax]
	public static void DrawPlot(int[] aCount, double ymax, int xmin, int xmax, boolean mirror, int[] color)
	{
		double zoom = 65536 / (xmax - xmin);
		int skipEntries = 1;
		if(zoom < 32)
			skipEntries = (int)(32/zoom);
		for(int i = xmin; i < xmax; i+=skipEntries)
		{
			int x = i;
			int y = aCount[i];
				
			/*
				mirror functionality - folds second half of distribution
				onto the first half in white to see assymmetry
			*/
			StdDraw.setPenColor(color[0], color[1], color[2]);
			if(i > (65536 / 2) && mirror)
			{
				 StdDraw.setPenColor(StdDraw.WHITE);
				 x = 65536 - x;
			}
			if(y==0)
				continue;
			if(y > ymax)
				y = (int)ymax;
			StdDraw.line(x, 0, x, y);
		}
	}
	public static void DrawInfo(double ymax, int c) 
	{
	    StdDraw.setPenColor(StdDraw.BLACK);
	    StdDraw.text(0 , ymax + (ymax / 32), c+"");
	}
	public static void DrawLegend(double ymin, double ymax, int xmin, int xmax, int plots, int N, int[] color, String filename)
	{
		if(N > 16)
			return;
		if(plots > 16)
			plots = 16;
		// 2 ^ 16 = zoom 1
		// to get tick space divide by (65536/new range) or multiply by new range and divde 2^16
		int tickSpacing = (xmax - xmin) / (plots + 1);
		double[] x = new double[4];
		double[] y = new double[4];
		int range = (xmax - xmin);

		//int start = 32768;// = ((int)(xmin / tickSpacing)) * tickSpacing;

		double height; 
		double shift;

		height = ymax+(ymax/32);
		shift = xmin + tickSpacing * N;

		x[0] = shift - 3 * (range / 128); y[0] = height - (ymax/64);
		x[1] = shift - (range / 128); y[1] = height - (ymax/64);
		x[2] = shift - (range / 128); y[2] = height + (ymax/64);
		x[3] = shift - 3 *(range / 128); y[3] = height + (ymax/64);

		StdDraw.setPenColor(color[0], color[1], color[2]);
		StdDraw.filledPolygon(x, y);
		StdDraw.setPenColor(StdDraw.BLACK);
		StdDraw.polygon(x, y);

		StdDraw.textLeft(shift, height, filename);
		//StdDraw.line(k, -1, k, (ymin/4));
	}
	public static String[] returnMultiInput(String prompt, Scanner scanner)
	{
		String input = "";
		System.out.print(prompt);
		if(scanner.hasNextLine() && scanner.hasNext()) 
		{
			input = scanner.nextLine();
		}
		String[] output = input.split("\\s+");
		return output;
	}
	public static int[] colorMeBlazer(int i, int N) 
	{
		int[] color = new int[3];
		//to get rainbow, pastel colors
		//Random random = new Random();

	    return color;
	}
	public static void main(String[] args) throws FileNotFoundException
	{
		Scanner scanner = new Scanner(System.in);

		String[] filename = returnMultiInput("Filename(s): ", scanner);

		System.out.print("Xmin: ");
		while(!scanner.hasNextInt()) 
		{
			System.out.print("Please enter an integer!\n");
			System.out.print("Xmin: ");
			scanner.next();
		}
		int xmin = scanner.nextInt();

		System.out.print("Xmax: ");
		while(!scanner.hasNextInt()) 
		{
			System.out.print("Please enter an integer!\n");
			System.out.print("Xmax: ");
			scanner.next();
		}
		int xmax = scanner.nextInt();

		/*System.out.print("Mirror mode: ");
		while(!scanner.hasNextBoolean()) 
		{
			System.out.print("Please enter \"True\" or \"False\"!\n");
			System.out.print("Mirror mode: ");
			scanner.next();
		}*/
		boolean mirror = false;//scanner.nextBoolean();

		System.out.print("Max Height: ");
		while(!scanner.hasNextInt()) 
		{
			System.out.print("Please enter an integer!\n");
			System.out.print("Max Height: ");
			scanner.next();
		}
		int ymax = scanner.nextInt();

		//int xmin = (65536 / 2) - (65536 / 2)/zoom;
		//int xmax = (65536 / 2) + (65536 / 2)/zoom;
		int range = xmax - xmin;
		double ymin = -(ymax/16);
		StdDraw.setCanvasSize(1800, 900);
		StdDraw.setXscale(xmin - (range / 32), xmax + (range / 32));	//Set up coordinate
		StdDraw.setYscale(ymin, ymax + (ymax / 16)); 					//system.

		DrawAxis(ymin, ymax, xmin, xmax);
		int[] color = new int[3];
		int[][] aCount = new int[filename.length][65536];

		for (int i = 0; i < filename.length ; ++i ) 
		{
			System.out.println(filename[i]);

			color[0] = (127 * ((int)(Math.random() * 2)));
			color[1] = (127 * ((int)(Math.random() * 2)));
			color[2] = (127 * ((int)(Math.random() * 2)));
			aCount[i] = LoadPoints(filename[i], 65536);

			DrawLegend(ymin, ymax, xmin, xmax, filename.length, i + 1, color, filename[i]);
			DrawPlot(aCount[i], ymax, xmin, xmax, mirror, color);
		}
	}
}