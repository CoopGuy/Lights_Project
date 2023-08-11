#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pigpio.h>

#ifndef IRDec
#define IRDec

#define bool uint16_t
#define true 1
#define false 0

typedef void (*IRMessageChangeCallback) (void*);
typedef struct Message
{
	uint32_t data;	
	bool Repeated;
} Message;
int Initialize_IR_NEC( int pin, IRMessageChangeCallback, int32_t inverted );
#endif
