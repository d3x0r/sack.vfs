


struct registrationOpts {
	const char *name;
	int width, height;
	int default_border;
	//1) Expose a function in the addon to allow Node to set the Javascript cb that will be periodically called back to :
	Local<Function> cbInitEvent; // event callback        ()  // return true/false to allow creation
	Local<Function> cbLoadEvent; // event callback       ( PTEXT parameters )
	Local<Function> cbDrawEvent; // event callback       ()  // return true/false if handled
	Local<Function> cbMouseEvent; // event callback      (int32,int32,uint32) // return true/false if handled
	Local<Function> cbKeyEvent; // event callback        (uint32)  // return true/false if handled
	Local<Function> cbDestroyEvent; // event callback    ()
	Local<Function> cbPropPageEvent; // event callback   () return PSI_CONTROL frame
	Local<Function> cbApplyPropEvent; // event callback  (frame)
	Local<Function> cbSaveEvent; // event callback       ( pvt )
	Local<Function> cbAddedEvent; // event callback     ( added Control )
	Local<Function> cbCaptionEvent; // event callback  ()
	Local<Function> cbFocusEvent; // event callback    (focused/notFocused)
	Local<Function> cbPositionEvent; // event callback  (start/end)

	Local<Function> cbHideEvent; // event callback         ()
	Local<Function> cbRevealEvent; // event callback       ()
	Local<Function> cbDrawCaptionEvent; // event callback  (image) // given the image to draw into
	Local<Function> cbDrawDecorationEvent; // event callback ()
	Local<Function> cbMoveEvent; // event callback   (start/end of move)
	Local<Function> cbSizeEvent; // event callback  (start/end of sizing)
	Local<Function> cbScaleEvent; // event callback   ()
	Local<Function> cbParentMoveEvent; // event callback        ( start/end of parent move)
	Local<Function> cbBeginEditEvent; // event callback   ()
	Local<Function> cbEndEditEvent; // event callback     ()
	Local<Function> cbReadPropEvent; // event callback     (prop Sheet)
	Local<Function> cbAbortPropEvent; // event callback   (prop sheet)
	Local<Function> cbDonePropEvent; // event callback    ()
	Local<Function> cbTouchEvent; // event callback ( [touches] )
	Local<Function> cbBorderDrawEvent; // event callback   (image)
	Local<Function> cbBorderMeasureEvent; // event callback  (int*,int*,int*,int*)
	Local<Function> cbDropAcceptEvent; // event callback  (filePath,x,y)
	Local<Function> cbRolloverEvent; // event callback  (true/false - enter/leave)

};



class RegistrationObject : public node::ObjectWrap{

public:
   CONTROL_REGISTRATION r;
   Persistent<Object> _this;

public:
	void InitRegistration( Isolate *isolate, struct registrationOpts *opts );
	RegistrationObject( Isolate *isolate, const char *caption );
	RegistrationObject( Isolate *isolate, struct registrationOpts *opts );

	static void NewRegistration( const FunctionCallbackInfo<Value>& args );
	static void New( const FunctionCallbackInfo<Value>& args );

	static void setCreate( const FunctionCallbackInfo<Value>& args );
	static void setDraw( const FunctionCallbackInfo<Value>& args );

	static void setMouse( const FunctionCallbackInfo<Value>& args );
	static void setKey( const FunctionCallbackInfo<Value>& args );
	static void setTouch( const FunctionCallbackInfo<Value>& args );

   ~RegistrationObject();


	//1) Expose a function in the addon to allow Node to set the Javascript cb that will be periodically called back to :
	Persistent<Function, CopyablePersistentTraits<Function>> cbInitEvent; // event callback        ()  // return true/false to allow creation
	Persistent<Function, CopyablePersistentTraits<Function>> cbLoadEvent; // event callback       ( PTEXT parameters )
	Persistent<Function, CopyablePersistentTraits<Function>> cbDrawEvent; // event callback       ()  // return true/false if handled
	Persistent<Function, CopyablePersistentTraits<Function>> cbMouseEvent; // event callback      (int32,int32,uint32) // return true/false if handled
	Persistent<Function, CopyablePersistentTraits<Function>> cbKeyEvent; // event callback        (uint32)  // return true/false if handled
	Persistent<Function, CopyablePersistentTraits<Function>> cbDestroyEvent; // event callback    ()
	Persistent<Function, CopyablePersistentTraits<Function>> cbPropPageEvent; // event callback   () return PSI_CONTROL frame
	Persistent<Function, CopyablePersistentTraits<Function>> cbApplyPropEvent; // event callback  (frame)
	Persistent<Function, CopyablePersistentTraits<Function>> cbSaveEvent; // event callback       ( pvt )
	Persistent<Function, CopyablePersistentTraits<Function>> cbAddedEvent; // event callback     ( added Control )
	Persistent<Function, CopyablePersistentTraits<Function>> cbCaptionEvent; // event callback  ()
	Persistent<Function, CopyablePersistentTraits<Function>> cbFocusEvent; // event callback    (focused/notFocused)
	Persistent<Function, CopyablePersistentTraits<Function>> cbPositionEvent; // event callback  (start/end)

