#ifndef INTS
#define INTS
#include <stdint.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t s32;
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

// To do: change to fixed point
typedef struct 
{
	double Value;
	double Width;
} discrete_unit;

typedef struct
{
	size_t Length;
	discrete_unit *Contents;
} discrete_distribution;

typedef union
{
    struct
    {
        u8 U0;
        u8 U1;
        u8 U2;
    };
    s16 LU0;
} s24;

typedef struct
{
	int BytesPerSample;
	size_t Length;

	union
	{
        void *Data;
        
		s8  *Data8;
		s16 *Data16;
        s24 *Data24;
		s32 *Data32;
	};

    wav_header Header;
} samples;
