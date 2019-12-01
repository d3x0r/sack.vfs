


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


class VoidObject : public node::ObjectWrap {

public:
	uintptr_t data;
	Persistent<Object> _this;
	//static v8::Persistent<v8::Function> constructor;   // generic void constructor
	//static v8::Persistent<v8::Function> constructor2;   // object color interface accessors
	VoidObject( uintptr_t data = 0 ); // inits to 0; if the value passed is 0

	static void Init( Isolate *isolate );
	static void New( const FunctionCallbackInfo<Value>& args );
	static void wrapSelf( Isolate* isolate, VoidObject *_this, Local<Object> into );
	static void releaseSelf( VoidObject *_this );
};

class RegistrationObject : public node::ObjectWrap{

public:
   CONTROL_REGISTRATION r;
   Persistent<Object> _this;

public:
	void InitRegistration( Isolate *isolate, struct registrationOpts *opts );
	RegistrationObject();
	RegistrationObject( Isolate *isolate, const char *caption );
	RegistrationObject( Isolate *isolate, struct registrationOpts *opts );

	static void NewRegistration( const FunctionCallbackInfo<Value>& args );
	static void New( const FunctionCallbackInfo<Value>& args );

	static void setCreate( const FunctionCallbackInfo<Value>& args );
	static void setDraw( const FunctionCallbackInfo<Value>& args );

	static void setMouse( const FunctionCallbackInfo<Value>& args );
	static void setKey( const FunctionCallbackInfo<Value>& args );
	static void setTouch( const FunctionCallbackInfo<Value>& args );

	static void setMove( const FunctionCallbackInfo<Value>& args );
	static void setSize( const FunctionCallbackInfo<Value>& args );

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


class PopupObject : public node::ObjectWrap {
public:
	PMENU popup;
	class ControlObject *parent;
	Persistent<Object> _this;
	class constructorSet* isolateCons;
	//static v8::Persistent<v8::Function> constructor;   // Popup
	uint32_t itemId;
	PLIST menuItems;
public:

	PopupObject();
	~PopupObject();

	static void wrapSelf( Isolate* isolate, PopupObject *_this, Local<Object> into );

	static void NewPopup( const FunctionCallbackInfo<Value>& args );
	static void addPopupItem( const FunctionCallbackInfo<Value>& args );
	static void trackPopup( const FunctionCallbackInfo<Value>& args );

};

class ListboxItemObject : public node::ObjectWrap {
public:
	PLISTITEM pli;
	int itemLevel;
	class ControlObject *control;
	Persistent<Object> _this;
	//static v8::Persistent<v8::Function> constructor;   // listbox item

	ListboxItemObject();
	~ListboxItemObject();

	static void New( const FunctionCallbackInfo<Value>& args );
	//static void Value( const FunctionCallbackInfo<Value>& args );
	static void Select( const FunctionCallbackInfo<Value>& args );
	static void removeListboxItem( const FunctionCallbackInfo<Value>&  args );

	static void wrapSelf( Isolate* isolate, ListboxItemObject *_this, Local<Object> into );

	static void addItem( const FunctionCallbackInfo<Value>& args );
	static void insertItem( const FunctionCallbackInfo<Value>& args );
	static void emptyBranch( const FunctionCallbackInfo<Value>& args );
	// on
	static void setEvents( const FunctionCallbackInfo<Value>& args );
	// accessors
	static void getText( const FunctionCallbackInfo<Value>&  args );
	static void setText( const FunctionCallbackInfo<Value>& args );
	static void getOpen( const FunctionCallbackInfo<Value>& args );
	static void setOpen( const FunctionCallbackInfo<Value>& args );
	Persistent<Function, CopyablePersistentTraits<Function>> cbOpened; // event callback        ()  // return true/false to allow creation
	//static Persistent<FunctionTemplate> listItemTemplate;
};

class MenuItemObject : public node::ObjectWrap {
public:
	PMENUITEM pmi;
	PopupObject *popup;
	uintptr_t uid;
	Persistent<Object> _this;
	//class constructorSet* isolateCons;
	MenuItemObject();
	~MenuItemObject();

	static void New( const FunctionCallbackInfo<Value>& args );
	static void Delete( const FunctionCallbackInfo<Value>& args );

	Persistent<Function, CopyablePersistentTraits<Function>> cbSelected; // event callback        ()  // return true/false to allow creation
	static void removeItem( const FunctionCallbackInfo<Value>&  args );

	static void wrapSelf( Isolate* isolate, MenuItemObject*_this, Local<Object> into );
};

class ControlObject : public node::ObjectWrap {

public:
	class constructorSet* isolateCons;
	ControlObject *frame;
	int done;
	int okay;
	PSI_CONTROL control; // this control
	PTHREAD waiter;
	//static v8::Persistent<v8::Function> constructor;   // Frame
	//static v8::Persistent<v8::Function> constructor2;  // Control
	//static v8::Persistent<v8::Function> registrationConstructor;  // Registration
	//static v8::Persistent<v8::FunctionTemplate> controlTemplate;
	//static v8::Persistent<v8::FunctionTemplate> frameTemplate;

	Persistent<Object> state;
	ImageObject *image;
	struct controlObjectFlags {
		unsigned tree : 1; // track state of listbox is tree-list
	} flags;
public:

