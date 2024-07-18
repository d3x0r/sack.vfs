#if WIN32

#include "global.h"

//Persistent<Function> RegObject::constructor;

void RegObject::Init( Local<Object> exports ) {
	Isolate* isolate = Isolate::GetCurrent();
	Local<Context> context = isolate->GetCurrentContext();
	Local<Object> regInterface = Object::New( isolate );

	// regInterface->Set( String::NewFromUtf8Literal( isolate, "get" ),

	NODE_SET_METHOD( regInterface, "get", getRegItem );
	NODE_SET_METHOD( regInterface, "set", setRegItem );

	SET( exports, "registry", regInterface );

}

#ifdef WIN32
static HKEY resolveHive( char *name ) {
	if( StrCaseCmp( name, "HKCU" ) == 0 || StrCaseCmp( name, "HKEY_CURRENT_USER" ) == 0 ) {
		return HKEY_CURRENT_USER;
	} else if( StrCaseCmp( name, "HKLM" ) == 0 || StrCaseCmp( name, "HKEY_LOCAL_MACHINE" ) == 0 ) {
		return HKEY_LOCAL_MACHINE;
	} else {
	}
   return (HKEY)0;
}
#endif


void RegObject::getRegItem(const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = Isolate::GetCurrent();
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error(
		    String::NewFromUtf8Literal( isolate, "required parameter, regPath, is missing." ) ) );
		return;
	}

	String::Utf8Value string1( isolate,  args[0] );
	char *key1 = StrDup( *string1 );
	char *keyTmp = key1;
	while( keyTmp[0] ) { if( keyTmp[0] == '/' ) keyTmp[0] = '\\'; keyTmp++; }
	{
		char *start = key1;
		char *end;
		HKEY hive;
		if( !(end = (char*)pathchr( start )) && argc < 2 ) {
			isolate->ThrowException( Exception::Error(
												String::NewFromUtf8Literal( isolate, "required parameter, regKey, is missing." ) ) );
			Deallocate( char*, key1 );
			return;
		}

		end[0] = 0;
		end++;
		char *keyStart = (char*)pathrchr( end );
		if( !keyStart ) {

			isolate->ThrowException( Exception::Error(
																	String::NewFromUtf8Literal( isolate, "required parameter, regKey, is missing." ) ) );
			Deallocate( char*, key1 );
			return;

		}
		keyStart[0] = 0;
		keyStart++;

#ifdef WIN32
		hive = resolveHive( start );
#endif
		if( !hive ) {
			isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Unknown root hive specified" ) ) );
			Deallocate( char*, key1 );
			return;
		}

		uint32_t dwStatus;
		HKEY hTemp;
		start = end+1;

		dwStatus = RegOpenKeyEx( hive, end, 0
		    , KEY_QUERY_VALUE|STANDARD_RIGHTS_READ|STANDARD_RIGHTS_READ
		    , &hTemp );
		if( dwStatus )
			lprintf( "open? %p  %s %08x %p", hive, end, dwStatus, hTemp );
		if( dwStatus == ERROR_FILE_NOT_FOUND )
		{
			// lprintf( "Key does not exist." );
			// on read don't create missing paths.
			// return 'undefined'
			return;
#ifdef WIN32

			DWORD dwDisposition;
			dwStatus = RegCreateKeyEx( hive,
													  end, 0
													 , /* it's only an in...*/(LPSTR)""
													 , REG_OPTION_NON_VOLATILE
													 , KEY_ALL_ACCESS
													 , NULL
													 , &hTemp
													 , &dwDisposition);
					if( dwDisposition == REG_OPENED_EXISTING_KEY )
						lprintf( "Failed to open, then could open???" );
					if( dwStatus ) {	// ERROR_SUCCESS == 0

						isolate->ThrowException( Exception::Error(
																				String::NewFromUtf8Literal( isolate, "Logic error" ) ) );
						Deallocate( char*, key1 );
						return;
					}
#endif
		}
		char pValue[512];
		DWORD dwRetType, dwBufSize = 512;
		
		//LONG x = RegEnumValue( hTemp, 0, pValue, &dwBufSize, 0, 0, 0, 0 );
		//lprintf( "First enum is : %08x  %s", (int)x, pValue );
		//dwBufSize = 512;

#ifdef WIN32
		dwStatus = RegQueryValueEx(hTemp, keyStart, 0                    	
										  , &dwRetType
										, (PBYTE)pValue
										  , &dwBufSize );

		RegCloseKey( hTemp );
#endif
		bool swap = false;
		if( dwStatus == ERROR_SUCCESS )
		{
			switch( dwRetType ) {
			case REG_EXPAND_SZ:
				{
					char expand[1024];
					ExpandEnvironmentStrings( pValue, expand, 1024 );
					args.GetReturnValue().Set( String::NewFromUtf8( isolate, expand, v8::NewStringType::kNormal ).ToLocalChecked() );
				}
				break;
			case REG_SZ:
				args.GetReturnValue().Set( String::NewFromUtf8( isolate, pValue, v8::NewStringType::kNormal ).ToLocalChecked() );
				break;

			case REG_DWORD_BIG_ENDIAN: {
				swap = true;
			//case REG_DWORD_LITTLE_ENDIAN:  this is also 4
			case REG_DWORD:
				{
					DWORD result;
					if( swap ) {
						char tmp = pValue[0];
						pValue[0] = pValue[3];
						pValue[3] = tmp;
						tmp = pValue[1];
						pValue[1] = pValue[2];
						pValue[2] = tmp;
					}
					result = ((DWORD*)pValue)[0];
					args.GetReturnValue().Set( Number::New( isolate, result ) );
				}
			}
			break;
			default:
				isolate->ThrowException( Exception::Error(
						String::NewFromUtf8Literal( isolate, "unsupported value type from registry." ) ) );
			}
		}
	}
}

