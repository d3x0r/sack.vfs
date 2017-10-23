
#undef plot

class ImageObject : public node::ObjectWrap{

public:
	ImageObject *container;
	Image image; // this control
	LOGICAL external;
	static v8::Persistent<v8::Function> constructor;

	Persistent<Object> _this;
	int jpegQuality;
public:

	static void Init( Handle<Object> exports );
	ImageObject( int w, int h, int x, int y, ImageObject *parent );
	ImageObject( const char *filename );
	ImageObject( Image image );
	ImageObject( uint8_t *buf, size_t len );

	static void New( const FunctionCallbackInfo<Value>& args );
	static void NewSubImage( const FunctionCallbackInfo<Value>& args );
	//static Persistent<Object>  NewImage( Isolate *isolate, Image image );
	static Local<Object> NewImage( Isolate *isolate, Image image, LOGICAL external );
	static ImageObject * MakeNewImage( Isolate*isolate, Image image, LOGICAL external );
	
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
	
	static void getPng( v8::Local<v8::String> field,
		const PropertyCallbackInfo<v8::Value>& args );
	static void getJpeg( v8::Local<v8::String> field,
		const PropertyCallbackInfo<v8::Value>& args );
	static void getJpegQuality( v8::Local<v8::String> field,
		const PropertyCallbackInfo<v8::Value>& args );
	static void setJpegQuality( v8::Local<v8::String> field,
		Local<Value> val,
		const PropertyCallbackInfo<void>& args );

	static void getWidth( v8::Local<v8::String> field,
		const PropertyCallbackInfo<v8::Value>& args );
	static void getHeight( v8::Local<v8::String> field,
		const PropertyCallbackInfo<v8::Value>& args );
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
	static v8::Persistent<v8::FunctionTemplate> tpl;

public:

	static void Init( Handle<Object> exports );
	ColorObject( int r, int g, int b, int a );
	ColorObject( CDATA rgba );
	ColorObject();
	static CDATA getColor( Local<Object> object );
	static Local<Object> makeColor( Isolate *isolate, CDATA rgba );
	static void New( const FunctionCallbackInfo<Value>& args );
	static bool isColor( Isolate *isolate, Local<Object> object );

	~ColorObject();


};

