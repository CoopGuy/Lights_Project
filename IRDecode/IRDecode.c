#include "IRDecode.h"

#define uValInRange(val, low, high) (val <= high && val >= low)
#define uValIsEqual_Err(val, target, err) uValInRange(val, target - err, target + err)	
#define MiliToMicro(ms) (1000 * ms)

// following numbers in microseconds
#define ALLOWED_ERROR 100
#define LEADER_LENGTH 9000
#define STANDARD_BUF 4500
#define REPEAT_BUF 2250
#define STD_BIT_LEAD 562
#define LOW_LENGTH 562
#define HIGH_LENGTH 1687
#define MAX_WIDTH_REPEAT_CYCLE 96187

#define DATA_WORD_SIZE 32
#define DATA_WORD_TYPE uint32_t

typedef struct DataPack
{
	IRMessageChangeCallback Callback;
	uint32_t LastTick;

	bool invert;
	bool ReceivedLeader;
	bool ReceivedBuffer;
	bool PullReady;

	uint16_t BufferType;
	DATA_WORD_TYPE Data;
	uint32_t DataIndex;

	bool MessageComplete;
} DataPack;

static void ResetDP( DataPack *DP )
{
	DP->ReceivedLeader = 0;
	DP->ReceivedBuffer = 0;
	DP->PullReady = 0;

	DP->BufferType = 0;
	DP->Data = 0;
	DP->DataIndex = 0;

	DP->MessageComplete = 0;
}

static bool SigLeaderCheck( int level, uint32_t tickdif, DataPack *DP)
{
	if ( level == 1 && tickdif > MAX_WIDTH_REPEAT_CYCLE )
	{
		ResetDP( DP );
	}
	if ( level == 0 && uValIsEqual_Err( tickdif, LEADER_LENGTH, ALLOWED_ERROR ) ) return !( DP->ReceivedLeader = true );
	return false;
}

static bool SigBufferCheck( int level, uint32_t tickdif, DataPack *DP )
{
	if ( level == 1 )
	{
		if ( uValIsEqual_Err( tickdif, STANDARD_BUF, ALLOWED_ERROR ) ) 
		{
			DP->BufferType = 1;
			return !( DP->ReceivedBuffer = true );
		}
		if ( uValIsEqual_Err( tickdif, REPEAT_BUF, ALLOWED_ERROR ) ) 
		{	
			DP->BufferType = 2;
			return !( DP->ReceivedBuffer = true );
		}
	}
	return true;
}

static bool PullDataBit( int level, uint32_t tickdif, DataPack *DP )
{
	if ( DP->BufferType == 1 )
	{
		if ( !DP->PullReady ) 
		{
			if ( DP->DataIndex == DATA_WORD_SIZE ) return !( DP->MessageComplete = true );
			return !( DP->PullReady = uValIsEqual_Err( tickdif, STD_BIT_LEAD, ALLOWED_ERROR ) );
		}

		DP->DataIndex++;
		if ( !uValIsEqual_Err( tickdif, LOW_LENGTH, ALLOWED_ERROR ) )
		{
			if ( !uValIsEqual_Err( tickdif, HIGH_LENGTH, ALLOWED_ERROR ) )
			{
				return true;
			}

			DP->Data |= 1 << ( DATA_WORD_SIZE - DP->DataIndex ); 
			// since data is initialized as 0 incrementing the count is the same as setting the bit as zero 	
		}
	
		// bit has been pulled so reset and return
		return DP->PullReady = false;
	}
	if ( DP->BufferType == 2 )
	{
		return !( level == 0 && 
			  uValIsEqual_Err( tickdif, STD_BIT_LEAD, ALLOWED_ERROR ) && 
			  ( DP->MessageComplete = true ) );
	}

	return true;
}

static void IRDecode( int gpio, int level, uint32_t tick, void* dat )
{
	if ( level == 2 ) return; // ignore timed callbacks

	// get tick difference between now and most recent level change and refresh stored ref tick
	DataPack *DP = dat;
	uint32_t tickdif = tick - DP->LastTick;
	DP->LastTick = tick;

	if ( DP->invert ) level = 1 - level; // invert if necessary 

	// await 9ms leader signal
	if ( !DP->ReceivedLeader )
	{
		SigLeaderCheck( level, tickdif, DP );
		return;
	}

	// await filter space after leader
	if ( !DP->ReceivedBuffer )
	{
		if ( SigBufferCheck( level, tickdif, DP ) )
		{
			ResetDP( DP );
		}
		return;
	}
	
	// await and store 4 8-bit codes OR await signal repeat bit
	if ( PullDataBit( level, tickdif, DP ) )
	{
		ResetDP( DP );	
		return;
	}

	// await EOS pulse AND call callback with received data
	if ( !DP->MessageComplete ) return; // skip message reporting if message is not fully received

	Message DataMsg = { DP->Data, ( DP->BufferType == 2 ? true : false ) };
	
	DP->ReceivedLeader = false;
	DP->ReceivedBuffer = false;

	(*DP->Callback)( &DataMsg );
}

int Initialize_IR_NEC( int pin, IRMessageChangeCallback cb, int32_t invert )
{
	if( gpioSetMode( pin, PI_INPUT ) )
	{
		perror("Bad Pin Set p1");
		return 1;
	}

	DataPack *DP;

       	if ( !( DP = calloc( 1, sizeof( DataPack ) ) ) )
	{
		perror("Calloc Error in IR init");
		return 1;
	}

	DP->LastTick = gpioTick();
        DP->Callback = cb;
	DP->invert = !!invert; // ensures invert follows true/false spec I've #define(d)

	if( gpioSetAlertFuncEx( pin, IRDecode, DP ) )
	{
		perror("Bad Alert Function Set");
		free( DP );
		return 1;
	}

	return 0;
}

/*
 * example usage
 *

void cb( void* temp )
{
	Message *msg = temp;
	if ( msg->Repeated )
		printf("Repeat Sig Received: %i\n", msg->data);
	else
		printf("\n%x\n", msg->data);
}

int main(int argc, char **argv)
{
	if( gpioInitialise() == PI_INIT_FAILED )
	{
		perror("Bad Init");
		return 1;
	}

	if( Initialize_IR_NEC( 14, cb, true) )
	{
		return 1;
	}

	while( 1 )
	{
		sleep(1);
	}
	return 0;
}
*/
