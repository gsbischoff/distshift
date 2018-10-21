#ifndef INTS
#define INTS
#include <stdint.h>

typedef uint32_t u32;
typedef uint16_t u16;
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
