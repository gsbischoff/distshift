#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>

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

void
ShiftDistribution(discrete_distribution Distribution, 
                  discrete_distribution Target)
{
	// Find area of input Dist and set widths to 1
	size_t Area = 0;
	for(int Index = 0;
	    Index < Distribution.Length;
		++Index)
	{
		Area += Distribution.Contents[Index].Value;
		Distribution.Contents[Index].Width = 1.0;
	}

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
CreateDistribution(void *Data, size_t DataSize, int BytesPerSample)
{
	discrete_distribution Result = {0};

	if(BytesPerSample == 3)
		return(Result);

	size_t NumValues = (1 << ((BytesPerSample * 8) - 1));
	Result.Length = NumValues;
	Result.Contents = calloc(sizeof(discrete_unit) * NumValues, 1);
	printf("Created distribution with %zu samples\n", NumValues);

	s8 *Byte = Data;
	s16 *Short = Data;
	for(int Index = 0;
	    Index < (DataSize / BytesPerSample);
		++Index)
	{
		switch(BytesPerSample)
		{
			case 1: 
			{
				int SubIndex = ((Byte[Index] > 0) ? Byte[Index] : -Byte[Index]);
				Result.Contents[SubIndex].Value++;
			} break;
			case 2: 
			{
				int SubIndex = ((Short[Index] > 0) ? Short[Index] : -Short[Index]);
				Result.Contents[SubIndex].Value++;
			} break;

			default: break;
		}
	}

	printf("\nFilled Result\n");

	return(Result);
}
u32
ByteSwapU32(u32 Value)
{
	u32 Result = ((Value >> 24) & (0xFF)) |
	             ((Value >> 8)  & (0xFF << 8)) |
				 ((Value << 8)  & (0xFF << 16)) |
				 ((Value << 24) & (0xFF << 24));

	return Result;
}
int
main(int ArgCount, char **Args)
{
	wav_header Header = {0};

	FILE *Input = fopen(Args[1], "rb");

	if(Input)
	{
		fread(&Header, 1, sizeof(wav_header), Input);

		printf("ChunkID: %.4s\n"
				"ChunkSize: %u\n"
				"Format: %.4s\n"
				"Subchunk1ID: %.4s\n"
				"Subchunk1Size: %u\n"
				"AudioFormat: %u\n"
				"NumChannels: %u\n"
				"SampleRate: %u\n"
				"ByteRate: %u\n"
				"BlockAlign: %u\n"
				"BitsPerSample: %u\n"
				"Subchunk2ID: %.4s\n"
				"Subchunk2Size: %u\n",
				(char *)&Header.ChunkID,
				Header.ChunkSize,
				(char *)&Header.Format,
				(char *)&Header.Subchunk1ID,
				Header.Subchunk1Size,
				Header.AudioFormat,
				Header.NumChannels,
				Header.SampleRate,
				Header.ByteRate,
				Header.BlockAlign,
				Header.BitsPerSample,
				(char *)&Header.Subchunk2ID,
				Header.Subchunk2Size);

		void *Data = malloc(Header.Subchunk2Size);

		fread(Data, 1, Header.Subchunk2Size, Input);
		fclose(Input);

		discrete_distribution Distribution = CreateDistribution(Data, Header.Subchunk2Size, Header.BitsPerSample / 8);

		// Make a normal distribution
		discrete_distribution Target;
		Target.Length = Distribution.Length;
		Target.Contents = calloc(sizeof(discrete_unit) * Distribution.Length, 1);

		


		ShiftDistribution(Distribution, Target);

		short *Samples = Data;
		free(Distribution.Contents);

	}
	else
	{
		fprintf(stderr, "Could not open file \"%s\"!\n", Args[1]);
	}
}