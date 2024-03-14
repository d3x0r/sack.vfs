
#ifndef WEBSOCKET_MODULE_H
#define WEBSOCKET_MODULE_H

/*
* web sock server Object
* Is also an accepted client at the socket level, 
* but before negotiating between classic http and web socket
* IT is also possibily a pipe connected that was an accepted ssh socket(native socket was already supported as PCLIENT)
* it turns into a wssiObject when the handshake is successful, which has a reference back to the accepted socket.
* 
* This object is shared with ssh2_module.cc, because it gets the accepted socket, and then hands it off to the wssiObject, which the event callbacks
* are dispatched in the C layer are handled for the wssiObject.
*/

class wssObject : public node::ObjectWrap {
	LOGICAL closed;
public:
	bool resolveAddr;
	bool resolveMac;
	
	PCLIENT pc;  // pc and wsPipe should be a union. usage is mutuaully exclusive
	/* but then I would need a flag anyway to indiciate which.
	* But then 
	*/
	struct html5_web_socket* wsPipe;

	Persistent<Object> _this;
	PLINKQUEUE eventQueue;
	int last_count_handled;
	int closing;
	PTHREAD event_waker;
	PLIST opening;
	PLIST requests;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback

	/* 
	* The following are the holders for event callbacks.
	* These are the callbacks that are called when the event occurs.
	*/
	Persistent<Function, CopyablePersistentTraits<Function>> openCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> acceptCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> requestCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> closeCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> errorCloseCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> errorLowCallback; //
	/*
	* this is the general async callback event being served
	* it somces from 
	*/
	struct wssEvent* eventMessage;
	bool ssl;
	enum wsReadyStates readyState;
	bool immediateEvent;
	
	Isolate* isolate;  // which thread this is in
	struct wssOptions* opts; // the options passed to the constructor
public:

	/**
	* constructor for the WebSocket Server Object (wssObject)
	* options are parsed from the JS object and passed to the C++ object in New()
	*/
	wssObject( struct wssOptions* opts );

	/**
	* used internally as the constructor function fo the JS object
	* @param {Object} options - the options for the websocket server
	* @return {wssObject} - the websocket server object
	*/
	static void New( const v8::FunctionCallbackInfo<Value>& args );

	/**
	* issues close to websocket, provides (code, reason) for close
	* @param {number} code - the close code
	* @param {string} reason - the reason for close
	*/
	static void close( const v8::FunctionCallbackInfo<Value>& args );
	/** 
	* sets events by name, and the callback function
	* event names are 'open', 'accept', 'request', 'close', 'error', 'errorLow'
	* @param {string} event - the event name
	* @param {function} callback - the callback function
	*/
	static void on( const v8::FunctionCallbackInfo<Value>& args );
	/*
	* in low error, can disable SSL, which will cause the HTTP handler
	* to attempt with the original data that was not SSL handshake request.
	* 
	*  @param {boolean} disable - true to disable SSL(maybe is is always true), false to enable SSL
	*/
	static void disableSSL( const FunctionCallbackInfo<Value>& args );

	// this layer doesn't get 'messages' - this layer handles handshake
	//static void onMessage( const v8::FunctionCallbackInfo<Value>& args );

	/**
	* Websocket server event - the socket has been accepted, and this allows
	* assignment of events to the new socket.
	* @arg {function} callback - the callback function
	*/
	static void onConnect( const v8::FunctionCallbackInfo<Value>& args );

	/**
	* Websocket server event, the socket has been accepted, and this allows
	* cancelling the opening of the socket.
	* @arg {function} callback - the callback function
	*/
	static void onAccept( const v8::FunctionCallbackInfo<Value>& args );

	/**
	* This is an HTTP request event callback; was not a handshake for websocket
	* @arg {function} callback - the callback function
	*/
	static void onRequest( const v8::FunctionCallbackInfo<Value>& args );

	/*
	* This is websocket client/server onclose event callback
	* This is the only event that is guaranteed to be called.
	* @arg {function} callback - the callback function
	*/
	static void onClose( const v8::FunctionCallbackInfo<Value>& args );

	/*
	* a getter for the close function?  why not also accept/connect/request?
	* @return {function} callback - the callback function
	*/
	static void getOnClose( const v8::FunctionCallbackInfo<Value>& args );

	/*
	* This is websocket client/server onerror support - which there rarely is a websocket error. 
	* Most notifications come as onClose instead.
	* @arg {function} callback - the callback function
	*/
	static void onError( const v8::FunctionCallbackInfo<Value>& args );

	/**
	* this is a failure to negaotiate 
	* this is a low level error, before the (HTTP) handshake.
	* @arg {function} callback - the callback function
	*/
	static void onErrorLow( const v8::FunctionCallbackInfo<Value>& args ); 

	/**
	* This accepts the socket, used in onAccept.
	* no arguments
	*/
	static void accept( const v8::FunctionCallbackInfo<Value>& args );
	/**
	* This rejects the socket, used in onAccept.
	* no arguments
	*/
	static void reject( const v8::FunctionCallbackInfo<Value>& args );
	
	/**
	* This is a getter for the readyState property
	* @return {number} readyState - the ready state of the websocket
	*/
	static void getReadyState( const FunctionCallbackInfo<Value>& args );

	~wssObject();
};


#endif