	static void Init( Local<Object> exports );
	ControlObject( const char *caption, int w, int h, int x, int y, int borderFlags, ControlObject *parent );
	ControlObject( ControlObject *parent, const char *caption, const char *title, int w, int h, int x, int y );
	ControlObject( const char *type, ControlObject *parent, int32_t x, int32_t y, uint32_t w, uint32_t h );
	ControlObject( PSI_CONTROL control = NULL );
	~ControlObject();

	static void wrapSelf( Isolate* isolate, ControlObject *_this, Local<Object> into );
	static void releaseSelf( ControlObject *_this );


	static void New( const FunctionCallbackInfo<Value>& args );
	static void NewControl( const FunctionCallbackInfo<Value>& args );
	static Local<Object> NewWrappedControl( Isolate* isolate, PSI_CONTROL pc );
	static void createFrame( const FunctionCallbackInfo<Value>& args );
	static void createControl( const FunctionCallbackInfo<Value>& args );
	static void getRenderer( const FunctionCallbackInfo<Value>& args );

	static void registerControl( const FunctionCallbackInfo<Value>& args );

	static void on( const FunctionCallbackInfo<Value>& args );
	static void get( const FunctionCallbackInfo<Value>& args );
	static void focus( const FunctionCallbackInfo<Value>& args );
	static void show( const FunctionCallbackInfo<Value>& args );
	static void edit( const FunctionCallbackInfo<Value>& args );
	static void save( const FunctionCallbackInfo<Value>& args );
	static void load( const FunctionCallbackInfo<Value>& args );

	static void getControlFont( const FunctionCallbackInfo<Value>& args );
	static void setControlFont( const FunctionCallbackInfo<Value>& args );

	

	static void close( const FunctionCallbackInfo<Value>& args );
	static void hide( const FunctionCallbackInfo<Value>& args );
	static void reveal( const FunctionCallbackInfo<Value>& args );
	static void redraw( const FunctionCallbackInfo<Value>& args );

	static void addSheetsPage( const FunctionCallbackInfo<Value>& args );

	static void getFrameBorder( const FunctionCallbackInfo<Value>& args );
	static void setFrameBorder( const FunctionCallbackInfo<Value>& args );

	static void getControlColor( const FunctionCallbackInfo<Value>& args );
	static void setControlColor( const FunctionCallbackInfo<Value>& args );
	static void getControlColor2( const FunctionCallbackInfo<Value>& args );
	static void setControlColor2( const FunctionCallbackInfo<Value>& args );

	static void getControlText( const FunctionCallbackInfo<Value>& args );
	static void setControlText( const FunctionCallbackInfo<Value>& args );
	static void getCoordinate( const FunctionCallbackInfo<Value>& args );
	static void setCoordinate( const FunctionCallbackInfo<Value>& args );

	// progress bar methods
	static void setProgressBarRange( const FunctionCallbackInfo<Value>& args );
	static void setProgressBarProgress( const FunctionCallbackInfo<Value>& args );
	static void setProgressBarColors( const FunctionCallbackInfo<Value>& args );
	static void setProgressBarTextEnable( const FunctionCallbackInfo<Value>& args );

	// console methods
	static void writeConsole( const FunctionCallbackInfo<Value>& args );
	static void setConsoleRead( const FunctionCallbackInfo<Value>& args );

	// button methods
	static void setButtonClick( const FunctionCallbackInfo<Value>& args );
	static void setButtonEvent( const FunctionCallbackInfo<Value>& args );

	// clock control extension
	static void makeAnalog( const FunctionCallbackInfo<Value>& args );
	//Persistent<Function, CopyablePersistentTraits<Function>> cbConsoleRead;  // event for console control callback (psi/console.h)
	//Persistent<Function, CopyablePersistentTraits<Function>> cbButtonClick;  // event for button control callback (psi/console.h)

	// a chunk of events that are general place holders for events from controls
	Persistent<Function, CopyablePersistentTraits<Function>> customEvents[5];  // event for button control callback (psi/console.h)

	static void getListboxIsTree( const FunctionCallbackInfo<Value>& args );
	static void setListboxIsTree( const FunctionCallbackInfo<Value> & args );

	static void setListboxTabs( const FunctionCallbackInfo<Value>& args );
	static void addListboxItem( const FunctionCallbackInfo<Value>&  args );
	static void resetListbox( const FunctionCallbackInfo<Value>& args );
	static void getListboxHeader( const FunctionCallbackInfo<Value>&  args );
	static void setListboxHeader( const FunctionCallbackInfo<Value>&  args );
	static void setListboxHScroll( const FunctionCallbackInfo<Value>&  args );
	static void measureListItem( const FunctionCallbackInfo<Value>&  args );
	static void listboxDisableUpdate( const FunctionCallbackInfo<Value>& args );

	static void setListboxOnDouble( const FunctionCallbackInfo<Value>&  args );
	static void setListboxOnSelect( const FunctionCallbackInfo<Value>&  args );
#define listboxOnDouble customEvents[0]
#define listboxOnSelect customEvents[1]

#define cbFrameEventOkay customEvents[0]
#define cbFrameEventCancel customEvents[1]
#define cbFrameEventAbort customEvents[2]


	static void getConsoleEcho( const FunctionCallbackInfo<Value>&  args );
	static void setConsoleEcho( const FunctionCallbackInfo<Value>&  args );
	static void getPassword( const FunctionCallbackInfo<Value>&  args );
	static void setPassword( const FunctionCallbackInfo<Value>&  args );

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



