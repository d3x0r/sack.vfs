#define DEFINE_GLOBAL
#include "../global.h"


void InitInterfaces( int opengl, int vulkan ) {
#if 0
	if( !g.pii ) {
		if( vulkan ) {
#ifdef _WIN32
			LoadFunction( "bag.image.vulkan.dll", NULL );
			LoadFunction( "bag.video.vulkan.dll", NULL );
#endif
#ifdef __LINUX__
			LoadFunction( "libbag.image.vulkan.so", NULL );
			LoadFunction( "libbag.video.vulkan.so", NULL );
#endif
			RegisterClassAlias(  "system/interfaces/vulkan.render"   , "system/interfaces/render" );
			RegisterClassAlias( "system/interfaces/vulkan.image"     ,"system/interfaces/image" );
			RegisterClassAlias( "system/interfaces/vulkan.render.3d" ,"system/interfaces/render.3d" );
			RegisterClassAlias( "system/interfaces/vulkan.image.3d"  ,"system/interfaces/image.3d" );
		} else {
#ifdef _WIN32
			LoadFunction( "bag.image.dll", NULL );
			LoadFunction( "bag.video.dll", NULL );
#endif
#ifdef __LINUX__
			LoadFunction( "libbag.image.so", NULL );
			LoadFunction( "libbag.video.so", NULL );
#endif
			RegisterClassAlias( "system/interfaces/sack.render", "system/interfaces/render" );
			RegisterClassAlias( "system/interfaces/sack.image", "system/interfaces/image" );
		}

	} else {
	}
#endif
}

/*
Persistent<Function> ImageObject::constructor;
Persistent<FunctionTemplate> ImageObject::tpl;
Persistent<Function> FontObject::constructor;
Persistent<Function> ColorObject::constructor;
Persistent<FunctionTemplate> ColorObject::tpl;
Persistent<Function> fontResult;
Persistent<Function> imageResult;
Persistent<Object> priorThis;
*/

static struct imageLocal {
	CDATA color;
	uv_async_t colorAsync; // keep this instance around for as long as we might need to do the periodic callback
	uv_async_t fontAsync; // keep this instance around for as long as we might need to do the periodic callback
	SFTFont fontResult;
}imageLocal;

static Local<Object> makeColor( Isolate *isolate, CDATA color ) {
	Local<Value> argv[1] = { Uint32::New( isolate, color ) };
	class constructorSet* c = getConstructors( isolate );
	Local<Function> cons = Local<Function>::New( isolate, c->ColorObject_constructor );
	Local<Object> cObject = cons->NewInstance( isolate->GetCurrentContext(), 1, argv ).ToLocalChecked();
	return cObject;
}

static void imageAsyncmsg( uv_async_t* handle ) {
	// Called by UV in main thread after our worker thread calls uv_async_send()
	//    I.e. it's safe to callback to the CB we defined in node!
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	Local<Context> context = isolate->GetCurrentContext();
	HandleScope scope( isolate );
	//lprintf( "async message notice. %p", myself );
	class constructorSet* c = getConstructors( isolate );
	if( !c->imageResult.IsEmpty() )
	{
		Local<Function> cb = Local<Function>::New( isolate, c->imageResult );
		Local<Object> _this = c->priorThis.Get( isolate );
		Local<Value> argv[1] = { ColorObject::makeColor( isolate, imageLocal.color ) };
		cb->Call( context, _this, 1, argv );
		uv_close( (uv_handle_t*)&imageLocal.colorAsync, NULL );
		c->imageResult.Reset();
	}
	if( !c->fontResult.IsEmpty() ) {
		Local<Function> cb = Local<Function>::New( isolate, c->fontResult );
		Local<Function> cons = Local<Function>::New( isolate, c->FontObject_constructor );
		Local<Object> result = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked();
		FontObject *fo = FontObject::Unwrap<FontObject>( result );
		FRACTION one = { 1,1 };
		fo->font = imageLocal.fontResult;

		Local<Value> argv[1] = { result };
		cb->Call( context, result, 1, argv );

		uv_close( (uv_handle_t*)&imageLocal.fontAsync, NULL );
		c->fontResult.Reset();
	}
	{
		class constructorSet* c = getConstructors( isolate );
		Local<Function>cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
		cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
	}
	//lprintf( "done calling message notice." );
}


static uintptr_t fontPickThread( PTHREAD thread ) {
	MemSet( &imageLocal.fontAsync, 0, sizeof( &imageLocal.fontAsync ) );
	uv_async_init( uv_default_loop(), &imageLocal.fontAsync, imageAsyncmsg );
	imageLocal.fontResult = PickFont( 0, 0, NULL, NULL, NULL );
	uv_async_send( &imageLocal.fontAsync );
	return 0;
}

static void pickFont( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
	class constructorSet* c = getConstructors( isolate );
	c->priorThis.Reset( isolate, args.This() );
	c->fontResult.Reset( isolate, Local<Function>::Cast( args[0] ) );
	ThreadTo( fontPickThread, 0 );
}

static uintptr_t colorPickThread( PTHREAD thread ) {
	MemSet( &imageLocal.colorAsync, 0, sizeof( &imageLocal.colorAsync ) );
	uv_async_init( uv_default_loop(), &imageLocal.colorAsync, imageAsyncmsg );
	CDATA color;
	PickColor( &color, 0, NULL );
	imageLocal.color = color;
	uv_async_send( &imageLocal.colorAsync );
	return 0;
}

static void pickColor( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
	class constructorSet* c = getConstructors( isolate );
	c->priorThis.Reset( isolate, args.This() );
	c->imageResult.Reset( isolate, Local<Function>::Cast( args[0] ) );
	ThreadTo( colorPickThread, 0 );
}