void RegObject::setRegItem(const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error(
					String::NewFromUtf8Literal( isolate, "required parameter, regPath, is missing." ) ) );
		return;
	}

	String::Utf8Value string1( isolate,  args[0] );
	char *key1 = StrDup( *string1 );
	char *keyTmp = key1;
	while( keyTmp[0] ) { if( keyTmp[0] == '/' ) keyTmp[0] = '\\'; keyTmp++; }
	{
		char *start = key1;
		char *end;
		HKEY hive;
		if( !(end = (char*)pathchr( start )) && argc < 2 ) {
			isolate->ThrowException( Exception::Error(
						String::NewFromUtf8Literal( isolate, "required parameter, regKey, is missing." ) ) );
			Deallocate( char*, key1 );
			return;

		}

		end[0] = 0;
		end++;
		char *keyStart = (char*)pathrchr( end );
		if( !keyStart ) {

			isolate->ThrowException( Exception::Error(
					String::NewFromUtf8Literal( isolate, "required parameter, regKey, is missing." ) ) );
			Deallocate( char*, key1 );
			return;

		}
		keyStart[0] = 0;
		keyStart++;

#ifdef WIN32
		hive = resolveHive( start );
#endif
		if( !hive ) {
			isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Unknown root hive specified" ) ) );
			Deallocate( char*, key1 );
			return;
		}

		DWORD dwStatus;
		HKEY hTemp;
		start = end+1;

		dwStatus = RegOpenKeyEx( hive, end, 0, KEY_ALL_ACCESS, &hTemp );
		if( dwStatus )
			lprintf( "open? %p  %s %08x %p", hive, end, dwStatus, hTemp );
		if( dwStatus == ERROR_FILE_NOT_FOUND )
		{
			DWORD dwDisposition;
#ifdef WIN32
			dwStatus = RegCreateKeyEx( hive,
													  end, 0
													 , ""
													 , REG_OPTION_NON_VOLATILE
													 , KEY_ALL_ACCESS
													 , NULL
													 , &hTemp
													 , &dwDisposition);
					if( dwDisposition == REG_OPENED_EXISTING_KEY )
						lprintf( "Failed to open, then could open???" );
					if( dwStatus ) {	// ERROR_SUCCESS == 0

						isolate->ThrowException( Exception::Error(
																				String::NewFromUtf8Literal( isolate, "Logic error" ) ) );
						Deallocate( char*, key1 );
						return;
					}
#endif
		}

		if( args[1]->IsNumber() ) {
			double v = args[1]->NumberValue(isolate->GetCurrentContext()).FromMaybe(0);
			DWORD dw = (DWORD)v;
#ifdef WIN32
			dwStatus = RegSetValueEx(hTemp, keyStart, 0
										  , REG_DWORD
										  , (const BYTE *)&dw, 4 );
#endif
			lprintf( "stauts of update is %d", dwStatus );

		} else if( args[1]->IsString() ) {
			String::Utf8Value val( isolate,  args[1] );
#ifdef WIN32
			dwStatus = RegSetValueEx(hTemp, keyStart, 0
										  , REG_SZ
										  , (const BYTE *)*val, (DWORD)StrLen( *val ) );
#endif
			if( dwStatus )
				lprintf( "Failed to set registry? %d %p %s", dwStatus, hTemp, keyStart );

		} else {
			isolate->ThrowException( Exception::Error(
																	String::NewFromUtf8Literal( isolate, "Don't know how to handle value passed." ) ) );
#ifdef WIN32
			RegCloseKey( hTemp );
#endif
			Deallocate( char*, key1 );
			return;
		}

		RegCloseKey( hTemp );

	}
}

#endif
