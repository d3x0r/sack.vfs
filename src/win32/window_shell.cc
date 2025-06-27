#include "../global.h"

// file associations can be done via
//	administrative privilege cmd.exe
//
//C:\Windows\system32>assoc .autoconfigbackup2=intershellbackup
//.autoconfigbackup2=intershellbackup
//
//C:\Windows\system32>assoc .autoconfigbackup3=intershellbackup
//.autoconfigbackup3=intershellbackup
//
//C:\Windows\system32>ftype intershellbackup=intershell.exe -restore -SQL %1
//intershellbackup=c:\full\path\to\intershell\intershell.exe -restore -SQL %1


//C:\Windows\system32>ftype /?
//Displays or modifies file types used in file extension associations
//
//FTYPE [fileType[=[openCommandString]]]
//
//  fileType  Specifies the file type to examine or change
//  openCommandString Specifies the open command to use when launching files
//						  of this type.
//
//Type FTYPE without parameters to display the current file types that
//have open command strings defined.  FTYPE is invoked with just a file
//type, it displays the current open command string for that file type.
//Specify nothing for the open command string and the FTYPE command will
//delete the open command string for the file type.  Within an open
//command string %0 or %1 are substituted with the file name being
//launched through the assocation.  %* gets all the parameters and %2
//gets the 1st parameter, %3 the second, etc.  %~n gets all the remaining
//parameters starting with the nth parameter, where n may be between 2 and 9,
//inclusive.  For example:
//
//	 ASSOC .pl=PerlScript
//	 FTYPE PerlScript=perl.exe %1 %*
//
//would allow you to invoke a Perl script as follows:
//
//	 script.pl 1 2 3
//
//If you want to eliminate the need to type the extensions, then do the
//following:
//
//	 set PATHEXT=.pl;%PATHEXT%
//
//and the script could be invoked as follows:
//
//	 script 1 2 3
//
//
//
//C:\Windows\system32>ftype intershellbackup=c:\bin32\intershell\intershell.exe -restore -SQL %1
//intershellbackup=c:\bin32\intershell\intershell.exe -restore -SQL %1
//



BOOL IsElevated( void ) {
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if( OpenProcessToken( GetCurrentProcess( ),TOKEN_QUERY,&hToken ) ) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof( TOKEN_ELEVATION );
        if( GetTokenInformation( hToken, TokenElevation, &Elevation, sizeof( Elevation ), &cbSize ) ) {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if( hToken ) {
        CloseHandle( hToken );
    }
    return fRet;
}

void isElevated( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	args.GetReturnValue().Set( IsElevated()?True(isolate):False(isolate) );	
}



void BeginWindowsShell( void ) 
{
	// Credit to Nicolas Escuder for figuring out how to *Hide the pretteh XP Logon screen*!
	// Credit also to GeoShell 4.11.8 changelog entry
	//lprintf( "Credit to Nicolas Escuder for figuring out how to *Hide the pretteh XP Logon screen*!" );
	//lprintf( "Credit also to GeoShell version 4.11.8 changelog entry..." );
	// Code borrowed from LiteStep (give it back when we're done using it ^.^).
	{
		HANDLE hLogonEvent = OpenEvent( EVENT_MODIFY_STATE, 1, "Global\\msgina: ShellReadyEvent" );	 // also: "Global\msgina: ReturnToWelcome"

		if( !hLogonEvent )
		{
			//lprintf( "Error : %d", GetLastError() );
			hLogonEvent = OpenEvent( EVENT_MODIFY_STATE, 1, "msgina: ShellReadyEvent" );	 // also: "Global\msgina: ReturnToWelcome"
			if( !hLogonEvent )
			{
				//lprintf( "Error : %d", GetLastError() );
				hLogonEvent = OpenEvent( EVENT_MODIFY_STATE, 1, "Global\\ShellDesktopSwitchEvent" );	 // also: "Global\msgina: ReturnToWelcome"

				if( !hLogonEvent )
				{
					//lprintf( "Error : %d", GetLastError() );
					hLogonEvent = OpenEvent( EVENT_MODIFY_STATE, 1, "ShellDesktopSwitchEvent" );	 // also: "Global\msgina: ReturnToWelcome"
					if( !hLogonEvent )
						lprintf( "Error finding event to set for windows login" );
				}
			}
		}
		SetEvent( hLogonEvent );
		CloseHandle( hLogonEvent );
	}

}

void beginWindowsShell( const v8::FunctionCallbackInfo<Value>& args ) {
	BeginWindowsShell();
}

BOOL SetShell( Isolate* isolate, CTEXTSTR program )
{
	DWORD dwStatus;
	HKEY hTemp;

	dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
	                        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", 0,
	                        KEY_WRITE, &hTemp );

	if( dwStatus == ERROR_FILE_NOT_FOUND )
	{
			//Banner2Message( "Failed to find registry key." );
			return FALSE;
	}
	else if( dwStatus == ERROR_ACCESS_DENIED ) {
			//Banner2Message( "Access Denied to login key" );
			return FALSE;
	}

	if( (dwStatus == ERROR_SUCCESS) && hTemp )
	{
		//lprintf( "Write shell to: %s", my_button->shell );
		dwStatus = RegSetValueEx(hTemp, "Shell", 0
		                        , REG_SZ
		                        , (BYTE*)program, (DWORD)(strlen( program ) * sizeof( TEXTCHAR )) );
		RegCloseKey( hTemp );
		if( dwStatus == ERROR_SUCCESS )
		{
		}
		else {
			isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Failed to set shell." ) ) );
			return FALSE;//Banner2Message( "Failed to set shell" );
		}
	}
	return TRUE;
}