void ImageObject::Init( Local<Object> exports ) {
	InitInterfaces( SACK_GetProfileInt( NULL, "SACK/Video Render/Use OpenGL 2", 0 )
					, SACK_GetProfileInt( NULL, "SACK/Video Render/Use Vulkan", 0 ) );
	Isolate* isolate = Isolate::GetCurrent();
	class constructorSet* c = getConstructors( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	Local<FunctionTemplate> imageTemplate;

	// Prepare constructor template
	imageTemplate = FunctionTemplate::New( isolate, New );
	imageTemplate->SetClassName( localStringExternal( isolate, "sack.Image" ) );
	imageTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

	// Prototype
	NODE_SET_PROTOTYPE_METHOD( imageTemplate, "Image", ImageObject::NewSubImage );

	NODE_SET_PROTOTYPE_METHOD( imageTemplate, "reset", ImageObject::reset );
	NODE_SET_PROTOTYPE_METHOD( imageTemplate, "fill", ImageObject::fill );
	NODE_SET_PROTOTYPE_METHOD( imageTemplate, "fillOver", ImageObject::fillOver );
	NODE_SET_PROTOTYPE_METHOD( imageTemplate, "line", ImageObject::line );
	NODE_SET_PROTOTYPE_METHOD( imageTemplate, "lineOver", ImageObject::lineOver );
	NODE_SET_PROTOTYPE_METHOD( imageTemplate, "plot", ImageObject::plot );
	NODE_SET_PROTOTYPE_METHOD( imageTemplate, "plotOver", ImageObject::plotOver );
	NODE_SET_PROTOTYPE_METHOD( imageTemplate, "drawImage", ImageObject::putImage );
	NODE_SET_PROTOTYPE_METHOD( imageTemplate, "drawImageOver", ImageObject::putImageOver );
	NODE_SET_PROTOTYPE_METHOD( imageTemplate, "imageSurface", ImageObject::imageData );

	imageTemplate->PrototypeTemplate()->SetAccessorProperty( localStringExternal( isolate, "png" )
		, FunctionTemplate::New( isolate, ImageObject::getPng )
		, Local<FunctionTemplate>(), (PropertyAttribute)(ReadOnly|DontDelete) );
	imageTemplate->PrototypeTemplate()->SetAccessorProperty( localStringExternal( isolate, "jpg" )
		, FunctionTemplate::New( isolate, ImageObject::getJpeg )
		, Local<FunctionTemplate>(), (PropertyAttribute)(ReadOnly | DontDelete) );
	imageTemplate->PrototypeTemplate()->SetAccessorProperty( localStringExternal( isolate, "jpgQuality" )
		, FunctionTemplate::New( isolate, ImageObject::getJpegQuality )
		, FunctionTemplate::New( isolate, ImageObject::setJpegQuality ), DontDelete );
	imageTemplate->PrototypeTemplate()->SetAccessorProperty( localStringExternal( isolate, "width" )
		, FunctionTemplate::New( isolate, ImageObject::getWidth )
		, Local<FunctionTemplate>(), (PropertyAttribute)(ReadOnly | DontDelete) );
	imageTemplate->PrototypeTemplate()->SetAccessorProperty( localStringExternal( isolate, "height" )
		, FunctionTemplate::New( isolate, ImageObject::getHeight )
		, Local<FunctionTemplate>(), (PropertyAttribute)(ReadOnly | DontDelete) );

	c->ImageObject_tpl.Reset( isolate, imageTemplate );
	c->ImageObject_constructor.Reset( isolate, imageTemplate->GetFunction(context).ToLocalChecked() );
	SET_READONLY( exports, "Image", imageTemplate->GetFunction( context ).ToLocalChecked() );

	Local<FunctionTemplate> fontTemplate;
	fontTemplate = FunctionTemplate::New( isolate, FontObject::New );
	fontTemplate->SetClassName( localStringExternal( isolate, "sack.Image.Font" ) );
	fontTemplate->InstanceTemplate()->SetInternalFieldCount( 4+1 );

	// Prototype
	NODE_SET_PROTOTYPE_METHOD( fontTemplate, "measure", FontObject::measure );
	NODE_SET_PROTOTYPE_METHOD( fontTemplate, "save", FontObject::save );

	SET_READONLY_METHOD( fontTemplate->GetFunction(context).ToLocalChecked(), "load", FontObject::load );

	Local<FunctionTemplate> colorTemplate;

	// Prepare constructor template
	colorTemplate = FunctionTemplate::New( isolate, ColorObject::New );
	colorTemplate->SetClassName( localStringExternal( isolate, "sack.Image.Color" ) );
	colorTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 required for wrap

	NODE_SET_PROTOTYPE_METHOD( colorTemplate, "toString", ColorObject::toString );
#define SetAccessor(b,c,d) SetAccessorProperty( b, FunctionTemplate::New( isolate, c ), FunctionTemplate::New( isolate, d ), DontDelete )
	colorTemplate->PrototypeTemplate()->SetAccessor( localStringExternal( isolate, "r" )
		, ColorObject::getRed, ColorObject::setRed );
	colorTemplate->PrototypeTemplate()->SetAccessor( localStringExternal( isolate, "g" )
		, ColorObject::getGreen, ColorObject::setGreen );
	colorTemplate->PrototypeTemplate()->SetAccessor( localStringExternal( isolate, "b" )
		, ColorObject::getBlue, ColorObject::setBlue );
	colorTemplate->PrototypeTemplate()->SetAccessor( localStringExternal( isolate, "a" )
		, ColorObject::getAlpha, ColorObject::setAlpha );
	c->ColorObject_tpl.Reset( isolate, colorTemplate );
	c->ColorObject_constructor.Reset( isolate, colorTemplate->GetFunction(context).ToLocalChecked() );


	c->FontObject_constructor.Reset( isolate, fontTemplate->GetFunction(context).ToLocalChecked() );
	SET_READONLY( imageTemplate->GetFunction(context).ToLocalChecked(), "Font", fontTemplate->GetFunction(context).ToLocalChecked() );
	SET_READONLY_METHOD( fontTemplate->GetFunction(context).ToLocalChecked(), "dialog", pickFont );

	Local<Object> colors = Object::New( isolate );
	if( g.pii ) {
		SET_READONLY( colors, "white", makeColor( isolate, BASE_COLOR_WHITE ) );
		SET_READONLY( colors, "black", makeColor( isolate, BASE_COLOR_BLACK ) );
		SET_READONLY( colors, "green", makeColor( isolate, BASE_COLOR_GREEN ) );
		SET_READONLY( colors, "blue", makeColor( isolate, BASE_COLOR_BLUE ) );
		SET_READONLY( colors, "red", makeColor( isolate, BASE_COLOR_RED ) );
		SET_READONLY( colors, "darkBlue", makeColor( isolate, BASE_COLOR_DARKBLUE ) );
		SET_READONLY( colors, "cyan", makeColor( isolate, BASE_COLOR_CYAN ) );
		SET_READONLY( colors, "brown", makeColor( isolate, BASE_COLOR_BROWN ) );
		SET_READONLY( colors, "lightBrown", makeColor( isolate, BASE_COLOR_LIGHTBROWN ) );
		SET_READONLY( colors, "magenta", makeColor( isolate, BASE_COLOR_MAGENTA ) );
		SET_READONLY( colors, "lightGrey", makeColor( isolate, BASE_COLOR_LIGHTGREY ) );
		SET_READONLY( colors, "darkGrey", makeColor( isolate, BASE_COLOR_DARKGREY ) );
		SET_READONLY( colors, "lightBlue", makeColor( isolate, BASE_COLOR_LIGHTBLUE ) );
		SET_READONLY( colors, "lightGreen", makeColor( isolate, BASE_COLOR_LIGHTGREEN ) );
		SET_READONLY( colors, "lightCyan", makeColor( isolate, BASE_COLOR_LIGHTCYAN ) );
		SET_READONLY( colors, "lightRed", makeColor( isolate, BASE_COLOR_LIGHTRED ) );
		SET_READONLY( colors, "lightMagenta", makeColor( isolate, BASE_COLOR_LIGHTMAGENTA ) );
		SET_READONLY( colors, "yellow", makeColor( isolate, BASE_COLOR_YELLOW ) );
		SET_READONLY( colors, "orange", makeColor( isolate, BASE_COLOR_ORANGE ) );
		SET_READONLY( colors, "niceOrange", makeColor( isolate, BASE_COLOR_NICE_ORANGE ) );
		SET_READONLY( colors, "purple", makeColor( isolate, BASE_COLOR_PURPLE ) );
	}
	Local<Function> i = imageTemplate->GetFunction(context).ToLocalChecked();
	SET_READONLY( i, "colors", colors );
	SET_READONLY( i, "Color", colorTemplate->GetFunction(context).ToLocalChecked() );
	SET_READONLY_METHOD( colorTemplate->GetFunction(context).ToLocalChecked(), "dialog", pickColor );

}
void ImageObject::getPng( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	class constructorSet* c = getConstructors( isolate );
	Local<FunctionTemplate> tpl = c->ImageObject_tpl.Get( isolate );
	if( tpl->HasInstance( args.This() ) ) {
		ImageObject *obj = ObjectWrap::Unwrap<ImageObject>( args.This() );
		uint8_t *buf;
		size_t size;
		if( PngImageFile( obj->image, &buf, &size ) )
		{
			Local<ArrayBuffer> arrayBuffer = ArrayBuffer::New( isolate, buf, size );

			PARRAY_BUFFER_HOLDER holder = GetHolder();
			holder->o.Reset( isolate, arrayBuffer );
			holder->o.SetWeak<ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
			holder->buffer = buf;
			args.GetReturnValue().Set( arrayBuffer );
		}
	}
}
void ImageObject::getJpeg( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
	class constructorSet* c = getConstructors( isolate );
	Local<FunctionTemplate> tpl = c->ImageObject_tpl.Get( isolate );
	if( tpl->HasInstance( args.This() ) ) {

		ImageObject *obj = ObjectWrap::Unwrap<ImageObject>( args.This() );
		uint8_t *buf;
		size_t size;
		if( JpgImageFile( obj->image, &buf, &size, obj->jpegQuality ) )
		{
			Local<ArrayBuffer> arrayBuffer = ArrayBuffer::New( isolate, buf, size );

			PARRAY_BUFFER_HOLDER holder = GetHolder();
			holder->o.Reset( isolate, arrayBuffer );
			holder->o.SetWeak<ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
			holder->buffer = buf;
			args.GetReturnValue().Set( arrayBuffer );
		}
	}
}

void ImageObject::getJpegQuality( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
  class constructorSet* c = getConstructors( isolate );
  Local<FunctionTemplate> tpl = c->ImageObject_tpl.Get( isolate );
	if( tpl->HasInstance( args.This() ) ) {
		ImageObject *obj = ObjectWrap::Unwrap<ImageObject>( args.This() );
		args.GetReturnValue().Set( Integer::New( isolate, obj->jpegQuality ) );
	}
}

void ImageObject::setJpegQuality( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
	ImageObject *obj = ObjectWrap::Unwrap<ImageObject>( args.This() );
	obj->jpegQuality = (int)args[0]->IntegerValue(isolate->GetCurrentContext()).ToChecked();
}

void ImageObject::getWidth( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
  class constructorSet* c = getConstructors( isolate );
  Local<FunctionTemplate> tpl = c->ImageObject_tpl.Get( isolate );
	if( tpl->HasInstance( args.This() ) ) {
		ImageObject *obj = ObjectWrap::Unwrap<ImageObject>( args.This() );
		if( obj->image )
			args.GetReturnValue().Set( Integer::New( args.GetIsolate(), (int)obj->image->width ) );
	}
}
void ImageObject::getHeight( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
  class constructorSet* c = getConstructors( isolate );
  Local<FunctionTemplate> tpl = c->ImageObject_tpl.Get( isolate );
	if( tpl->HasInstance( args.This() ) ) {
		ImageObject *obj = ObjectWrap::Unwrap<ImageObject>( args.This() );
		if( obj->image )
			args.GetReturnValue().Set( Integer::New( args.GetIsolate(), (int)obj->image->height ) );
	}
}

ImageObject::ImageObject( uint8_t *buf, size_t len ) {
	image = DecodeMemoryToImage( buf, len );
	jpegQuality = 78;
}

ImageObject::ImageObject( const char *filename )  {
   image = LoadImageFile( filename );
   jpegQuality = 78;
}
ImageObject::ImageObject( Image image ) {
	this->image = image;
	jpegQuality = 78;
}

ImageObject::ImageObject( int x, int y, int w, int h, ImageObject * within )  {
	jpegQuality = 78;
	if( within )
	{
		container = within;
		image = MakeSubImage( within->image, x, y, w, h );
	}
	else {
		container = NULL;
		image = MakeImageFile( w, h );
	}
}

ImageObject::~ImageObject(void) {
	if( !external ) {
		lprintf( "Image has been garbage collected." );
		UnmakeImageFile( image );
	}
}

void ImageObject::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
  class constructorSet* c = getConstructors( isolate );
	if( args.IsConstructCall() ) {

		int x = 0, y = 0, w = 1024, h = 768;
		Local<Object> parentImage;
		ImageObject *parent = NULL;
		char *filename = NULL;
		ImageObject* obj = NULL;
		Local<ArrayBuffer> buffer;
		int argc = args.Length();
		if( argc > 0 ) {
			if( args[0]->IsUint8Array() ) {
				Local<Uint8Array> u8arr = args[0].As<Uint8Array>();
				Local<ArrayBuffer> myarr = u8arr->Buffer();
				obj = new ImageObject( (uint8_t*)myarr->GetContents().Data(), myarr->ByteLength() );
			} else if( args[0]->IsTypedArray() ) {
				Local<TypedArray> u8arr = args[0].As<TypedArray>();
				Local<ArrayBuffer> myarr = u8arr->Buffer();
				obj = new ImageObject( (uint8_t*)myarr->GetContents().Data(), myarr->ByteLength() );
			} else if( args[0]->IsArrayBuffer() ) {
				Local<ArrayBuffer> myarr = args[0].As<ArrayBuffer>();
				obj = new ImageObject( (uint8_t*)myarr->GetContents().Data(), myarr->ByteLength() );
			} else if( args[0]->IsNumber() )
				w = (int)args[0]->NumberValue(context).ToChecked();
			else {
				String::Utf8Value fName( isolate, args[0]->ToString(context).ToLocalChecked() );
				filename = StrDup( *fName );
			}
		}
		if( !filename && !obj ) {
			if( argc > 1 ) {
				h = (int)args[1]->NumberValue(context).ToChecked();
			}
			if( argc > 2 ) {
				parentImage = args[2]->ToObject( context).ToLocalChecked();
				parent = ObjectWrap::Unwrap<ImageObject>( parentImage );
			}
			if( argc > 3 ) {
				x = (int)args[3]->NumberValue(context).ToChecked();
			}
			if( argc > 4 ) {
				y = (int)args[4]->NumberValue(context).ToChecked();
			}
		}
		// Invoked as constructor: `new MyObject(...)`
		if( !obj ) {
			if( filename )
				obj = new ImageObject( filename );
			else
				obj = new ImageObject( x, y, w, h, parent );
		}
		obj->_this.Reset( isolate, args.This() );
		obj->Wrap( args.This() );
		args.GetReturnValue().Set( args.This() );

	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		int argc = args.Length();
		Local<Value> *argv = new Local<Value>[argc];
		for( int n = 0; n < argc; n++ )
			argv[n] = args[n];

		Local<Function> cons = Local<Function>::New( isolate, c->ImageObject_constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
		delete argv;
	}
}

void ImageObject::NewSubImage( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	if( args.IsConstructCall() ) {

		int x = 0, y = 0, w = 1024, h = 768;
		Local<Object> parentImage;
		ImageObject *parent = ObjectWrap::Unwrap<ImageObject>( args.This() );
		int argc = args.Length();
		int arg_ofs = 0;
		if( argc > 0 ) {
			if( args[0]->IsObject() ) {
				ImageObject *parent = ObjectWrap::Unwrap<ImageObject>( args[0]->ToObject( context).ToLocalChecked() );
				arg_ofs = 1;
			}
		if( (argc+arg_ofs) > 0 )
			x = (int)args[0+arg_ofs]->NumberValue(context).ToChecked();
		}
		if( (argc + arg_ofs) > 1 ) {
			y = (int)args[1 + arg_ofs]->NumberValue(context).ToChecked();
		}
		if( (argc + arg_ofs) > 2 ) {
			w = (int)args[2 + arg_ofs]->NumberValue(context).ToChecked();
		}
		if( (argc + arg_ofs) > 3 ) {
			h = (int)args[3 + arg_ofs]->NumberValue(context).ToChecked();
		}

		// Invoked as constructor: `new MyObject(...)`
		ImageObject* obj;
		obj = new ImageObject( x, y, w, h, parent );

		obj->Wrap( args.This() );
		args.GetReturnValue().Set( args.This() );
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		int argc = args.Length();
		Local<Value> *argv = new Local<Value>[argc+1];
		int n;
		for( n = 0; n < argc; n++ )
			argv[n+1] = args[n];
		argv[0] = args.Holder();

    class constructorSet* c = getConstructors( isolate );
    Local<Function> cons = Local<Function>::New( isolate, c->ImageObject_constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
		delete argv;
	}
}


Local<Object> ImageObject::NewImage( Isolate*isolate, Image image, LOGICAL external ) {
	// Invoked as constructor: `new MyObject(...)`
	ImageObject* obj;

	int argc = 0;
	Local<Value> *argv = new Local<Value>[argc];
  class constructorSet* c = getConstructors( isolate );
  Local<Function> cons = Local<Function>::New( isolate, c->ImageObject_constructor );
	Local<Object> lo = cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked();
	obj = ObjectWrap::Unwrap<ImageObject>( lo );
	obj->image = image;
	obj->external = external;
	return lo;
}

ImageObject * ImageObject::MakeNewImage( Isolate*isolate, Image image, LOGICAL external ) {
	// Invoked as constructor: `new MyObject(...)`
	ImageObject* obj;

	int argc = 0;
	Local<Value> *argv = new Local<Value>[argc];
  class constructorSet* c = getConstructors( isolate );
  Local<Function> cons = Local<Function>::New( isolate, c->ImageObject_constructor );
	Local<Object> lo = cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked();
	obj = ObjectWrap::Unwrap<ImageObject>( lo );
	obj->image = image;
	obj->external = external;
	return obj;
}


void ImageObject::reset( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	args[3]->ToObject( context).ToLocalChecked();
	ImageObject *io = ObjectWrap::Unwrap<ImageObject>( args.This() );
	ClearImage( io->image );
}

void ImageObject::fill( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	ImageObject *io = ObjectWrap::Unwrap<ImageObject>( args.This() );
	int argc = args.Length();
	int x, y, w, h, c = BASE_COLOR_BLACK;
	if( argc == 1 ) {
		if( args[0]->IsObject() ) {
			ColorObject *co = ObjectWrap::Unwrap<ColorObject>( args[0]->ToObject( context).ToLocalChecked() );
			c = co->color;
		} else if( args[0]->IsUint32() )
			c = (int)args[0]->Uint32Value(context).ToChecked();
		else if( args[0]->IsNumber() )
			c = (int)args[0]->NumberValue(context).ToChecked();
		else
			c = 0;
		x = 0;
		y = 0;
		w = io->image->width;
		h = io->image->height;
	} else {
		if( argc > 0 ) {
			x = (int)args[0]->NumberValue(context).ToChecked();
		}
		if( argc > 1 ) {
			y = (int)args[1]->NumberValue(context).ToChecked();
		}
		if( argc > 2 ) {
			w = (int)args[2]->NumberValue(context).ToChecked();
		}
		if( argc > 3 ) {
			h = (int)args[3]->NumberValue(context).ToChecked();
		}
		if( argc > 4 ) {
			if( args[4]->IsObject() ) {
				ColorObject *co = ObjectWrap::Unwrap<ColorObject>( args[4]->ToObject( context).ToLocalChecked() );
				c = co->color;
			} else if( args[4]->IsUint32() )
				c = (int)args[4]->Uint32Value(context).ToChecked();
			else if( args[4]->IsNumber() )
				c = (int)args[4]->NumberValue(context).ToChecked();
			else
				c = 0;
		}
	}
	BlatColor( io->image, x, y, w, h, c );
}

void ImageObject::fillOver( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	int argc = args.Length();
	ImageObject *io = ObjectWrap::Unwrap<ImageObject>( args.This() );
	int x, y, w, h, c;
	if( argc > 0 ) {
		x = (int)args[0]->NumberValue(context).ToChecked();
	}
	if( argc > 1 ) {
		y = (int)args[1]->NumberValue(context).ToChecked();
	}
	if( argc > 2 ) {
		w = (int)args[2]->NumberValue(context).ToChecked();
	}
	if( argc > 3 ) {
		h = (int)args[3]->NumberValue(context).ToChecked();
	}
	if( argc > 4 ) {
		if( args[4]->IsObject() ) {
			ColorObject *co = ObjectWrap::Unwrap<ColorObject>( args[4]->ToObject( context).ToLocalChecked() );
			c = co->color;
		}
		else if( args[4]->IsUint32() )
			c = (int)args[4]->Uint32Value(context).ToChecked();
		c = (int)args[4]->NumberValue(context).ToChecked();
	}
	BlatColorAlpha( io->image, x, y, w, h, c );
}

void ImageObject::plot( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	int argc = args.Length();
	ImageObject *io = ObjectWrap::Unwrap<ImageObject>( args.This() );
	int x, y, c;
	if( argc > 0 ) {
		x = (int)args[0]->NumberValue(context).ToChecked();
	}
	if( argc > 1 ) {
		y = (int)args[1]->NumberValue(context).ToChecked();
	}
	if( argc > 2 ) {
		if( args[2]->IsObject() ) {
			ColorObject *co = ObjectWrap::Unwrap<ColorObject>( args[2]->ToObject( context).ToLocalChecked() );
			c = co->color;
		}
		else if( args[2]->IsUint32() )
			c = (int)args[2]->Uint32Value(context).ToChecked();
		else
			c = (int)args[2]->NumberValue(context).ToChecked();
	}
	g.pii->_plot( io->image, x, y, c );
}

void ImageObject::plotOver( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	int argc = args.Length();
	ImageObject *io = ObjectWrap::Unwrap<ImageObject>( args.This() );
	int x, y, c;
	if( argc > 0 ) {
		x = (int)args[0]->NumberValue(context).ToChecked();
	}
	if( argc > 1 ) {
		y = (int)args[1]->NumberValue(context).ToChecked();
	}
	if( argc > 2 ) {
		if( args[2]->IsObject() ) {
			ColorObject *co = ObjectWrap::Unwrap<ColorObject>( args[2]->ToObject( context).ToLocalChecked() );
			c = co->color;
		}
		else if( args[2]->IsUint32() )
			c = (int)args[2]->Uint32Value(context).ToChecked();
		else
			c = (int)args[2]->NumberValue(context).ToChecked();
	}
	plotalpha( io->image, x, y, c );
}

void ImageObject::line( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	int argc = args.Length();
	ImageObject *io = ObjectWrap::Unwrap<ImageObject>( args.This() );
	int x, y, xTo, yTo, c;
	if( argc > 0 ) {
		x = (int)args[0]->NumberValue(context).ToChecked();
	}
	if( argc > 1 ) {
		y = (int)args[1]->NumberValue(context).ToChecked();
	}
	if( argc > 2 ) {
		xTo = (int)args[2]->NumberValue(context).ToChecked();
	}
	if( argc > 3 ) {
		yTo = (int)args[3]->NumberValue(context).ToChecked();
	}
	if( argc > 4 ) {
		if( args[2]->IsObject() ) {
			ColorObject *co = ObjectWrap::Unwrap<ColorObject>( args[4]->ToObject( context).ToLocalChecked() );
			c = co->color;
		}
		else if( args[2]->IsUint32() )
			c = (int)args[4]->Uint32Value(context).ToChecked();
		else
			c = (int)args[4]->NumberValue(context).ToChecked();
	}
	if( x == xTo )
		do_vline( io->image, x, y, yTo, c );
	else if( y == yTo )
		do_hline( io->image, y, x, xTo, c );
	else
		do_line( io->image, x, y, xTo, yTo, c );
}

void ImageObject::lineOver( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	ImageObject *io = ObjectWrap::Unwrap<ImageObject>( args.This() );
	int argc = args.Length();
	int x, y, xTo, yTo, c;
	if( argc > 0 ) {
		x = (int)args[0]->NumberValue(context).ToChecked();
	}
	if( argc > 1 ) {
		y = (int)args[1]->NumberValue(context).ToChecked();
	}
	if( argc > 2 ) {
		xTo = (int)args[2]->NumberValue(context).ToChecked();
	}
	if( argc > 3 ) {
		yTo = (int)args[3]->NumberValue(context).ToChecked();
	}
	if( argc > 4 ) {
		if( args[4]->IsObject() ) {
			ColorObject *co = ObjectWrap::Unwrap<ColorObject>( args[4]->ToObject( context).ToLocalChecked() );
			c = co->color;
		}
		else if( args[2]->IsUint32() )
			c = (int)args[4]->Uint32Value(context).ToChecked();
		else
			c = (int)args[4]->NumberValue(context).ToChecked();
	}
	if( x == xTo )
		do_vlineAlpha( io->image, x, y, yTo, c );
	else if( y == yTo )
		do_hlineAlpha( io->image, y, x, xTo, c );
	else
		do_lineAlpha( io->image, x, y, xTo, yTo, c );
}

// {x, y output
// w, h} output
// {x, y input
// w, h} input
void ImageObject::putImage( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	ImageObject *io = ObjectWrap::Unwrap<ImageObject>( args.This() );
	int argc = args.Length();
	ImageObject *ii;// = ObjectWrap::Unwrap<ImageObject>( args.This() );
	int x = 0, y= 0, xAt, yAt;
	int w, h;
	if( argc > 0 ) {
		ii = ObjectWrap::Unwrap<ImageObject>( args[0]->ToObject( context).ToLocalChecked() );
		if( !ii || !ii->image ) {
			lprintf( "Bad First paraemter, must be an image to put?" );
			return;
		}
		else {
			w = ii->image->width;
			h = ii->image->height;
		}
	}
	else {
		// throw error ?
		return;
	}
	if( argc > 2 ) {
		x = (int)args[1]->NumberValue(context).ToChecked();
		y = (int)args[2]->NumberValue(context).ToChecked();
	}
	else {
		x = 0;
		y = 0;
		//isolate->ThrowException( Exception::Error( localStringExternal( isolate, "Required parameters for position missing." ) ) );
		//return;
	}
	if( argc > 3 ) {
		xAt = (int)args[3]->NumberValue(context).ToChecked();

		if( argc > 4 ) {
			yAt = (int)args[4]->NumberValue(context).ToChecked();
		}
		else {
			isolate->ThrowException( Exception::Error( localStringExternal( isolate, "Required parameters for position missing." ) ) );
			return;
		}
		if( argc > 5 ) {
			w = (int)args[5]->NumberValue(context).ToChecked();
			if( argc > 6 ) {
				h = (int)args[6]->NumberValue(context).ToChecked();
			}
			if( argc > 7 ) {
				int ow, oh;
		
				ow = xAt;
				oh = yAt;
				xAt = w;
				yAt = h;
				if( argc > 7 ) {
					w = (int)args[7]->NumberValue(context).ToChecked();
					if( w < 0 ) w = ii->image->width;
				}
				if( argc > 8 ) {
					h = (int)args[8]->NumberValue(context).ToChecked();
					if( h < 0 ) h = ii->image->height;
				}
				if( ow && oh && w && h )
					BlotScaledImageSizedEx( io->image, ii->image, x, y, ow, oh, xAt, yAt, w, h, 1, BLOT_COPY );
			}
			else
				BlotImageSizedEx( io->image, ii->image, x, y, xAt, yAt, w, h, 0, BLOT_COPY );
		}
		else  {
			w = xAt; 
			h = yAt;
			BlotImageSizedEx( io->image, ii->image, x, y, 0, 0, w, h, 0, BLOT_COPY );
		}
	}
	else
		BlotImageEx( io->image, ii->image, x, y, 0, BLOT_COPY );
}

void ImageObject::putImageOver( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	ImageObject *io = ObjectWrap::Unwrap<ImageObject>( args.This() );
	ImageObject *ii;// = ObjectWrap::Unwrap<ImageObject>( args.This() );
	int argc = args.Length();
	int x, y, xTo, yTo, c;
	if( argc > 0 ) {
		ii = ObjectWrap::Unwrap<ImageObject>( args[0]->ToObject( context).ToLocalChecked() );
	}
	if( argc > 1 ) {
		x = (int)args[1]->NumberValue(context).ToChecked();
	}
	if( argc > 2 ) {
		y = (int)args[2]->NumberValue(context).ToChecked();
	}
	if( argc > 2 ) {
		xTo = (int)args[2]->NumberValue(context).ToChecked();

		if( argc > 3 ) {
			yTo = (int)args[3]->NumberValue(context).ToChecked();
		}
		if( argc > 4 ) {
			c = (int)args[4]->NumberValue(context).ToChecked();
		}
		else {
		}
	}
	else
		BlotImageEx( io->image, ii->image, x, y, ALPHA_TRANSPARENT, BLOT_COPY );
}

void ImageObject::imageData( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	
	ImageObject *io = ObjectWrap::Unwrap<ImageObject>( args.This() );

	//Context::Global()
	size_t length;
	Local<SharedArrayBuffer> ab =
		SharedArrayBuffer::New( isolate,
			GetImageSurface( io->image ),
			length = io->image->height * io->image->pwidth );
	Local<Uint8Array> ui = Uint8Array::New( ab, 0, length );

	args.GetReturnValue().Set( ui );
}



FontObject::~FontObject() {
}

FontObject::FontObject( const char *filename, int w, int h, int flags ) {
   font = InternalRenderFontFile( filename, w, h, NULL, NULL, flags );
}

FontObject::FontObject(  ) {
	font = NULL;
}

void FontObject::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	if( args.IsConstructCall() ) {

		int w = 24, h = 24;
		int flags;
		Local<Object> parentFont;
		FontObject *parent = NULL;
		char *filename = NULL;

		int argc = args.Length();

		if( argc > 0 ) {
			String::Utf8Value fName( isolate, args[0]->ToString(context).ToLocalChecked() );
			filename = StrDup( *fName );
		} else {
			FontObject* obj;
			obj = new FontObject();
			obj->Wrap( args.This() );
			args.GetReturnValue().Set( args.This() );
			return;
		}

		if( argc > 1 ) {
			w = (int)args[1]->NumberValue(context).ToChecked();
		}
		if( argc > 2 ) {
			h = (int)args[2]->NumberValue(context).ToChecked();
		}
		if( argc >3 ) {
			flags = (int)args[3]->NumberValue(context).ToChecked();
		}
		else
			flags = 3;

		// Invoked as constructor: `new MyObject(...)`
		FontObject* obj;
		if( filename )
			obj = new FontObject( filename, w, h, flags );

		obj->Wrap( args.This() );
		args.GetReturnValue().Set( args.This() );

	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		int argc = args.Length();
		Local<Value> *argv = new Local<Value>[argc];
		for( int n = 0; n < argc; n++ )
			argv[n] = args[n];

		class constructorSet* c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->FontObject_constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
		delete argv;
	}
}


void FontObject::measure( const FunctionCallbackInfo<Value>& args ) {
		Isolate* isolate = args.GetIsolate();
		FontObject *fo = ObjectWrap::Unwrap<FontObject>( args.This() );
		int argc = args.Length();
}

void FontObject::save( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	FontObject *fo = ObjectWrap::Unwrap<FontObject>( args.This() );
	String::Utf8Value fName( isolate, args[0]->ToString(context).ToLocalChecked() );
	size_t dataLen;
	POINTER data;
	GetFontRenderData( fo->font, &data, &dataLen );
	FILE *out = sack_fopen( 0, *fName, "wb" );
	if( out ) {
		sack_fwrite( data, dataLen, 1, out );
		sack_fclose( out );
	}
}

void FontObject::load( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	String::Utf8Value fName( isolate, args[0]->ToString(context).ToLocalChecked() );
	int argc = args.Length();

	size_t dataLen;
	PFONTDATA data;
	FILE *out = sack_fopen( 0, *fName, "rb" );
	if( out ) {
		dataLen = sack_fsize( out );
		data = (PFONTDATA)NewArray( uint8_t, dataLen );
		sack_fread( data, dataLen, 1, out );
		sack_fclose( out );
	}

	class constructorSet* c = getConstructors( isolate );
	Local<Function> cons = Local<Function>::New( isolate, c->FontObject_constructor );
	Local<Object> result;
	args.GetReturnValue().Set( result = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked() );

	FontObject *fo = ObjectWrap::Unwrap<FontObject>( result );
	FRACTION one = { 1,1 };
	fo->font = RenderScaledFontData( data, &one, &one );

}




ColorObject::~ColorObject() {
}
ColorObject::ColorObject() {
	color = 0xFFCCFFFF;
}
ColorObject::ColorObject( int r,int grn, int b, int a ) {
	color = MakeAlphaColor( r, grn, b, a );
}
ColorObject::ColorObject( CDATA r ) {
	color = r;
}

bool ColorObject::isColor( Isolate *isolate, Local<Object> object ) {
	class constructorSet* c = getConstructors( isolate );
	Local<FunctionTemplate> colorTpl = c->ColorObject_tpl.Get( isolate );
	return colorTpl->HasInstance( object );
}

CDATA ColorObject::getColor( Local<Object> object ) {
	ColorObject *co = ObjectWrap::Unwrap<ColorObject>( object );
	return co->color;
}

Local<Object> ColorObject::makeColor( Isolate *isolate, CDATA rgba ) {
	class constructorSet* c = getConstructors( isolate );
	Local<Function> cons = Local<Function>::New( isolate, c->ColorObject_constructor );
	Local<Object> _color = cons->NewInstance( isolate->GetCurrentContext(), 0, NULL ).ToLocalChecked();
	ColorObject *co = ObjectWrap::Unwrap<ColorObject>( _color );
	co->color = rgba;
	return _color;
}

void ColorObject::toString( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	ColorObject *co = ObjectWrap::Unwrap<ColorObject>( args.This() );
	char buf[128];
	snprintf( buf, 128, "{r:%d,g:%d,b:%d,a:%d}", RedVal( co->color ), GreenVal( co->color ), BlueVal( co->color ), AlphaVal( co->color ) );
	
	args.GetReturnValue().Set( localStringExternal( isolate, buf ) );
}

void ColorObject::New( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	if( args.IsConstructCall() ) {
		int r, grn, b, a;
		int argc = args.Length();

		ColorObject* obj;
		if( argc == 1 ) {
			if( args[0]->IsObject() ) {
				Local<Object> o = args[0]->ToObject( context).ToLocalChecked();
				r = (int)o->Get( context, localStringExternal( isolate, "r" ) ).ToLocalChecked()->NumberValue(context).ToChecked();
				grn = (int)o->Get( context, localStringExternal( isolate, "g" ) ).ToLocalChecked()->NumberValue(context).ToChecked();
				b = (int)o->Get( context, localStringExternal( isolate, "b" ) ).ToLocalChecked()->NumberValue(context).ToChecked();
				a = (int)o->Get( context, localStringExternal( isolate, "a" ) ).ToLocalChecked()->NumberValue(context).ToChecked();
				obj = new ColorObject( r,grn,b,a );

			}
			else if( args[0]->IsUint32() ) {
				r = (int)args[0]->Uint32Value(context).ToChecked();
				obj = new ColorObject( r );
			}
			else if( args[0]->IsNumber() ) {
				r = (int)args[0]->NumberValue(context).ToChecked();
				obj = new ColorObject( SetAlpha( r, 255 ) );
			}
			else if( args[0]->IsString() ) {
				// parse string color....
				//Config
			}

		}
		else if( argc == 4 ) {
			r = (int)args[0]->NumberValue(context).ToChecked();
			grn = (int)args[1]->NumberValue(context).ToChecked();
			b = (int)args[2]->NumberValue(context).ToChecked();
			a = (int)args[3]->NumberValue(context).ToChecked();
			obj = new ColorObject( r, grn, b, a );
		}
		else {
			obj = new ColorObject();
		}
		obj->Wrap( args.This() );
		args.GetReturnValue().Set( args.This() );

	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		int argc = args.Length();
		Local<Value> *argv = new Local<Value>[argc];
		for( int n = 0; n < argc; n++ )
			argv[n] = args[n];

		class constructorSet* c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->ColorObject_constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
		delete argv;
	}
}


void ColorObject::getRed( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	class constructorSet* c = getConstructors( isolate );
	if( c->ColorObject_tpl.Get( isolate )->HasInstance( args.This() ) ) {
		ColorObject *co = ObjectWrap::Unwrap<ColorObject>( args.This() );
		args.GetReturnValue().Set( Integer::New( isolate, RedVal( co->color ) ) );
	}
}
void ColorObject::getGreen( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
	class constructorSet* c = getConstructors( isolate );
	if( c->ColorObject_tpl.Get( isolate )->HasInstance( args.This() ) ) {
		ColorObject *co = ObjectWrap::Unwrap<ColorObject>( args.This() );
		args.GetReturnValue().Set( Integer::New( isolate, GreenVal( co->color ) ) );
	}
}
void ColorObject::getBlue( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	class constructorSet* c = getConstructors( isolate );
	if( c->ColorObject_tpl.Get( isolate )->HasInstance( args.This() ) ) {
		ColorObject *co = ObjectWrap::Unwrap<ColorObject>( args.This() );
		args.GetReturnValue().Set( Integer::New( isolate, BlueVal( co->color ) ) );
	}
}
void ColorObject::getAlpha( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	class constructorSet* c = getConstructors( isolate );
	Local<Context> context = isolate->GetCurrentContext();
	if( c->ColorObject_tpl.Get( isolate )->HasInstance( args.This() ) ) {
		ColorObject *co = ObjectWrap::Unwrap<ColorObject>( args.This() );
		args.GetReturnValue().Set( Integer::New( isolate, AlphaVal( co->color ) ) );
	}
}
void ColorObject::setRed( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	ColorObject *co = ObjectWrap::Unwrap<ColorObject>( args.This() );
	double val = args[0]->NumberValue(context).ToChecked();
	if( val < 0 ) val = 0;
	else if( val > 255 ) val = 255;
	if( val > 0 && val < 1.0 ) val = 255 * val;
	SetRedValue( co->color, (COLOR_CHANNEL)val );
}
void ColorObject::setGreen( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	ColorObject *co = ObjectWrap::Unwrap<ColorObject>( args.This() );
	double val = args[0]->NumberValue(context).ToChecked();
	if( val < 0 ) val = 0;
	else if( val > 255 ) val = 255;
	if( val > 0 && val < 1.0 ) val = 255 * val;
	SetGreenValue( co->color, (COLOR_CHANNEL)val );
}
void ColorObject::setBlue( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	ColorObject *co = ObjectWrap::Unwrap<ColorObject>( args.This() );
	double val = args[0]->NumberValue(context).ToChecked();
	if( val < 0 ) val = 0;
	else if( val > 255 ) val = 255;
	if( val > 0 && val < 1.0 ) val = 255 * val;
	SetBlueValue( co->color, (COLOR_CHANNEL)val );
}
void ColorObject::setAlpha( const FunctionCallbackInfo<Value>&  args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	ColorObject *co = ObjectWrap::Unwrap<ColorObject>( args.This() );
	double val = args[0]->NumberValue(context).ToChecked();
	if( val < 0 ) val = 0;
	else if( val > 255 ) val = 255;
	if( val > 0 && val < 1.0 ) val = 255 * val;
	SetAlphaValue( co->color, (COLOR_CHANNEL)val );
}
