
class is_control : public node::ObjectWrap {
public:
	class InterShellObject *type;
	char *caption;
	LOGICAL isButton;
	PSI_CONTROL pc;

	struct menu_button *button;
	Persistent<Object> self;
	Persistent<Object> surface; // used to pass to draw callback

	Persistent<Object> psvControl;
	// return value from javascript - javascript state object.
	Persistent<Object> psvData;

	static void NewControlInstance( const FunctionCallbackInfo<Value>& args );

};

class InterShellObject : public node::ObjectWrap {
public:
	//static v8::Persistent<v8::Function> buttonConstructor;
	//static v8::Persistent<v8::Function> buttonInstanceConstructor;
	//static v8::Persistent<v8::Function> controlConstructor;
	//static v8::Persistent<v8::Function> controlInstanceConstructor;
	//static v8::Persistent<v8::Function> customControlConstructor;
	//static v8::Persistent<v8::Function> customControlInstanceConstructor;
	//static v8::Persistent<v8::Function> intershellConstructor;
	//static v8::Persistent<v8::Function> configConstructor;

	char *name;
	PLINKQUEUE events;
	uv_async_t async;

	class RegistrationObject *registration;

	LOGICAL bCustom;

	Persistent<Object> self;
	Persistent<Object> registrationObject;

	Persistent<Function, CopyablePersistentTraits<Function>> cbClick; // event callback        ()  // return true/false to allow creation
	Persistent<Function, CopyablePersistentTraits<Function>> cbCreate; // event callback        ()  // return true/false to allow creation
	Persistent<Function, CopyablePersistentTraits<Function>> cbDestroy; // event callback        ()  // return true/false to allow creation
	Persistent<Function, CopyablePersistentTraits<Function>> cbDraw; // event callback        ()  // return true/false to allow creation
	Persistent<Function, CopyablePersistentTraits<Function>> cbMouse; // event callback        ()  // return true/false to allow creation

public:

	static void Init( Local<Object> exports );
	InterShellObject( char *name, LOGICAL bControl );
	InterShellObject( );
	~InterShellObject();

	static void NewApplication( const FunctionCallbackInfo<Value>& args );
	static void NewButton( const FunctionCallbackInfo<Value>& args );
	static void NewControl( const FunctionCallbackInfo<Value>& args );
	static void NewCustomControl( const FunctionCallbackInfo<Value>& args );
	static void NewConfiguration( const FunctionCallbackInfo<Value>& args );

#define M(n)	static void n( const FunctionCallbackInfo<Value>& args ); // unload a DLL
 
	M( onCreateCustomControl )
	M( onCreateButton)
	M( onClickButton)
	M( onSaveButton)
	M( onLoadButton)

	M( onCreateControl )
	M( onSaveControl )
	M( onLoadControl )


	static void setTitle( const FunctionCallbackInfo<Value>& args );
	static void setStyle( const FunctionCallbackInfo<Value>& args );
	static void setTextColor( const FunctionCallbackInfo<Value>& args );
	static void setBackground( const FunctionCallbackInfo<Value>& args );
	static void setSecondary( const FunctionCallbackInfo<Value>& args );

	static void addConfigMethod( const FunctionCallbackInfo<Value>& args );


};

