

enum SSH2_EventCodes {
	SSH2_EVENT_CLOSE,  // closes event handle
	SSH2_EVENT_HANDSHAKE, // handshake complete
	SSH2_EVENT_AUTHDONE, // authentication complete
	SSH2_EVENT_CONNECTED, // connected to server
	SSH2_EVENT_CHANNEL, // channel opened
	SSH2_EVENT_DATA, // data received
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

public:
	SSH2_Channel();
	~SSH2_Channel();


	// interfaces with sack_ssh to call the data callback
	static void DataCallback( uintptr_t psv, int stream, const uint8_t* data, size_t length ); 
	// interfaces with sack_ssh to call the close callback
	static void CloseCallback( uintptr_t psv ); 

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	/*
	* Set Read Callback
	*/
	static void Read( const v8::FunctionCallbackInfo<Value>& args );
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
	uv_async_t async = {}; // keep this instance around for as long as we might need to do the periodic callback
	bool binary = 0;
	Persistent<Promise::Resolver> connectPromise;

	Persistent<Promise::Resolver> channelPromise;


public:
	SSH2_Object( Isolate* isolate );
	~SSH2_Object();

	static void Init( Isolate* isolate, Local<Object> exports );
	static void New( const v8::FunctionCallbackInfo<Value>& args );
	/*
	* Connect to a server
	*/
	static void Connect( const v8::FunctionCallbackInfo<Value>& args );
	/*
	* opens a new channel - channelOpen callback is triggered on completion
	* returns a promise that resolves to a channel object
	*/
	static void OpenChannel( const v8::FunctionCallbackInfo<Value>& args );

	// utility callbacks for sack_ssh 
	static void handshook( uintptr_t psv, const uint8_t* string );
	static void authDone( uintptr_t psv, LOGICAL success );
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



typedef struct SSH2_Event SSH2_EVENT;
#define MAXSSH2_EVENTSPERSET 32
DeclareSet( SSH2_EVENT );

struct SSH2_Global {
	SSH2_EVENTSET* eventSet;
};

static struct SSH2_Global global;

#define getEvent() GetFromSet( SSH2_EVENT, global.eventSet )
#define makeEvent(name) SSH2_EVENT* name = getEvent()
#define dropEvent(evt) DeleteFromSet( SSH2_EVENT, global.eventSet, evt )