	Persistent<Function, CopyablePersistentTraits<Function>> cbHideEvent; // event callback         ()
	Persistent<Function, CopyablePersistentTraits<Function>> cbRevealEvent; // event callback       ()
	Persistent<Function, CopyablePersistentTraits<Function>> cbDrawCaptionEvent; // event callback  (image) // given the image to draw into
	Persistent<Function, CopyablePersistentTraits<Function>> cbDrawDecorationEvent; // event callback ()
	Persistent<Function, CopyablePersistentTraits<Function>> cbMoveEvent; // event callback   (start/end of move)
	Persistent<Function, CopyablePersistentTraits<Function>> cbSizeEvent; // event callback  (start/end of sizing)
	Persistent<Function, CopyablePersistentTraits<Function>> cbScaleEvent; // event callback   ()
	Persistent<Function, CopyablePersistentTraits<Function>> cbParentMoveEvent; // event callback        ( start/end of parent move)
	Persistent<Function, CopyablePersistentTraits<Function>> cbBeginEditEvent; // event callback   ()
	Persistent<Function, CopyablePersistentTraits<Function>> cbEndEditEvent; // event callback     ()
	Persistent<Function, CopyablePersistentTraits<Function>> cbReadPropEvent; // event callback     (prop Sheet)
	Persistent<Function, CopyablePersistentTraits<Function>> cbAbortPropEvent; // event callback   (prop sheet)
	Persistent<Function, CopyablePersistentTraits<Function>> cbDonePropEvent; // event callback    ()
	Persistent<Function, CopyablePersistentTraits<Function>> cbTouchEvent; // event callback ( [touches] )
	Persistent<Function, CopyablePersistentTraits<Function>> cbBorderDrawEvent; // event callback   (image)
	Persistent<Function, CopyablePersistentTraits<Function>> cbBorderMeasureEvent; // event callback  (int*,int*,int*,int*)
	Persistent<Function, CopyablePersistentTraits<Function>> cbDropAcceptEvent; // event callback  (filePath,x,y)
	Persistent<Function, CopyablePersistentTraits<Function>> cbRolloverEvent; // event callback  (true/false - enter/leave)

};



class ControlObject : public node::ObjectWrap {

public:
	ControlObject *frame;
	PSI_CONTROL control; // this control
	static v8::Persistent<v8::Function> constructorDisplay;   // Borderless, 'fullscreen' display
	static v8::Persistent<v8::Function> constructor;   // Frame
	static v8::Persistent<v8::Function> constructor2;  // Control
	static v8::Persistent<v8::Function> constructor3;  // Control
	static v8::Persistent<v8::FunctionTemplate> controlTemplate;

	Persistent<Object> state;
	ImageObject *image;
public:

	static void Init( Handle<Object> exports );
	ControlObject( const char *caption, int w, int h, int x, int y, int borderFlags, ControlObject *parent );
	ControlObject( ControlObject *parent, const char *caption, const char *title, int w, int h, int x, int y );
	ControlObject( const char *type, ControlObject *parent, int32_t x, int32_t y, uint32_t w, uint32_t h );
	ControlObject();
	~ControlObject();

	static void wrapSelf( Isolate* isolate, ControlObject *_this, Local<Object> into );
	static void ControlObject::releaseSelf( ControlObject *_this );

	static void New( const FunctionCallbackInfo<Value>& args );
	static void NewDisplay( const FunctionCallbackInfo<Value>& args );
	static void NewControl( const FunctionCallbackInfo<Value>& args );
	static Local<Object> ControlObject::NewWrappedControl( Isolate* isolate, PSI_CONTROL pc );
	static void createFrame( const FunctionCallbackInfo<Value>& args );
	static void createControl( const FunctionCallbackInfo<Value>& args );

	static void registerControl( const FunctionCallbackInfo<Value>& args );

	static void show( const FunctionCallbackInfo<Value>& args );

