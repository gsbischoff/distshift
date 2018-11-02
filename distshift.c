#include "distribution.c"

#ifndef INTS
#define INTS
#include <stdint.h>
typedef uint32_t u32;
typedef uint16_t u16;
typedef int16_t s16;
typedef int8_t s8;
#endif

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

    return(Result);
}

void
MapAllWithDither(samples Samples, transfer_function TransferFunction)
{
    // Todo: dither [TPDF, +/- 0.5 LSB]
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

samples
WaveRead(char *Filename)
{
    if(!Filename)
        return((samples) {0});

    FILE *Input = fopen(Filename, "rb");

    if(!Input)
        return((samples) {0});

    wav_header Header = {0};

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
    fread(Data, 1, Header.Subchunk2Size, Input);
    fclose(Input);

    samples Samples = {0};
    Samples.BytesPerSample = (Header.BitsPerSample / 8);
    Samples.Length = (Header.Subchunk2Size / Samples.BytesPerSample);
    Samples.Data = Data;

    return(Samples);
}

int
main(int ArgCount, char **Args)
{
    if(ArgCount < 2)
        return(0);

    samples Samples = WaveRead(Args[1]);
    samples TargetSamples = WaveRead(Args[2]);

    discrete_distribution Distribution = CreateDistribution(Samples);
    size_t DistributionArea = GetDistributionArea(Distribution);

    // Make a target distribution -- default is normal
    discrete_distribution TargetDistribution;

    if(TargetSamples.Data)
        TargetDistribution = CreateDistribution(TargetSamples);
    else
        TargetDistribution = CreateNormalDistribution(Distribution.Length, DistributionArea);

    // Shift!
    ShiftDistribution(Distribution, TargetDistribution);

    // Integrate!
    transfer_function TransferFunction = CreateTransferFunction(Distribution);

    // Apply the function
    MapAllWithDither(Samples, TransferFunction);

    free(Distribution.Contents);
    free(TargetDistribution.Contents);

    free(Samples.Data);
    free(TargetSamples.Data);
}