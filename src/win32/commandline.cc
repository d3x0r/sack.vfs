
#define UMDF_USING_NTSTATUS 

#include "../global.h"

#include <ntstatus.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <winternl.h>


// missing value for getting command line found in Process Hacker https://sourceforge.net/p/yaprocmon/
// NtQueryInformationProcess 
// ProcessCommandLineInformation, // 60, q: UNICODE_STRING

#define ProcessCommandLineInformation 60


struct info {
	UNICODE_STRING uString;
	wchar_t chars[1];
};

int GetProcessParent( int pid ) {
	HANDLE hToken;
	//BOOL bOpenToken = 
	OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken );

	LUID luid;
	LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &luid );
	//LookupPrivilegeValue( NULL, SE_, &luid3 );
	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	// Enable the privilege
	//BOOL bAdjust = 
	AdjustTokenPrivileges( hToken,
		FALSE,
		&tp,
		0, // size of the next privilege thing
		(PTOKEN_PRIVILEGES)NULL,
		(PDWORD)NULL );
	//lprintf( "Token Adjust: %d %d", bOpenToken, bAdjust );
	HANDLE hp = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	PROCESSENTRY32 pe = { 0 };
	pe.dwSize = sizeof( PROCESSENTRY32 );
	int found = 0;
	{
			if( Process32First( hp, &pe ) ) {
				do {
					if( pe.th32ProcessID < 5) continue; // skip idle and System
					//static wchar_t processBaseName[256];
					
					if( pe.th32ProcessID == pid ) {
						if( pe.th32ParentProcessID > 5)
							found = pe.th32ParentProcessID;
						break;
					}

				} while( Process32Next( hp, &pe ) );
			}
	}
	CloseHandle( hp );
	CloseHandle( hToken );
	return found;
}


