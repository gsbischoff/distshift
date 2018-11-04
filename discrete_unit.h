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

// To do: change to fixed point
typedef struct 
{
#if 1
	unsigned int Value;
	double Width;
#elif
	u64 Value;
	u64 Width;
#endif
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
} samples;
