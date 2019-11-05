




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
	
	static void getFrameBuffer( const FunctionCallbackInfo<Value>& args );
	//static void getFrameBuffer( const FunctionCallbackInfo<Value>& args );

	~VulkanObject();

	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback

					  //1) Expose a function in the addon to allow Node to set the Javascript cb that will be periodically called back to :
	Persistent<Function, CopyablePersistentTraits<Function>> cbInitEvent; // event callback        ()  // return true/false to allow creation
	Persistent<Function, CopyablePersistentTraits<Function>> cbMouse; // event callback        ()  // return true/false to allow creation
	Persistent<Function, CopyablePersistentTraits<Function>> cbKey; // event callback        ()  // return true/false to allow creation
	Persistent<Function, CopyablePersistentTraits<Function>> cbDraw; // event callback        ()  // return true/false to allow creation


	PLINKQUEUE receive_queue;


};


