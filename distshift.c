#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>

#include <assert.h>

#include "discrete_unit.h"
#ifndef INTS
#define INTS
#include <stdint.h>
typedef uint32_t u32;
typedef uint16_t u16;
typedef int16_t s16;
typedef int8_t s8;
#endif

#define PI 3.141592653589793238462643383279


typedef struct
{
	u32 ChunkID;
	u32 ChunkSize;
	u32 Format;

	u32 Subchunk1ID;
	u32 Subchunk1Size;
	u16 AudioFormat;
	u16 NumChannels;
	u32 SampleRate;
	u32 ByteRate;
	u16 BlockAlign;
	u16 BitsPerSample;

	//u32 ExtraParamSize;
	//u32 ExtraParams;

	u32 Subchunk2ID;
	u32 Subchunk2Size;
} wav_header;

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

// Stores a mapping from [0, 2^7 - 1] or [0, 2^15 - 1] to new values in that range
typedef struct
{
	int BytesPerSample;

	union
	{
		s8 Map8[1 << 7];
		s16 Map16[1 << 15];
		// s32 Map32[1 << 31]; uh not supported
	};
} transfer_function;

transfer_function
CreateTransferFunction(discrete_distribution Distribution)
{
	//sizeof(transfer_function);
	transfer_function Result = {0};

	// MapX[x] ::= f(x) ->
	double AccumulatedOffset = 0.0;
	switch(Distribution.Length)
	{
		case (1 << 7): 
		{
			Result.BytesPerSample = 1;
			for(int Index = 0;
			    Index < (1 << 7);
				++Index)
			{
				Result.Map8[Index] = Distribution.Contents[Index].Width;
				assert(AccumulatedOffset < Distribution.Length);
			}
		} break;
		case (1 << 15): 
		{
			Result.BytesPerSample = 2;
			for(int Index = 0;
			    Index < (1 << 15);
				++Index)
			{
				Result.Map16[Index] = Distribution.Contents[Index].Width;
				assert(AccumulatedOffset < Distribution.Length);
			}
		} break;
		default: break;
	}
}

void
MapAll(samples Samples, transfer_function TransferFunction)
{
	if(Samples.BytesPerSample != TransferFunction.BytesPerSample)
		return;

	for(size_t Index = 0;
	    Index < Samples.Length;
		++Index)
	{
		switch(Samples.BytesPerSample)
		{
			case 1: 
			{
				s8 Sample = Samples.Data8[Index];
				if(Sample < 0)
				{
					Samples.Data8[Index] = -TransferFunction.Map8[-Sample];
				}
				else
				{
					Samples.Data8[Index] = TransferFunction.Map8[Sample];
				}
			} break;
			case 2: 
			{
				s16 Sample = Samples.Data16[Index];
				if(Sample < 0)
				{
					Samples.Data16[Index] = -TransferFunction.Map16[-Sample];
				}
				else
				{
					Samples.Data16[Index] = TransferFunction.Map16[Sample];
				}
			} break;
			default: break;
		}
	}
}

int
main(int ArgCount, char **Args)
{
	wav_header Header = {0};

	FILE *Input = fopen(Args[1], "rb");

	if(Input)
	{
		fread(&Header, 1, sizeof(wav_header), Input);

		printf("ChunkSize: %u\n"
				"Subchunk1Size: %u\n"
				"AudioFormat: %u\n"
				"BitsPerSample: %u\n"
				"Subchunk2Size: %u\n",
				Header.ChunkSize,
				Header.Subchunk1Size,
				Header.AudioFormat,
				Header.BitsPerSample,
				Header.Subchunk2Size);

		void *Data = malloc(Header.Subchunk2Size);

		samples Samples = {0};
		Samples.BytesPerSample = (Header.BitsPerSample / 8);
		Samples.Length = (Header.Subchunk2Size / Samples.BytesPerSample);

		switch(Samples.BytesPerSample)
		{
			case 1: { Samples.Data8 = Data; } break;
			case 2: { Samples.Data16 = Data; } break;
			case 4: { Samples.Data32 = Data; } break;

			default: break;
		}

		fread(Data, 1, Header.Subchunk2Size, Input);
		fclose(Input);

		discrete_distribution Distribution = CreateDistribution(Samples);
		size_t DistributionArea = GetDistributionArea(Distribution);

		// Make a normal distribution
		discrete_distribution Target = CreateNormalDistribution(Distribution.Length, DistributionArea);

		// Shift!
		ShiftDistribution(Distribution, Target);

		// Integrate!
		transfer_function TransferFunction = CreateTransferFunction(Distribution);

		// Apply the function
		MapAll(Samples, TransferFunction);


		free(Distribution.Contents);

	}
	else
	{
		fprintf(stderr, "Could not open file \"%s\"!\n", Args[1]);
	}
}