	static void hide( const FunctionCallbackInfo<Value>& args );
	static void reveal( const FunctionCallbackInfo<Value>& args );
	static void redraw( const FunctionCallbackInfo<Value>& args );

	static void writeConsole( const FunctionCallbackInfo<Value>& args );
	static void setConsoleRead( const FunctionCallbackInfo<Value>& args );

	static void getControlColor( v8::Local<v8::Name> field,
		const PropertyCallbackInfo<v8::Value>& args );
	static void setControlColor( v8::Local<v8::Name> field,
		Local<Value> value,
		const PropertyCallbackInfo<void>& args );


	Persistent<Function, CopyablePersistentTraits<Function>> cbConsoleRead;  // event for console control callback (psi/console.h)



																			 //1) Expose a function in the addon to allow Node to set the Javascript cb that will be periodically called back to :
	Persistent<Function, CopyablePersistentTraits<Function>> cbInitEvent; // event callback        ()  // return true/false to allow creation
	Persistent<Function, CopyablePersistentTraits<Function>> cbLoadEvent; // event callback       ( PTEXT parameters )
	Persistent<Function, CopyablePersistentTraits<Function>> cbDrawEvent; // event callback       ()  // return true/false if handled
	Persistent<Function, CopyablePersistentTraits<Function>> cbMouseEvent; // event callback      (int32,int32,uint32) // return true/false if handled
	Persistent<Function, CopyablePersistentTraits<Function>> cbKeyEvent; // event callback        (uint32)  // return true/false if handled
	Persistent<Function, CopyablePersistentTraits<Function>> cbDestroyEvent; // event callback    ()
	Persistent<Function, CopyablePersistentTraits<Function>> cbPropPageEvent; // event callback   () return PSI_CONTROL frame
	Persistent<Function, CopyablePersistentTraits<Function>> cbApplyPropEvent; // event callback  (frame)
	Persistent<Function, CopyablePersistentTraits<Function>> cbSaveEvent; // event callback       ( pvt )
	Persistent<Function, CopyablePersistentTraits<Function>> cbAddedEvent; // event callback     ( added Control )
	Persistent<Function, CopyablePersistentTraits<Function>> cbCaptionEvent; // event callback  ()
	Persistent<Function, CopyablePersistentTraits<Function>> cbFocusEvent; // event callback    (focused/notFocused)
	Persistent<Function, CopyablePersistentTraits<Function>> cbPositionEvent; // event callback  (start/end)

	Persistent<Function, CopyablePersistentTraits<Function>> cbHideEvent; // event callback         ()
	Persistent<Function, CopyablePersistentTraits<Function>> cbRevealEvent; // event callback       ()
	Persistent<Function, CopyablePersistentTraits<Function>> cbDrawCaptionEvent; // event callback  (image) // given the image to draw into
	Persistent<Function, CopyablePersistentTraits<Function>> cbDrawDecorationEvent; // event callback ()
	Persistent<Function, CopyablePersistentTraits<Function>> cbMoveEvent; // event callback   (start/end of move)
	Persistent<Function, CopyablePersistentTraits<Function>> cbSizeEvent; // event callback  (start/end of sizing)
	Persistent<Function, CopyablePersistentTraits<Function>> cbScaleEvent; // event callback   ()
	Persistent<Function, CopyablePersistentTraits<Function>> cbParentMoveEvent; // event callback        ( start/end of parent move)
	Persistent<Function, CopyablePersistentTraits<Function>> cbBeginEditEvent; // event callback   ()
	Persistent<Function, CopyablePersistentTraits<Function>> cbEndEditEvent; // event callback     ()
	Persistent<Function, CopyablePersistentTraits<Function>> cbReadPropEvent; // event callback     (prop Sheet)
	Persistent<Function, CopyablePersistentTraits<Function>> cbAbortPropEvent; // event callback   (prop sheet)
	Persistent<Function, CopyablePersistentTraits<Function>> cbDonePropEvent; // event callback    ()
	Persistent<Function, CopyablePersistentTraits<Function>> cbTouchEvent; // event callback ( [touches] )
	Persistent<Function, CopyablePersistentTraits<Function>> cbBorderDrawEvent; // event callback   (image)
	Persistent<Function, CopyablePersistentTraits<Function>> cbBorderMeasureEvent; // event callback  (int*,int*,int*,int*)
	Persistent<Function, CopyablePersistentTraits<Function>> cbDropAcceptEvent; // event callback  (filePath,x,y)
	Persistent<Function, CopyablePersistentTraits<Function>> cbRolloverEvent; // event callback  (true/false - enter/leave)

	RegistrationObject *registration;
};


