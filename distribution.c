#include "common_headers.h"
#include "discrete_unit.h"
#define PI 3.141592653589793238462643383279

inline double
PDF(double X)
{
	double InvSqrt2Pi = 1 / sqrt(2 * PI);
	double Result = InvSqrt2Pi * exp((-0.5) * X * X);

	return(Result);
}

// Distributes the value (stretches it) from being a 1-by-(Value) block
//  to being a (NewWidth)-by-(NewValue) block with the same area, while
//  overlapping an interval of the target distribution with the same area.
double
DistributeValueOnInterval(double LastBlockStart,
                          double Value,
						  discrete_distribution Target)
{
	// Get the index of the block our startpoint is overlapping, i.e. round down
	size_t Index = (size_t) LastBlockStart;

	// Get its value
	size_t TargetBlockValue = Target.Contents[Index++].Value;

	// Get the portion of that value that remains between our blockstart and the next block
	double RemainingWidth = (Index - LastBlockStart);
	double Area = TargetBlockValue * RemainingWidth;

	// While we can still add the next index's area, to our spread-sum, get it and do so
	while(Area + Target.Contents[Index].Value < Value)
	{
		// do the same things as above -- but we can add a full-width (1.0) of a block now
		TargetBlockValue = Target.Contents[Index++].Value;
		
		Area += TargetBlockValue;

		if(Index > Target.Length)
			return(INFINITY);
	}

	double ExtraWidth = 0.0;
	// If we have less than the full amount of the next block to add, find the remaining width
	if(Area < Value)
	{
		double RemainingValue = (Value - Area); // Need to find the amount of next block to advance to get this area
		ExtraWidth = RemainingValue / Target.Contents[Index].Value;
	}

	return(Index + ExtraWidth);
}

size_t
GetDistributionArea(discrete_distribution Distribution)
{
	// Find area of input Dist
	size_t Result = 0;
	for(int Index = 0;
	    Index < Distribution.Length;
		++Index)
	{
		discrete_unit Block = Distribution.Contents[Index];
		Result += (Block.Value * Block.Width);
	}

	return(Result);
}

void
ShiftDistribution(discrete_distribution Distribution, 
                  discrete_distribution Target)
{
	// Find area of input Dist and set widths to 1
	size_t Area = GetDistributionArea(Distribution);
	size_t TArea = GetDistributionArea(Target);
	printf("Area of Dist:   %zu\nArea of Target: %zu\n", Area, TArea);

	// These are offsets in the distribution where the current block -- rectagular
	// unit of the discrete distribution -- begins and the points from which on the 
	// X axis we will sum up area in the target distribution. Last allows us to see
	// how far the current blok has been spread out.
	double CurrentBlockStart = 0.0;
	double LastBlockStart = CurrentBlockStart;

	// Shift!
	for(int Index = 0;
	    Index < Distribution.Length;
		++Index)
	{
		LastBlockStart = CurrentBlockStart;
		size_t Value = Distribution.Contents[Index].Value;
		CurrentBlockStart = DistributeValueOnInterval(
			LastBlockStart, Distribution.Contents[Index].Value, Target);

		if(CurrentBlockStart == INFINITY) return;

		double WidthCovered = (CurrentBlockStart - LastBlockStart);

		// The width and value have hanged so that the area is still the same
		// WidthCovered * NewValue = Value; NewValue = Value / WidthCovered;
		Distribution.Contents[Index].Value = (Value / WidthCovered);
		Distribution.Contents[Index].Width = WidthCovered;	// Changed from 1.0 to this new value
	}
}

discrete_distribution
CreateNormalDistribution(size_t Length, size_t DistributionArea)
{
	discrete_distribution Result = {0};
	Result.Length = Length;
	Result.Contents = calloc(sizeof(discrete_unit) * Length, 1);

	// PDF from 0 to inf is area 0.5
	// So values of PD must be 2 * DistributionArea times higher

	// 2 * VertScale * PDF( Length / Stretch) = 0.5 
	double Stretch = 10000.0;
	double VerticalScale = (DistributionArea / Stretch);
	for(int Index = 0;
	    Index < Result.Length;
		++Index)
	{
		double X = ((double)Index / Stretch);
		Result.Contents[Index].Value = 2 * VerticalScale * PDF(X);
		Result.Contents[Index].Width = 1.0;
	}
	printf("CreateNormalDistribution: PDF at endpoint is %u\n", Result.Contents[Length].Value);

	return(Result);
}

discrete_distribution
CreateDistribution(samples Samples)
{
	discrete_distribution Result = {0};

	if(Samples.BytesPerSample == 3)
		return(Result);

	int BitsPerSample = (Samples.BytesPerSample * 8);
	size_t NumValues = (1 << (BitsPerSample - 1));
	Result.Length = NumValues;
	Result.Contents = calloc(sizeof(discrete_unit) * NumValues, 1);
	printf("Created distribution with %zu samples\n", NumValues);

	for(int Index = 0;
	    Index < Samples.Length;
		++Index)
	{
		switch(Samples.BytesPerSample)
		{
			case 1: 
			{
				int SubIndex = ((Samples.Data8[Index] > 0) ? Samples.Data8[Index] : -Samples.Data8[Index]);
				Result.Contents[SubIndex].Value++;
				Result.Contents[SubIndex].Width = 1.0;
			} break;
			case 2: 
			{
				int SubIndex = ((Samples.Data16[Index] > 0) ? Samples.Data16[Index] : -Samples.Data16[Index]);
				Result.Contents[SubIndex].Value++;
				Result.Contents[SubIndex].Width = 1.0;
			} break;

			default: break;
		}
	}

	printf("\nFilled Result\n");

	return(Result);
}