
#undef plot

class ImageObject : public node::ObjectWrap{

public:
	ImageObject *container;
	Image image; // this control
	LOGICAL external;
	static v8::Persistent<v8::Function> constructor;

public:

	static void Init( Handle<Object> exports );
	ImageObject( int w, int h, int x, int y, ImageObject *parent );
	ImageObject( const char *filename );
	ImageObject( Image image );

	static void New( const FunctionCallbackInfo<Value>& args );
	static void NewSubImage( const FunctionCallbackInfo<Value>& args );
	//static Persistent<Object>  NewImage( Isolate *isolate, Image image );
	static Local<Object> NewImage( Isolate *isolate, Image image, LOGICAL external );

	
	static void reset( const FunctionCallbackInfo<Value>& args );
	static void fill( const FunctionCallbackInfo<Value>& args );
	static void fillOver( const FunctionCallbackInfo<Value>& args );
	static void plot( const FunctionCallbackInfo<Value>& args );
	static void plotOver( const FunctionCallbackInfo<Value>& args );
	static void line( const FunctionCallbackInfo<Value>& args );
	static void lineOver( const FunctionCallbackInfo<Value>& args );
	static void putImage( const FunctionCallbackInfo<Value>& args );
	static void putImageOver( const FunctionCallbackInfo<Value>& args );
	static void imageData( const FunctionCallbackInfo<Value>& args );
	


   ~ImageObject();


};



class FontObject : public node::ObjectWrap{

public:
   FontObject *container;
	SFTFont font; // this control

	static v8::Persistent<v8::Function> constructor;

public:

	static void Init( Handle<Object> exports );
   FontObject( const char *filename, int w, int h, int flags );

	static void New( const FunctionCallbackInfo<Value>& args );
	
	static void measure( const FunctionCallbackInfo<Value>& args );



   ~FontObject();


};



class ColorObject : public node::ObjectWrap {

public:
	CDATA color;

	static v8::Persistent<v8::Function> constructor;

public:

	static void Init( Handle<Object> exports );
	ColorObject( int r, int g, int b, int a );
	ColorObject( CDATA rgba );

	static void New( const FunctionCallbackInfo<Value>& args );


	~ColorObject();


};