void setShell( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	String::Utf8Value process( USE_ISOLATE( isolate ) args[0]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	BOOL status = SetShell( isolate, *process );
	args.GetReturnValue().Set( status?True(isolate):False(isolate) );
}


BOOL DisableTaskManager( Isolate* isolate, DWORD dwOne ) {

	DWORD dwStatus;
	HKEY hTemp;

	dwStatus = RegOpenKeyEx( HKEY_CURRENT_USER,
	                         "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", 0,
	                         KEY_WRITE, &hTemp );
	if( dwStatus == ERROR_FILE_NOT_FOUND )
	{
		DWORD dwDispos;
		dwStatus = RegCreateKeyEx( HKEY_CURRENT_USER
		                         , "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", 0
		                         , NULL  //lpclass
		                         , 0  // dwOptions
		                         , KEY_WRITE //sam desired
		                         , NULL // secureity attrib
		                         , &hTemp
		                         , &dwDispos
		                         );

		if( dwStatus == ERROR_FILE_NOT_FOUND )
		{
		}
		else if( dwStatus == ERROR_ACCESS_DENIED ) {
			//Banner2Message( "Access Denied to login key" );
			return FALSE;
		}
	}
	else if( dwStatus == ERROR_ACCESS_DENIED ) {
			//Banner2Message( "Access Denied to login key" );
			return FALSE;
	}
	if( (dwStatus == ERROR_SUCCESS) && hTemp )
	{
		//DWORD dwOne = 1;
		dwStatus = RegSetValueEx(hTemp, "DisableTaskMgr", 0
		                         , REG_DWORD
		                         , (const BYTE *)&dwOne, sizeof( dwOne ) );
		if( dwStatus == ERROR_SUCCESS )
		{
		}
		else
			;//Banner2Message( "Failed to set disable task manager" );

		RegCloseKey( hTemp );
		return TRUE;
	}
	return FALSE;
}

void disableTaskManager( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	BOOL option = args[0]->TOBOOL( isolate );
	BOOL success = DisableTaskManager( isolate, option?1:0 );
	args.GetReturnValue().Set( success?True(isolate):False(isolate) );
}