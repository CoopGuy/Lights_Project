#include <stdint.h>
#include <inttypes.h>

#include "IRDecode.h"
#include <pigpio.h>

#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "sys/time.h"

/*
   defined in IRDecode
   included as reference  

   struct message
   {
   	uint32_t data;
	char Repeated;
   };
*/

typedef struct light 
{
	char r, g, b;
} light;

void cb( void* m )
{
	Message *msg = m;
	if ( msg->Repeated ) return;
	if ( msg->data == 0x00FFA25D )
		gpioWrite( 15, 1 );
	else 
		gpioWrite( 15, 0 );

}

void run(int handle)
{
	const uint8_t num_lights = 8;
	
	const char high = 0xc0;
	const char low  = 0xf8;
	
	light *lights = malloc(sizeof(light)*num_lights);
	char *bit_vals = malloc(24*num_lights), *color_bytes = (char*)lights;
	
	int i,j;

	for(i = 0; i < num_lights; i++)
	{
		lights[i].r = 60;
		lights[i].g = 0;
		lights[i].b = 0;
	}
	for(i = 0; i < num_lights*3; i++)
	{
		char current = color_bytes[i];
		for(j = 0; j < 8; j++)
		{
			bit_vals[8*i+j] = ((0x01 << j) & current) > 0 ? low : high;
		}
	}

	spiWrite(handle, bit_vals, 24*num_lights);
}

int main(int argc, char* argv[])
{
	if(gpioInitialise() == PI_INIT_FAILED)
	{
		perror("Bad GPIO Init");
		return 1;
	} 
	
	if ( Initialize_IR_NEC( 14, cb, 1 ) )
	{
		perror("Bad IR Init");
		return 1;
	}

	if ( gpioSetMode( 18, PI_OUTPUT ) )
	{
		perror("Bad LED Init");
		return 1;
	}

	int handle = spiOpen(0, 8000000, 0x0000);
	if( handle < 0 )
	{
		perror("Bad spi open");
		return 1;
	}

	run(handle);

	gpioTerminate();
}
