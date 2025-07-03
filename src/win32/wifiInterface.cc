
#include "../global.h"
#include <wlanapi.h>


static HANDLE hWlan;
static DWORD useVersion = 0;
static PLIST devices;
static PLIST eventStates;

struct wifi_event_message {
	int closeEvent;
	WLAN_NOTIFICATION_DATA notice;
	const char *code;
	LOGICAL code_is_buffer;
	const char *source;
};

struct eventState {
	PLINKQUEUE eventQueue;
	constructorSet *c;
	Persistent<Function> cb;
};

struct device {
	int number;
	char *name;
	GUID guid;
	DWORD status;
	WCHAR GuidString[ 39 ];
	char friendlyname[ 64 ];
};



void getInterfaces_( ReturnValue<Value> *returnValue, Isolate *isolate, LOGICAL buildObject ) {
	Local<Array> result;
	if( buildObject )
		result = Array::New( isolate );
	PWLAN_INTERFACE_INFO_LIST list;
	WlanEnumInterfaces( hWlan, NULL, &list );
	struct device *pDev;
	INDEX idx;
	LIST_FORALL( devices, idx, struct device *, pDev ) {
		// delete all devices.
		pDev->number = -1;
	}
	for( int i = 0; i < list->dwNumberOfItems; i++ ) {
		LIST_FORALL( devices, idx, struct device *, pDev ) {
			if( MemCmp( &pDev->guid, &list->InterfaceInfo[ i ].InterfaceGuid, sizeof( GUID ) ) == 0 ) {
				pDev->number = i;
				break;
			}
		}
		if( !pDev ) {
			pDev         = NewArray( struct device, 1 );
			StringFromGUID2( list->InterfaceInfo[ i ].InterfaceGuid, (LPOLESTR)&pDev->GuidString
			                              , sizeof( pDev->GuidString ) / sizeof( *pDev->GuidString ) );

			pDev->name   = WcharConvert( list->InterfaceInfo[ i ].strInterfaceDescription );
			pDev->guid   = list->InterfaceInfo[ i ].InterfaceGuid;
			pDev->status = list->InterfaceInfo[ i ].isState;
			{
				HKEY hTemp;
				char buf[ 512 ];
				snprintf( buf, 512, "SYSTEM\\CurrentControlSet\\Enum\\SWD\\RADIO\\%ls", pDev->GuidString );
				DWORD dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE, buf, 0
				                             , KEY_QUERY_VALUE | STANDARD_RIGHTS_READ | STANDARD_RIGHTS_READ, &hTemp );
				if( dwStatus )
					lprintf( "open? %p  %s %08x %p", HKEY_LOCAL_MACHINE, buf, dwStatus, hTemp );
				char pValue[ 512 ];
				DWORD dwRetType, dwBufSize = 512;

				// LONG x = RegEnumValue( hTemp, 0, pValue, &dwBufSize, 0, 0, 0, 0 );
				// lprintf( "First enum is : %08x  %s", (int)x, pValue );
				// dwBufSize = 512;

				dwStatus = RegQueryValueEx( hTemp, "FriendlyName", 0, &dwRetType, (PBYTE)pValue, &dwBufSize );
				if( dwStatus )
					lprintf( "Probably had bad key so this is bad? %d %p %s", dwStatus, hTemp, pValue );
				RegCloseKey( hTemp );
				StrCpyEx( pDev->friendlyname, pValue, sizeof( pDev->friendlyname ) );
			}
			// lprintf( "String:%d %ls %s", x, pDev->GuidString, pDev->friendlyname );
			AddLink( &devices, pDev );
			pDev->number = FindLink( &devices, pDev );

		}
		pDev->status = list->InterfaceInfo[ i ].isState;
	}

	if( buildObject ) LIST_FORALL( devices, idx, struct device *, pDev ) {
			if( pDev->number < 0 )
				continue; // not in the list.
		Local<Context> context = isolate->GetCurrentContext();
		Local<Object> intr;
		intr = Object::New( isolate );
		SET( intr, "device"
			, String::NewFromUtf8/*NewFromTwoByte*/( isolate, pDev->name )
			        .ToLocalChecked() );

		SET( intr, "state", Number::New( isolate, pDev->status ) );

		SET( intr, "name"
			, String::NewFromUtf8( isolate, pDev->friendlyname, v8::NewStringType::kNormal ).ToLocalChecked() );
		switch( pDev->status ) {
		case wlan_interface_state_not_ready:
			SET( intr, "status", String::NewFromUtf8Literal( isolate, "ready" ) );
			break;
		case wlan_interface_state_connected:
			SET( intr, "status", String::NewFromUtf8Literal( isolate, "connected" ) );
			break;
		case wlan_interface_state_ad_hoc_network_formed:
			SET( intr, "status", String::NewFromUtf8Literal( isolate, "adhoc" ) );
			break;
		case wlan_interface_state_disconnecting:
			SET( intr, "status", String::NewFromUtf8Literal( isolate, "diconnecting" ) );
			break;
		case wlan_interface_state_disconnected:
			SET( intr, "status", String::NewFromUtf8Literal( isolate, "disconnected" ) );
			break;
		case wlan_interface_state_associating:
			SET( intr, "status", String::NewFromUtf8Literal( isolate, "associating" ) );
			break;
		case wlan_interface_state_discovering:
			SET( intr, "status", String::NewFromUtf8Literal( isolate, "discovering" ) );
			break;
		case wlan_interface_state_authenticating:
			SET( intr, "status", String::NewFromUtf8Literal( isolate, "authenticating" ) );
			break;
		}
		// list->InterfaceInfo[i].
		SETN( result, pDev->number, intr );
	}
	if( !result.IsEmpty() )
		returnValue->Set( result );
	WlanFreeMemory( list );

}

