
#include <mi.h>
#include "sack.h"

struct local {
	MI_Application miApplication;
	MI_Session miSession;
} l;

//Error	LNK2019	unresolved external symbol MI_Application_InitializeV1 referenced in function "void __cdecl MI_Init(void)" ( ? MI_Init@@YAXXZ )	sack_vfs	M : \javascript\vfs\native\build\wmi.obj	1

void MI_Init( void ) {

	MI_Result miResult;
	lprintf( "Init..." );
	miResult = MI_Application_Initialize( 0,               // Flags - Must be 0
		NULL,            // Application ID
		NULL,            // Extended error
		&l.miApplication ); // Application 
	if( miResult != MI_RESULT_OK ) { /* Handle error */
	}

	lprintf( "new Session..." );
	miResult = MI_Application_NewSession( &l.miApplication, // Application 
		L"WINRM",       // WimRM Protocol
		L"localhost",   // Machine destination
		NULL,           // Options
		NULL,           // Callbacks
		NULL,           // Extended error
		&l.miSession );    // Session 
	if( miResult != MI_RESULT_OK ) { /* Handle error */
	}

	MI_Operation miOperation = MI_OPERATION_NULL;

	lprintf( "Enumerate..." );

	MI_Session_EnumerateInstances( &l.miSession,       // Session 
		0,               // Flags
		NULL,            // Options
		L"root\\cimv2",   // CIM Namespace
		L"Win32_Process", // Class name
		MI_FALSE,        // Retrieve only keys
		NULL,            // Callbacks
		&miOperation );   // Operation

	const MI_Instance *miInstance;
	const MI_Instance* completionDetails;
	MI_Boolean more;
	const MI_Char *errorMessage;
	int instanceCount = 0;
	do {
		lprintf( "Get Instance..." );
		miResult = MI_Operation_GetInstance( &miOperation,
			&miInstance,
			&more,
			&miResult,
			&errorMessage,
			&completionDetails
		);
		lprintf( "Soemthing? %d %S", miResult, errorMessage );
		//The following demonstrates using the instance just received.
		if( miInstance ) {
			MI_Value value;
			MI_Type type;
			MI_Uint32 flags;
			lprintf( "Get Element" );
			//Athough the Name property is shown here to demonstrate, you could substitute another property
			miResult = MI_Instance_GetElement( miInstance,  // Instance
				L"Name",     // Element (property) name
				&value,      // Element value
				&type,       // Element type
				&flags,      // Flags
				NULL );       // Index
			if( miResult != MI_RESULT_OK ) {
				wprintf( L"MI_Instance_GetElement failed. MI_RESULT: %ld)\n", miResult );
				return;
			}
			lprintf( "Process Name: %S\n", value.string );

			instanceCount++;
		}


	} while( more );

	lprintf( "Operation Close?" );
	miResult = MI_Operation_Close( &miOperation );
	if( miResult != MI_RESULT_OK ) { /* Handle error */
	}

	lprintf( "session close" );
	miResult = MI_Session_Close( &l.miSession, // Session
		NULL,       // Completion context
		NULL );      // Completion callback
	if( miResult != MI_RESULT_OK ) { /* Handle error */
	}

	lprintf( "Application Close?" );
	miResult = MI_Application_Close( &l.miApplication );
	if( miResult != MI_RESULT_OK ) { /* Handle error */
	}

}