PLIST GetProcessCommandLines( const char* process ) {
	PLIST results = NULL;
	HANDLE hToken;
	//BOOL bOpenToken = 
	OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken );

	LUID luid;
	LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &luid );
	//LookupPrivilegeValue( NULL, SE_, &luid3 );
		TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	// Enable the privilege
	//BOOL bAdjust = 
	AdjustTokenPrivileges( hToken,
		FALSE,
		&tp,
		0, // size of the next privilege thing
		(PTOKEN_PRIVILEGES)NULL,
		(PDWORD)NULL );
	//lprintf( "Token Adjust: %d %d", bOpenToken, bAdjust );
	HANDLE hp = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	size_t processNameLen = StrLen( process );
	PROCESSENTRY32 pe = { 0 };
	pe.dwSize = sizeof( PROCESSENTRY32 );
	{
		struct info* pInfo = NewPlus( struct info, 1500 * sizeof( wchar_t ) );
		size_t zInfoSize = sizeof( struct info ) + 1500 * sizeof( wchar_t );
		pInfo->uString.Buffer = pInfo->chars;
		pInfo->uString.Length = 0;
		pInfo->uString.MaximumLength = (USHORT)(zInfoSize - sizeof( UNICODE_STRING ));
		wchar_t* processFileName = NewArray( wchar_t, 256 );
		size_t processFileNameLength = 256;
		if( Process32First( hp, &pe ) ) {
			do {
				if( pe.th32ProcessID < 5) continue; // skip idle and System
				//static wchar_t processBaseName[256];
				HANDLE hProcess = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);

				if( hProcess ) {
					ULONG returnLength;
					LOGICAL retry;
					NTSTATUS result;
					// base Name needs PROCESS_VM_READ and PROCESS_QUERY_INFORMATION
					//GetModuleBaseNameW( hProcess, NULL, processBaseName, 256 );
					do {
						DWORD dwResult = GetProcessImageFileNameW(
							hProcess,
							processFileName,
							(DWORD)processFileNameLength
						);
						retry = FALSE;
						if( !dwResult ) {
							DWORD dwError = GetLastError();
							if( dwError == ERROR_INVALID_HANDLE ) {
								lprintf( "error Code: %d %p", dwError, hProcess );
								continue;
							} else if( dwError == ERROR_INSUFFICIENT_BUFFER ) {
								processFileNameLength *= 2;
								ReleaseEx( processFileName DBG_SRC );
								processFileName = NewArray( wchar_t, processFileNameLength );
								retry = TRUE;
							} else
								lprintf( "error Code: %d", dwError );
						}
					} while( retry );
					wchar_t* filename = pathrchrW( processFileName );
					if( filename ) filename++; else filename = processFileName;
					//lprintf( "Testing %d %s %S", dwResult, process, filename );
					if( process && ( StrCaseCmpEx_u8u16( process, filename, processNameLen ) != 0 ) ) {
						CloseHandle( hProcess );
						continue;
					}
					do {
						retry = FALSE;
						result = NtQueryInformationProcess( hProcess, (PROCESSINFOCLASS)ProcessCommandLineInformation
							, pInfo, (ULONG)zInfoSize, &returnLength );
						if( result == STATUS_INFO_LENGTH_MISMATCH ) {
							//lprintf( "Need more characters?" );
							Release( pInfo );
							pInfo = (struct info*)Allocate( returnLength );
							pInfo->uString.Buffer = pInfo->chars;
							pInfo->uString.Length = 0;
							pInfo->uString.MaximumLength = (USHORT)(returnLength - sizeof( UNICODE_STRING ));
							zInfoSize = returnLength;
							retry = TRUE;
						} else if( result == 0 ) {
							struct command_line_result* result_val = NewArray( struct command_line_result, 1 );
							result_val->dwProcessId = pe.th32ProcessID;
							result_val->processName = WcharConvert( processFileName );
							result_val->data = WcharConvert_v2( pInfo->chars, pInfo->uString.Length, &result_val->length DBG_SRC );
							AddLink( &results, result_val );
							//lprintf( "Adding result: %p %p %S", pInfo->uString.Buffer, pInfo->chars, pInfo->chars );
						} else {
							lprintf( "Unhandled status: %08x for PID:%d", result, pe.th32ProcessID );
						}
					} while( retry );

					CloseHandle( hProcess );
				} else {
					DWORD dwError = GetLastError();
					if( dwError == ERROR_ACCESS_DENIED ) {
						//lprintf( "Didn't open process ID %d %d", dwError, pe.th32ProcessID );
					} else 
						lprintf( "Didn't open process ID %d %d", dwError, pe.th32ProcessID );
				}
			} while( Process32Next( hp, &pe ) );
		}
		ReleaseEx( pInfo DBG_SRC );
		ReleaseEx( processFileName DBG_SRC );
	}
	CloseHandle( hp );
	CloseHandle( hToken );
	return results;
}

void ReleaseCommandLineResults( PLIST* ppResults ) {
	if( !ppResults ) return;

	INDEX idx;
	struct command_line_result* result;
	LIST_FORALL( ppResults[0], idx, struct command_line_result*, result ) {
		ReleaseEx( result->processName DBG_SRC );
		ReleaseEx( result->data DBG_SRC );
		ReleaseEx( result DBG_SRC );
	}

	DeleteList( ppResults );
}

#if 0
void test( void ) {
	PLIST results = GetCommandLine( NULL );// "chro" );
	INDEX idx;
	struct command_line_result* result;
	LIST_FORALL( results, idx, struct command_line_result*, result ) {
		lprintf( "Found: %d %.*s", result->dwProcessId, result->length, result->data );
	}
	ReleaseCommandLineResults( &results );

	results = GetCommandLine( NULL );
	LIST_FORALL( results, idx, struct command_line_result*, result ) {
		lprintf( "Found: %d %.*s", result->dwProcessId, result->length, result->data );
	}
	ReleaseCommandLineResults( &results );

}
#endif