void getInterfaces( Local<Name> property, const PropertyCallbackInfo<Value> &args ) {
	ReturnValue<Value> rval = args.GetReturnValue();
	getInterfaces_( &rval, args.GetIsolate(), TRUE );
}

void connectWifi( const v8::FunctionCallbackInfo<Value> &args ) {
	Isolate *isolate       = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	DOT11_SSID ssid_data;
	WLAN_CONNECTION_PARAMETERS params;
	params.dot11BssType      = dot11_BSS_type_infrastructure; //dot11_BSS_type_any;
	params.pDesiredBssidList = NULL;
	params.dwFlags           = 0;
	//params.dwFlags |= WLAN_CONNECTION_HIDDEN_NETWORK; // if the network is hidden
	//params.dwFlags |= WLAN_CONNECTION_ADHOC_JOIN_ONLY; // if the network is adhoc
	//params.dwFlags |= WLAN_CONNECTION_IGNORE_PRIVACY_BIT;
	//params.dwFlags |= WLAN_CONNECTION_EAPOL_PASSTHROUGH;
	//params.dwFlags |= WLAN_CONNECTION_PERSIST_DISCOVERY_PROFILE; 
	//params.dwFlags |= WLAN_CONNECTION_PERSIST_DISCOVERY_PROFILE_CONNECTION_MODE_AUTO;
	//params.dwFlags |= WLAN_CONNECTION_PERSIST_DISCOVERY_PROFILE_OVERWRITE_EXISTING;

	int index;
	struct device *dev;
	
	if( args[ 0 ]->IsNumber() ) {
		index = args[ 0 ]->ToInteger( context ).ToLocalChecked()->Value();
		INDEX idx;
		LIST_FORALL( devices, idx, struct device *, dev ) {
			if( dev->number == index )
				break;
		}
	} else {
		String::Utf8Value devname( isolate, args[ 0 ]->ToString(context).ToLocalChecked() );
		LIST_FORALL( devices, index, struct device *, dev ) {
			//lprintf( "Looking for:%s %s", dev->friendlyname, *devname );
			if( StrCmp( dev->friendlyname, *devname ) == 0 ) {
				break;
			}
		}
		if( !dev ) {
			lprintf( "No Such Device: %s", *devname );
			return;
		}
	}
	//lprintf( "Using dev: %d %p", index, dev );
	String::Utf8Value ssid(
	     USE_ISOLATE( isolate ) args[ 1 ]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	String::Utf8Value profile(
	     USE_ISOLATE( isolate ) args[ 2 ]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
	params.strProfile = CharWConvert( *profile ); 
	//lprintf( "Profile:%ls", params.strProfile );
	params.pDot11Ssid = &ssid_data;
	params.wlanConnectionMode = wlan_connection_mode_profile;
	MemCpy( params.pDot11Ssid->ucSSID, *ssid, 32 );
	//lprintf( "profile %s", *profile);
	params.pDot11Ssid->uSSIDLength = StrLen( *ssid );
	//lprintf( "Sssid %s %d", *ssid, params.pDot11Ssid->uSSIDLength );
	if( params.pDot11Ssid->uSSIDLength > 32 )
		params.pDot11Ssid->uSSIDLength = 32;
	
	DWORD dwStatusConnect = WlanConnect( hWlan, &dev->guid, &params, NULL );

	args.GetReturnValue().Set( Integer::New( isolate, dwStatusConnect ) );
	//lprintf( "Connect Failed? %d", dwStausConnect );

	Release( params.strProfile );

}
//static nStrings1 = {


void disconnectWifi( const v8::FunctionCallbackInfo<Value> &args ) {
	Isolate *isolate       = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	// Local<Object> options = args[0]->ToObject( context ).ToLocalChecked();
	// BOOL option = options->TOBOOL( isolate );

	int index;
	struct device *dev;

	if( args[ 0 ]->IsNumber() ) {
		index = args[ 0 ]->ToInteger( context ).ToLocalChecked()->Value();
		INDEX idx;
		LIST_FORALL( devices, idx, struct device *, dev ) {
			if( dev->number == index )
				break;
		}
	} else {
		String::Utf8Value devname( isolate, args[ 0 ]->ToString( context ).ToLocalChecked() );
		LIST_FORALL( devices, index, struct device *, dev ) {
			// lprintf( "Looking for:%s %s", dev->friendlyname, *devname );
			if( StrCmp( dev->friendlyname, *devname ) == 0 ) {
				break;
			}
		}
		if( !dev ) {
			lprintf( "No Such Device: %s", *devname );
			return;
		}
	}
	DWORD dwStatusConnect = WlanDisconnect( hWlan, &dev->guid, NULL );

	args.GetReturnValue().Set( Integer::New( isolate, dwStatusConnect ) );
	// lprintf( "Connect Failed? %d", dwStausConnect );
}


static void handleNotifications( PWLAN_NOTIFICATION_DATA data, PVOID unused ) { 
	/*
	* typedef struct _WLAN_NOTIFICATION_DATA {
  DWORD NotificationSource;
  DWORD NotificationCode;
  GUID  InterfaceGuid;
  DWORD dwDataSize;
  PVOID pData;
} WLAN_NOTIFICATION_DATA, *PWLAN_NOTIFICATION_DATA;
*/
	CTEXTSTR source = NULL;
	CTEXTSTR code   = NULL;
#define ss(a) source=a; /*lprintf( a );*/
#define cs(a) code=a; /*lprintf( a );*/
	if( data->NotificationSource & L2_NOTIFICATION_SOURCE_WLAN_MSM ) {
		ss( "WLAN MSM notification" );
	}
	if( data->NotificationSource & L2_NOTIFICATION_SOURCE_WLAN_ACM ) {
		ss( "WLAN ACM notification" );
	} 
	if( data->NotificationSource & L2_NOTIFICATION_SOURCE_WLAN_SECURITY ) {
		ss( "WLAN Security notification" );
	} else if( data->NotificationSource & L2_NOTIFICATION_SOURCE_WFD ) {
		if( data->NotificationCode == 0x00100002 ) {

		}
	}
	if( data->pData
	  && data->NotificationCode != wlan_notification_acm_operational_state_change
	  && ( data->NotificationCode != wlan_notification_acm_scan_complete || data->dwDataSize != 12 )
	  && ( data->NotificationCode != wlan_notification_acm_scan_fail || data->dwDataSize != 4 )
	  && data->NotificationCode != wlan_notification_acm_profile_unblocked
	  && data->NotificationCode != wlan_notification_acm_connection_start
	  && data->NotificationCode != wlan_notification_acm_disconnecting
	  && data->NotificationCode != wlan_notification_acm_autoconf_enabled
	  && data->NotificationCode != wlan_notification_acm_autoconf_disabled
	  && data->NotificationCode != wlan_notification_acm_background_scan_disabled
	  && data->NotificationCode != wlan_notification_acm_bss_type_change
	  && data->NotificationCode != wlan_notification_acm_background_scan_enabled
	  && data->NotificationCode != wlan_notification_acm_connection_complete
	  && data->NotificationCode != wlan_notification_acm_disconnected
	  && data->NotificationCode != wlan_notification_acm_connection_attempt_fail
	  && data->NotificationCode != 0x10000002 //
	)
		;  // this was debugging to catch information that might be useful... I think I'm happy now with what I do know...
		//lprintf( "Event has extra data with it: %d %d %d", data->dwDataSize, data->NotificationSource
		//       , data->NotificationCode );
	struct device *pDev;
	INDEX idx;
	char *text_msgbuf = NULL;
	LIST_FORALL( devices, idx, struct device *, pDev ) {
		if( MemCmp( &pDev->guid, &data->InterfaceGuid, sizeof( GUID ) ) == 0 ) {
			break;
		}
	}

	if( !pDev ) {
		getInterfaces_( NULL, NULL, FALSE );
		LIST_FORALL( devices, idx, struct device *, pDev ) {
			if( MemCmp( &pDev->guid, &data->InterfaceGuid, sizeof( GUID ) ) == 0 ) {
				break;
			}
		}
		if( !pDev ) {
			//lprintf( "Found a new device? do a new enum?" );
			return;
		}
	}



	if( data->NotificationCode == wlan_notification_acm_background_scan_enabled ) {
		cs( "background scan enabled" );
	} else if( data->NotificationCode == wlan_notification_acm_scan_list_refresh ) {
		cs( "scan list refresh" );
	} else if( data->NotificationCode == wlan_notification_acm_background_scan_disabled ) {
		cs( "background scan disabled" );
	} else if( data->NotificationCode == wlan_notification_acm_connection_start ) {
		cs( "connection start" );
	} else if( data->NotificationCode == wlan_notification_acm_connection_complete ) {
		cs( "connection complete" );
	} else if( data->NotificationCode == wlan_notification_acm_scan_complete ) {
		cs( "scan complete" );
	} else if( data->NotificationCode == wlan_notification_acm_power_setting_change ) {
		cs( "power setting change" );
	} else if( data->NotificationCode == wlan_notification_acm_screen_power_change ) {
		cs( "screen power change" );
	} else if( data->NotificationCode == wlan_notification_acm_scan_fail ) {
		cs( "scan failed" );
	} else if( data->NotificationCode == wlan_notification_acm_connection_attempt_fail ) {
		cs( "connection failed" );
	} else if( data->NotificationCode == wlan_notification_acm_bss_type_change ) {
		cs( "bss type change" );
	} else if( data->NotificationCode == wlan_notification_acm_filter_list_change ) {
		cs( "filter list change" );
	} else if( data->NotificationCode == wlan_notification_acm_profile_blocked ) {
		cs( "profile blocked" );
	} else if( data->NotificationCode == wlan_notification_acm_disconnected ) {
		cs( "disconnected" );
	} else if( data->NotificationCode == wlan_notification_acm_adhoc_network_state_change ) {
		cs( "adhoc network state change" );
	} else if( data->NotificationCode == wlan_notification_acm_profile_unblocked ) {
		cs( "profile unblocked" );
	} else if( data->NotificationCode == wlan_notification_acm_autoconf_enabled ) {
		cs( "autoconf enabled" );
	} else if( data->NotificationCode == wlan_notification_acm_autoconf_disabled ) {
		cs( "autoconf disabled" );
	} else if( data->NotificationCode == wlan_notification_acm_interface_arrival ) {
		cs( "interface arrival" );
	} else if( data->NotificationCode == wlan_notification_acm_interface_removal ) {
		cs( "interface removal" );
	} else if( data->NotificationCode == wlan_notification_acm_profile_change ) {
		cs( "profile change" );
	} else if( data->NotificationCode == wlan_notification_acm_profile_name_change ) {
		cs( "profile name change" );
	} else if( data->NotificationCode == wlan_notification_acm_profiles_exhausted ) {
		cs( "profiles exhausted" );
	} else if( data->NotificationCode == wlan_notification_acm_network_not_available ) {
		cs( "network not available" );
	} else if( data->NotificationCode == wlan_notification_acm_network_available ) {
		cs( "network available" );
	} else if( data->NotificationCode == wlan_notification_acm_disconnecting ) {
		cs( "disconnecting" );
	} else if( data->NotificationCode == wlan_notification_acm_operational_state_change ) {
		cs( "operational state change" );
	} else {
		text_msgbuf = NewArray( char, 42 );
		snprintf( text_msgbuf, 42, "Unknown notification code: %d", data->NotificationCode );
		code = text_msgbuf;
		//lprintf( "Unknown notification code: %d", data->NotificationCode );
	}

	if( source && code ) {
		INDEX idx;
		struct eventState *es;
		LIST_FORALL( eventStates, idx, struct eventState *, es ) {
			if( es->cb.IsEmpty() )
				continue;
			struct wifi_event_message *msgbuf
			     = NewPlus( struct wifi_event_message, sizeof( wifi_event_message ) + data->dwDataSize );
			msgbuf->notice         = *data;
			msgbuf->code           = code;
			msgbuf->code_is_buffer = !!text_msgbuf;
			msgbuf->source         = source;
			if( data->dwDataSize ) {
				msgbuf->notice.pData = msgbuf + 1;
				MemCpy( msgbuf->notice.pData, data->pData, data->dwDataSize );
			}
			msgbuf->closeEvent    = 0;
			EnqueLink( &es->eventQueue, msgbuf );
			uv_async_send( &es->c->wifiAsync );
		}
	}
	//if( data->NotificationCode == wlan_notification_acm_disconnected ) {
	//data->NotificationCode
	/*This member can be a ONEX_NOTIFICATION_TYPE, WLAN_NOTIFICATION_ACM, WLAN_NOTIFICATION_MSM, or WLAN_HOSTED_NETWORK_NOTIFICATION_CODE enumeration value.*/
	//ONEX_NOTIFICATION_TYPE asdf;
	//WLAN_NOTIFICATION_ACM acm;
	//WLAN_HOSTED_NETWORK_NOTIFICATION_CODE code;
	/*    
	wlan_hosted_network_state_change = L2_NOTIFICATION_CODE_V2_BEGIN,
    wlan_hosted_network_peer_state_change,
    wlan_hosted_network_radio_state_change,
	*/


	//WlanNotificationCallback( 
}

void setupNotifications( void ) { 
	WlanRegisterNotification( hWlan, WLAN_NOTIFICATION_SOURCE_ALL, TRUE, handleNotifications
	                        , NULL, NULL, NULL );
}

static void wifi_asyncmsg__( Isolate *isolate, Local<Context> context, struct eventState * myself ) {
	struct wifi_event_message *msg;
	while( msg = (struct wifi_event_message *)DequeLink( &myself->eventQueue ) ) {
		Local<Object> event = Object::New( isolate );
		struct device *pDev;

		INDEX idx;
		LIST_FORALL( devices, idx, struct device *, pDev ) {
			if( MemCmp( &pDev->guid, &msg->notice.InterfaceGuid, sizeof( GUID ) ) == 0 ) {
				break;
			}
		}
		SET( event, "interface", Integer::New( isolate, pDev?pDev->number:-2 ) );
		SET( event, "source", Integer::New( isolate, msg->notice.NotificationSource ) );
		SET( event, "sourceText", String::NewFromUtf8( isolate, msg->source ).ToLocalChecked() );
		SET( event, "code", Integer::New( isolate, msg->notice.NotificationCode ) );
		SET( event, "codeText", String::NewFromUtf8( isolate, msg->code ).ToLocalChecked() );
		if( msg->notice.NotificationCode == wlan_notification_acm_operational_state_change
		  || msg->notice.NotificationCode == wlan_notification_acm_scan_fail ) {
			SET( event, "mode", Integer::New( isolate, *(ULONG*)msg->notice.pData ) );
		} else if( msg->notice.NotificationCode == wlan_notification_acm_scan_complete ) {
			if( msg->notice.pData ) {
				SET( event, "index", Integer::New( isolate, ( (ULONG *)msg->notice.pData )[ 0 ] ) );
				SET( event, "v1", Integer::New( isolate, ( (ULONG *)msg->notice.pData )[ 1 ] ) );
				SET( event, "v2", Integer::New( isolate, ( (ULONG *)msg->notice.pData )[ 2 ] ) );
			} else {
				// no results?
				SET( event, "index", Integer::New( isolate, -1 ) );
				SET( event, "v1", Integer::New( isolate, -1 ) );
				SET( event, "v2", Integer::New( isolate, -1 ) );
			}
		} else if( msg->notice.NotificationCode == wlan_notification_acm_profile_unblocked ) {
			SET( event, "name", String::NewFromTwoByte( isolate, (uint16_t *) (((ULONG*)msg->notice.pData)+1) ).ToLocalChecked() );
		} else if( msg->notice.NotificationCode == wlan_notification_acm_connection_start
			|| msg->notice.NotificationCode == wlan_notification_acm_disconnecting 
			|| msg->notice.NotificationCode == wlan_notification_acm_background_scan_disabled 
			|| msg->notice.NotificationCode == wlan_notification_acm_autoconf_enabled 
			|| msg->notice.NotificationCode == wlan_notification_acm_autoconf_disabled
	         || msg->notice.NotificationCode == wlan_notification_acm_bss_type_change
			|| msg->notice.NotificationCode == wlan_notification_acm_background_scan_enabled
		         || msg->notice.NotificationCode == wlan_notification_acm_connection_complete
		         || msg->notice.NotificationCode == wlan_notification_acm_disconnected
		         || msg->notice.NotificationCode == wlan_notification_acm_connection_attempt_fail
			) {
			if( msg->notice.pData ) {
				struct buf {
					ULONG zero;
					WCHAR profile[ 256 ];
					ULONG ssidLen;
					char SSID[ 32 ];
				} *tmp = (struct buf *)msg->notice.pData;
				SET( event, "profile", String::NewFromTwoByte( isolate, (uint16_t *)tmp->profile ).ToLocalChecked() );
				SET( event, "SSID"
				   , String::NewFromUtf8( isolate, tmp->SSID, v8::NewStringType::kNormal, tmp->ssidLen ).ToLocalChecked() );
			}

		}
	
		Local<Function> cb     = myself->cb.Get( isolate );
		Local<Value> args[ 1 ] = { event };
		cb->Call( context, event, 1, args );
		if( msg->code_is_buffer )
			Release( msg->code );
		Release( msg );
	}
}

void handleWifiMsg( uv_async_t* handle ) {
	v8::Isolate *isolate = v8::Isolate::GetCurrent();
	HandleScope scope( isolate );
	wifi_asyncmsg__( isolate, isolate->GetCurrentContext(), (struct eventState *)handle->data );

	{
		class constructorSet *c = getConstructors( isolate );
		if( !c->ThreadObject_idleProc.IsEmpty() ) {
			Local<Function> cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
			cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
		}
	}
}

static void registerEventCallback( const v8::FunctionCallbackInfo<Value> &args ) {
	Isolate *isolate        = args.GetIsolate();
	class constructorSet *c = getConstructors( isolate );
	INDEX idx;
	struct eventState *es;
	// wait until someone wants events to register for events.

	LIST_FORALL( eventStates, idx, struct eventState *, es ) {
		if( es->c == c ) {
			break;
		}
	}

	if( es && !es->c->wifiAsync.data ) {
		es->c->wifiAsync.data = es;
		uv_async_init( c->loop, &c->wifiAsync, handleWifiMsg );
	}


	if( args.Length() > 0 && args[ 0 ]->IsFunction() ) {
		Local<Function> cb = Local<Function>::Cast( args[ 0 ] );
		es->cb.Reset( isolate, cb );
	} else {
		isolate->ThrowException( Exception::Error(
			    String::NewFromUtf8Literal( isolate, "Must specify a function to register for events." ) ) );
	}
}

void InitWifiInterface( Isolate *isolate, Local<Object>exports ){
	//Isolate* isolate = Isolate::GetCurrent();
	Local<Context> context = isolate->GetCurrentContext();
	constructorSet *c          = getConstructors(isolate);
	struct eventState *es      = new eventState();
	es->c                      = c;
	es->eventQueue             = CreateLinkQueue();
	AddLink( &eventStates, es );

	Local<Object> o2 = Object::New( isolate );

	o2->SetNativeDataProperty( context, String::NewFromUtf8Literal( isolate, "interfaces" )
		, getInterfaces
		, nullptr //Local<Function>()
		, Local<Value>()
		, PropertyAttribute::ReadOnly
		, SideEffectType::kHasSideEffect
		, SideEffectType::kHasSideEffect
	);

	SET_READONLY_METHOD( o2, "onEvent", registerEventCallback );

	SET_READONLY_METHOD( o2, "connect", connectWifi );
	SET_READONLY_METHOD( o2, "disconnect", disconnectWifi );
	SET_READONLY( exports, "WIFI", o2 );
}

PRELOAD( initLibrary ) { 
	//DWORD dwError1 = 
	WlanOpenHandle( 2, NULL, &useVersion, &hWlan );
	setupNotifications();
}
