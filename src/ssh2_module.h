#ifndef SSH2_MODULE_H
#define SSH2_MODULE_H

class WssObject;

enum SSH2_EventCodes {
	SSH2_EVENT_CLOSE,  // closes event handle
	SSH2_EVENT_ERROR,  // error event (is on a session operation)
	SSH2_EVENT_CHANNEL_ERROR, // error is on a channel operation
	SSH2_EVENT_HANDSHAKE, // handshake complete
	SSH2_EVENT_AUTHDONE, // authentication complete
	SSH2_EVENT_CONNECTED, // connected to server
	SSH2_EVENT_CHANNEL, // channel opened
	SSH2_EVENT_DATA, // data received
	SSH2_EVENT_SETENV,
	SSH2_EVENT_PTY,
	SSH2_EVENT_SHELL,
	SSH2_EVENT_EXEC,
	SSH2_EVENT_FORWARD,
	SSH2_EVENT_FORWARD_CONNECT,
	SSH2_EVENT_WS_REVERSE_CHANNEL,
	SSH2_EVENT_WS_REVERSE_CONNECT,
	SSH2_EVENT_NET_REVERSE_CHANNEL,
	SSH2_EVENT_NET_REVERSE_CONNECT,
	SSH2_EVENT_LISTEN_ERROR,
};

struct SSH2_Event {
	enum SSH2_EventCodes code;
	void* data;
	void* data2;
	int iData;
	size_t length;
	bool success;
	PTHREAD waiter;
	bool done;

};

class SSH2_Channel : public node::ObjectWrap {
public:
	class SSH2_Object* ssh2;
	struct ssh_channel* channel;
	Persistent<Object> jsObject;  // the object that represents this channel
	Persistent<Function> dataCallback; // called when data is received
	Persistent<Function> closeCallback; // called when the channel is closed
	Persistent<Function> errorCallback; // called when the channel experiences an error outside of a promised result

	Persistent<Promise::Resolver> ptyPromise;
	Persistent<Promise::Resolver> shellPromise;
	Persistent<Promise::Resolver> execPromise;
	Persistent<Promise::Resolver> setenvPromise;

	PLINKQUEUE activePromises;
	bool binary = 0;
	class SSH2_RemoteListen* remoteListen;
	struct html5_web_socket* wsPipe;
	/*
	* eof Event handler
	*/
	void( *internal_eofCallback )( SSH2_Channel*, struct html5_web_socket* wsPipe );
	/*
	* close Event handler
	*/
	void( *internal_closeCallback )( SSH2_Channel*, struct html5_web_socket* wsPipe );
public:
	SSH2_Channel();
	~SSH2_Channel();


	static void Error( uintptr_t psv, int errcode, const char* string, int errlen );
	// interfaces with sack_ssh to call the data callback
	static void DataCallback( uintptr_t psv, int stream, const uint8_t* data, size_t length ); 
	// interfaces with sack_ssh to call the close callback
	static void CloseCallback( uintptr_t psv ); 
	static void PtyCallback( uintptr_t psv, LOGICAL success );
	static void ShellCallback( uintptr_t psv, LOGICAL success );
	static void ExecCallback( uintptr_t psv, LOGICAL success );
	static void SetEnvCallback( uintptr_t psv, LOGICAL success );

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	/*
	* Set Read Callback
	*/
	static void Read( const v8::FunctionCallbackInfo<Value>& args );
	static void setBinaryRead( const v8::FunctionCallbackInfo<Value>& args );
	static void getBinaryRead( const v8::FunctionCallbackInfo<Value>& args );
	/*
	* Send data to the channel
	*/
	static void Send( const v8::FunctionCallbackInfo<Value>& args );
	/*
	* Set Close Callback
	*/
	static void Close( const v8::FunctionCallbackInfo<Value>& args );
	/*
	* Set Remote environment variable
	*/
	static void SetEnv( const v8::FunctionCallbackInfo<Value>& args );
	/*
	* Request a pseudo terminal
	* returns a promise that resolves to a boolean
	*/
	static void OpenPTY( const v8::FunctionCallbackInfo<Value>& args );
	/*
	* Request a shell
	* returns a promise that resolves to a boolean
	*/
	static void Shell( const v8::FunctionCallbackInfo<Value>& args );
	/*
	* Request a command
	* returns a promise that resolves to a boolean
	*/
	static void Exec( const v8::FunctionCallbackInfo<Value>& args );

};


