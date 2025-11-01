


class VulkanCommandObject : public node::ObjectWrap {
 public:
	VulkanCommandObject();
	static void New( const FunctionCallbackInfo<Value> &args );
	// begin (re)recording commands
	static void record( const FunctionCallbackInfo<Value> &args );
	// play recorded commands
	static void play( const FunctionCallbackInfo<Value> &args );
};

class VulkanBufferObject : public node::ObjectWrap {
 public:
	int dirty; // if true, then the buffer needs to be updated
	VulkanBufferObject();
	static void New( const FunctionCallbackInfo<Value> &args );

	// set content of an array buffer to transmit
	static void bufferGetter( const FunctionCallbackInfo<Value> &args );
};

class VulkanShaderObject : public node::ObjectWrap {
 public:

	VulkanShaderObject();
	static void New( const FunctionCallbackInfo<Value> &args );
};

class VulkanObject : public node::ObjectWrap {

public:
	PRENDERER r; // this control
	int drawn;
	int closed;
	Persistent<Object> surface; // used to pass to draw callback

	static v8::Persistent<v8::Function> constructor;

public:

	static void Init( Isolate* isolate, Local<Object> exports );
	VulkanObject();

	static void New( const FunctionCallbackInfo<Value>& args );

	// a camera has a render surface, aspect, width and height, position
	// probably since this is a visible object it can be placed on the display with 
	// a percentage?
	static void addCamera( const FunctionCallbackInfo<Value> &args );

	static void addCommand( const FunctionCallbackInfo<Value> &args );



	static void getFrameBuffer( const FunctionCallbackInfo<Value>& args );
	//static void getFrameBuffer( const FunctionCallbackInfo<Value>& args );

	~VulkanObject();

	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback

					  //1) Expose a function in the addon to allow Node to set the Javascript cb that will be periodically called back to :
	PERSISTENT_FUNCTION cbInitEvent; // event callback        ()  // return true/false to allow creation
	PERSISTENT_FUNCTION cbMouse; // event callback        ()  // return true/false to allow creation
	PERSISTENT_FUNCTION cbKey; // event callback        ()  // return true/false to allow creation
	PERSISTENT_FUNCTION cbDraw; // event callback        ()  // return true/false to allow creation


	PLINKQUEUE receive_queue;


};


