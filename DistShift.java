//This program takes an input discrete distribution and a target distribution of the
//same weight and outputs a transformation array

public class DistShift
{
	public static void main(String[] args)
	{
		double[] input = {1,3,6,6,3,1};
		double[] target = {2,2,2,2,2,2};

		double[] arr = DistributionShift(input, target);
		System.out.print("{ ");
		for(double item : arr)
			System.out.print(item + " ");
		System.out.println("}");
	}
	//Area of one bar should equal the area of the target from the start to the length it's stretch out
	//Shifts from left point, for use from middle of distribution towards right edge
	public static double[] DistributionShift(double[] input, double[] target)
	{
		double inputSize = 0;
		double targetSize = 0;

		for(double j : input)
			inputSize += j;
		for(double k : target)
			targetSize += k;

		//scale input array's elements if their sums aren't equal
		if(inputSize != targetSize)
		{
			System.out.println("INPUT DIST HAS BEEN RESCALED!!");
			double mult = targetSize / inputSize;
			for(int i = 0; i < input.length; i++)
				input[i] *= mult;
		}

		double start = 0;
		double[] shifts = new double[input.length];
		for(int i = 0; i < input.length; i++)
		{
			//System.out.println("");
			//System.out.println("INDEX NO.: " + i);
			
			double areaFilled = 0;
			double width = 0;
			double position = start;
			//System.out.println("Start: " + start);
			//System.out.println("Input: " + input[i]);

			double dirtLeft = input[i];
			while(dirtLeft > (((int) position + 1) - position) * target[(int) position])
			{
				int next = (int) position + 1;

				dirtLeft -= (next - position) * target[(int) position];

				width += next - position;
				position = next;
			}

			if(dirtLeft > 0)
			{
				width += dirtLeft / target[(int) position];
				position += dirtLeft / target[(int) position];
			}

			start = position;
			shifts[i] = width;
		}
		return shifts;
	}
}