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

typedef struct
{
    float a;
    float b;
} interval;

// Stores a mapping from [0, 2^7 - 1] or [0, 2^15 - 1] to new values in that range
// Used to map to s8's and s16's, but the operation _actually_ maps into intervals
//  is a big honking struct, thankfully we only need to make one of them
typedef struct
{
	int BytesPerSample;

	union
	{
		interval Map8[1 << 7];
		interval Map16[1 << 15];
		// s32 Map32[1 << 31]; uh not supported
	};
} transfer_function;

transfer_function *
CreateTransferFunction(discrete_distribution Distribution)
{
	//sizeof(transfer_function);
	transfer_function *Result = calloc(1, sizeof(transfer_function));

	// Map[x] ::= f(x) -> [Lo, Hi] -- maps a single scalar onto an interval (returns an interval)
	double AccumulatedOffset = 0.0;
	switch(Distribution.Length)
	{
		case (1 << 7): 
		{
			Result->BytesPerSample = 1;
			for(int Index = 0;
			    Index < (1 << 7);
				++Index)
			{
                Result->Map8[Index].a = AccumulatedOffset;
                Result->Map8[Index].b = AccumulatedOffset + Distribution.Contents[Index].Width;

                AccumulatedOffset += Distribution.Contents[Index].Width;

				assert(AccumulatedOffset < Distribution.Length);
			}
		} break;
		case (1 << 15): 
		{
			Result->BytesPerSample = 2;
			for(int Index = 0;
			    Index < (1 << 15);
				++Index)
			{
				Result->Map16[Index].a = AccumulatedOffset;
                Result->Map16[Index].b = AccumulatedOffset + Distribution.Contents[Index].Width;

                AccumulatedOffset += Distribution.Contents[Index].Width;

				assert(AccumulatedOffset < Distribution.Length);
			}
		} break;
		default: break;
	}

    return(Result);
}

void
MapAllWithDither(samples Samples, transfer_function *TransferFunction)
{
    // Todo: dither [TPDF, +/- 0.5 LSB]
    //  With dither: Map[X] -> [a,b] -> UniformRandomValue(a,b) -> f32 -> TPDFDither(f32)
	if(Samples.BytesPerSample > 3)
		return;

    // Map[X]
    srand(1);
	for(size_t Index = 0;
	    Index < Samples.Length;
	    ++Index)
	{
        interval Value;
		switch(Samples.BytesPerSample)
		{
			case 1: 
			{
				s8 Sample = Samples.Data8[Index];
				if(Sample < 0)
				{
					Value = TransferFunction->Map8[-Sample];
                    Value.a = -Value.a;
                    Value.b = -Value.b;
				}
				else
				{
					Value = TransferFunction->Map8[Sample];
				}
			} break;
			case 2: 
			{
                s16 Sample = Samples.Data16[Index];
				if(Sample < 0)
				{
					Value = TransferFunction->Map16[-Sample];
                    Value.a = -Value.a;
                    Value.b = -Value.b;
				}
				else
				{
					Value = TransferFunction->Map16[Sample];
				}
			} break;
			default: break;
		}

        // Uniformly choose a value on that interval
        float RandomValue = rand();
        float RandomValueOnInterval = ((RandomValue / (float)RAND_MAX) * (Value.b - Value.a)) + Value.a;

        // Dither w/ TPDF noise of 2 LSB peak-to-peak [Vanderkooy, Lipshitz]
        // Triangular on [0, 65 536], mean at 32 767
        float TPDF = (((float)rand() / (float)RAND_MAX) +
                        ((float)rand() / (float)RAND_MAX));

        // Now, scale to [-1, 1], or 2 LSB peak-to-peak
        float Dither = ((TPDF - 32768.f) / 32768.f);

        // Add the dither, requantize
        float DitheredSample = (RandomValueOnInterval + Dither);

        switch(Samples.BytesPerSample)
		{
			case 1: { Samples.Data8[Index]  = (s8) DitheredSample; } break;
            case 2: { Samples.Data16[Index] = (s16) DitheredSample; } break;
            case 3: { Samples.Data24[Index].LU0 = (s16) DitheredSample; } break;
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
    transfer_function *TransferFunction = CreateTransferFunction(Distribution);

    // Apply the function
    MapAllWithDither(Samples, TransferFunction);

    free(Distribution.Contents);
    free(TargetDistribution.Contents);

    free(Samples.Data);
    free(TargetSamples.Data);

    free(TransferFunction);
}