class SSH2_Object : public node::ObjectWrap {

public:
	PLINKQUEUE eventQueue = CreateLinkQueue();
	Persistent<Object> jsObject;
	struct ssh_session* session = NULL;
	bool ivm_hosted;
	class constructorSet *c;
	uv_async_t async = {}; // keep this instance around for as long as we might need to do the periodic callback
	bool binary = 0;
	uint8_t fingerprintData[20];
	Persistent<Object> connectOptions;
	Persistent<Promise::Resolver> connectPromise;
	Persistent<Promise::Resolver> loginPromise;
	Persistent<Promise::Resolver> channelPromise;
	Persistent<Promise::Resolver> forwardPromise;

	Persistent<Promise::Resolver> *activePromise;


public:
	SSH2_Object( Isolate* isolate );
	~SSH2_Object();

	static void Init( Isolate* isolate, Local<Object> exports );
	static void New( const v8::FunctionCallbackInfo<Value>& args );
	/*
	* Getter for connection fingerprint
	*/
	static void fingerprint( const v8::FunctionCallbackInfo<Value>& args );
	/*
	* Connect to a server
	*/
	static void Connect( const v8::FunctionCallbackInfo<Value>& args );
	static void Login( const v8::FunctionCallbackInfo<Value>& args );
	// forward socket connection to remote
	static void Forward( const v8::FunctionCallbackInfo<Value>& args );
	// reverse socket connectio from remote
	static void Reverse( const v8::FunctionCallbackInfo<Value>& args );
	/*
	* opens a new channel - channelOpen callback is triggered on completion
	* returns a promise that resolves to a channel object
	*/
	static void OpenChannel( const v8::FunctionCallbackInfo<Value>& args );

	// utility callbacks for sack_ssh 
	static void handshook( uintptr_t psv, const uint8_t* string );
	static void authDone( uintptr_t psv, LOGICAL success );
	static void Error( uintptr_t psv, int errcode, const char* string, int errlen );
	/*
	* Opens a channel - returns a promise that reolves to a channel object
	*/
	static uintptr_t channelOpen( uintptr_t psv, ssh_channel* channel );
	/*
	* This is used to setup a remote socket that connects back to the local system
	*/
	static uintptr_t remoteForward( uintptr_t psv, ssh_channel* channel );
	/*
	* This is called when to forward a local port to a remote system
	*/
	static uintptr_t portForward( uintptr_t psv, ssh_channel* channel );
};


/*
* This is a remote listener object that listens for connections on the remote system
* and forwards them to the local system.
* This is used to create a reverse tunnel.
* Websocket module has ability to turn this into a websocket listener/server.
* sack.Websocket.ssh2ws( listener );
*/
class SSH2_RemoteListen : public node::ObjectWrap {

public:
	bool ivm_hosted;
	class constructorSet *c;
	SSH2_Object* ssh2;
	struct ssh_listener* listener;
	Persistent<Object> jsObject;  // the object that represents this channel
	//struct html5_web_socket* ws;
	class wssObject* wss;

public:
	SSH2_RemoteListen();
	~SSH2_RemoteListen();

	static void listenChannelOpen( uintptr_t psv, ssh_channel* channel );
	// allocate a new JS object
	static void New( const v8::FunctionCallbackInfo<Value>& args );
	// close the remote listener and listener object.
	static void Close( const v8::FunctionCallbackInfo<Value>& args );

	// Enable the remote listener to accept websocket and HTTP connections
	static void toWS( const v8::FunctionCallbackInfo<Value>& args );

	static void Error( uintptr_t psv, int errcode, const char* string, int errlen );

};

typedef struct SSH2_Event SSH2_EVENT;
#define MAXSSH2_EVENTSPERSET 32
DeclareSet( SSH2_EVENT );

struct SSH2_Global {
	SSH2_EVENTSET* eventSet;
	PLIST rootThreads;
};

#ifndef SSH2_MODULE_CPP
extern
#endif
struct SSH2_Global global;

#define getEvent() GetFromSet( SSH2_EVENT, &global.eventSet )
#define makeEvent(name) SSH2_EVENT* name = getEvent()
#define dropEvent(evt) DeleteFromSet( SSH2_EVENT, global.eventSet, evt )

#endif