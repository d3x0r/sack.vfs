
#include "sack.h"

uintptr_t opened( PCLIENT pc, uintptr_t psv )
{
   WebSocketEnableAutoPing( pc, 60000 );
	WebSocketBeginSendText( pc, "[", 1 );
	WebSocketBeginSendText( pc, "{\"op\":\"firstOp\"}", 16 );
	WebSocketBeginSendText( pc, ",", 1 );
	WebSocketBeginSendText( pc, "{\"op\":\"secondOp\"}", 17 );
	WebSocketSendText( pc, "]", 1 );

	WebSocketSendText( pc, "{\"op\":\"login\",\"key\":\"AlphaNumeric\"}", sizeof( "{\"op\":\"login\",\"key\":\"AlphaNumeric\"}" )-1 );

   return psv;
}

void event( PCLIENT pc, uintptr_t psv, CPOINTER buffer, size_t msglen )
{
	lprintf( "Received:" );
   LogBinary( buffer, msglen );
}
void error( PCLIENT pc, uintptr_t psv, int error )
{
	lprintf( "error: %d", error );
}
void closed( PCLIENT pc, uintptr_t psv )
{
   ((PCLIENT*)psv)[0] = NULL;
}


int main( int argc, char **argv )
{
InvokeDeadstart();
NetworkStart();
	PCLIENT pc = WebSocketOpen( argv[1], 0,
										opened
									  , event
									  , closed
                              , error
									  , (uintptr_t)&pc
									  , "C&C" );
	while( pc )
      WakeableSleep(1000 );
}
