
#include "global.h"

struct optionStrings {
	Isolate *isolate;
	//Eternal<String> *String;
	Eternal<String> *workString;
	Eternal<String> *binString;
	Eternal<String> *argString;
	Eternal<String> *envString;
	Eternal<String> *binaryString;
	Eternal<String> *inputString;
	Eternal<String>* inputString2;
	Eternal<String> *endString;
	Eternal<String> *firstArgIsArgString;
	Eternal<String>* groupString;
	Eternal<String>* consoleString;
	Eternal<String>* useBreakString;
	Eternal<String>* useSignalString;
	Eternal<String>* noWindowString;
	Eternal<String>* hiddenString;
	Eternal<String>* noKillString;
	Eternal<String>* noWaitString;
	Eternal<String>* detachedString;
	Eternal<String>* noInheritStdio;
#if _WIN32
	Eternal<String>* adminString;
	Eternal<String>* moveToString;
	Eternal<String>* styleString;

	// for style window
	Eternal<String>* windowString;
	Eternal<String>* windowExString;
	Eternal<String>* classString;
	// for move window
	Eternal<String>* timeoutString;
	Eternal<String>* cbString;
	Eternal<String>* xString;
	Eternal<String>* yString;
	Eternal<String>* widthString;
	Eternal<String>* heightString;
	Eternal<String>* displayString;
	Eternal<String>* monitorString;

	// for display monitor information
	Eternal<String>* currentString;
	Eternal<String>* primaryString;
	Eternal<String>* deviceString;
#endif
};


static struct local {
	PLIST tasks;
} l;


static void GetProcessId( const FunctionCallbackInfo<Value>& args );  // get title by process ID
static void GetProcessParentId( const FunctionCallbackInfo<Value>& args );

#if _WIN32
static void doMoveWindow( Isolate*isolate, Local<Context> context, TaskObject *task, HWND hWnd, Local<Object> opts ); // move a task window
static void doStyleWindow( Isolate* isolate, Local<Context> context, TaskObject* task, HWND hWnd, Local<Object> opts ); // style a task window

static void getProcessWindowTitle( const FunctionCallbackInfo<Value>& args );  // get title by process ID
static void setProcessWindowStyles( const FunctionCallbackInfo<Value>& args );  // set window and class styles
static void getProcessWindowStyles( const FunctionCallbackInfo<Value>& args );  // get window and class styles
static void getProcessWindowPos( const FunctionCallbackInfo<Value>& args );
static void setProcessWindowPos( const FunctionCallbackInfo<Value>& args );
static void dropConsole( const FunctionCallbackInfo<Value>& args );
#endif
//v8::Persistent<v8::Function> TaskObject::constructor;

static struct optionStrings *getStrings( Isolate *isolate ) {
	static PLIST strings;
	INDEX idx;
	struct optionStrings * check;
	LIST_FORALL( strings, idx, struct optionStrings *, check ) {
		if( check->isolate == isolate )
			break;
	}
	if( !check ) {
		check = NewArray( struct optionStrings, 1 );
		AddLink( &strings, check );
		check->isolate = isolate;
		check->workString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "work" ) );
		check->binString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "bin" ) );
		check->argString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "args" ) );
		check->envString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "env" ) );
		check->binaryString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "binary" ) );
		check->inputString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "input" ) );
		check->groupString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "newGroup" ) );
		check->consoleString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "newConsole" ) );
		check->inputString2 = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "errorInput" ) );
		check->endString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "end" ) );
		check->firstArgIsArgString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "firstArgIsArg" ) );
		check->useBreakString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "useBreak" ) );
		check->useSignalString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "useSignal" ) );
		check->noWindowString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "noWindow" ) );
		check->hiddenString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "hidden" ) );
		check->noKillString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "noKill" ) );
		check->noWaitString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "noWait" ) );
		check->detachedString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "detached" ) );
		check->noInheritStdio =  new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "noInheritStdio" ) );
		
#if _WIN32
		check->adminString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "admin" ) );
		check->moveToString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "moveTo" ) );
		check->styleString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "style" ) );
		check->windowString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "window" ) );
		check->windowExString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "windowEx" ) );
		check->classString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "class" ) );

		check->timeoutString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "timeout" ) );
		check->cbString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "cb" ) );
		check->xString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "x" ) );
		check->yString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "y" ) );
		check->widthString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "width" ) );
		check->heightString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "height" ) );
		check->displayString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "display" ) );
		check->currentString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "current" ) );
		check->primaryString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "primary" ) );
		check->deviceString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "device" ) );
		check->monitorString = new Eternal<String>( isolate, String::NewFromUtf8Literal( isolate, "monitor" ) );
#endif
	}
	return check;
}

TaskObject::TaskObject():_this(), endCallback(), inputCallback(), inputCallback2()
{
	task = NULL;
	binary = false;
	ending = false;
	ended = false;
	stopped = false;
	killed = false;
	exitCode = 0;
	killAtExit = false;
	output = CreateLinkQueue();
	output2 = CreateLinkQueue();
#ifdef _WIN32
	moved = 0;
	styled = 0;
#endif
	//this[0] = _blankTask;
}

TaskObject::~TaskObject() {
	struct taskObjectOutputItem* outmsg;

	while( outmsg = (struct taskObjectOutputItem*)DequeLink( &this->output ) ) {
		Deallocate( struct taskObjectOutputItem*, outmsg );
	}

	while( outmsg = (struct taskObjectOutputItem*)DequeLink( &this->output2 ) )
		Deallocate( struct taskObjectOutputItem*, outmsg );

	DeleteLink( &l.tasks, this );
	DeleteLinkQueue( &output );
	DeleteLinkQueue( &output2 );
	if( task && !ended ) {
		StopProgram( task );
	}
}

void InitTask( Isolate *isolate, Local<Object> exports ) {

	Local<FunctionTemplate> taskTemplate;
	taskTemplate = FunctionTemplate::New( isolate, TaskObject::New );
	taskTemplate->SetClassName( String::NewFromUtf8Literal( isolate, "sack.task" ) );
	taskTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "end", TaskObject::End );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "isRunning", TaskObject::isRunning );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "send", TaskObject::Write );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "terminate", TaskObject::Terminate );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "write", TaskObject::Write );
#if _WIN32
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "getStyles", TaskObject::getProcessWindowStyles );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "getPosition", TaskObject::getProcessWindowPos );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "moveWindow", TaskObject::moveWindow );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "refreshWindow", TaskObject::refreshWindow );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "setStyles", TaskObject::setProcessWindowStyles );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "styleWindow", TaskObject::styleWindow ); // this is the preferred method.
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "windowTitle", TaskObject::getWindowTitle );

#endif

	taskTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "exitCode" )
		, FunctionTemplate::New( isolate, TaskObject::getExitCode )
		, Local<FunctionTemplate>() );
	taskTemplate->ReadOnlyPrototype();
	class constructorSet* c = getConstructors( isolate );
	c->TaskObject_constructor.Reset( isolate, taskTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	Local<Function> taskF;
	SET_READONLY( exports, "Task", taskF = taskTemplate->GetFunction( isolate->GetCurrentContext() ).ToLocalChecked() );
	SET_READONLY_METHOD( taskF, "loadLibrary", TaskObject::loadLibrary );
	SET_READONLY_METHOD( taskF, "getProcessList", TaskObject::GetProcessList );
	SET_READONLY_METHOD( taskF, "processId", ::GetProcessId );
	SET_READONLY_METHOD( taskF, "parentId", ::GetProcessParentId );
	SET_READONLY_METHOD( taskF, "kill", TaskObject::KillProcess );
	SET_READONLY_METHOD( taskF, "stop", TaskObject::StopProcess );
#ifdef _WIN32
	SET_READONLY_METHOD( taskF, "dropConsole", dropConsole );
	SET_READONLY_METHOD( taskF, "getDisplays", TaskObject::getDisplays );
	SET_READONLY_METHOD( taskF, "getPosition", ::getProcessWindowPos );
	SET_READONLY_METHOD( taskF, "getStyles", ::getProcessWindowStyles );
	SET_READONLY_METHOD( taskF, "getTitle", getProcessWindowTitle );
	SET_READONLY_METHOD( taskF, "setPosition", ::setProcessWindowPos );
	SET_READONLY_METHOD( taskF, "setStyles", ::setProcessWindowStyles );

	Local<Object> styles = Object::New( isolate );
	Local<Object> windowStyles = Object::New( isolate );
	Local<Object> windowExStyles = Object::New( isolate );
	Local<Object> classStyles = Object::New( isolate );
	SET_READONLY( taskF, "style", styles );
	SET_READONLY( styles, "window", windowStyles );
#define SetStyle( object, style ) SET_READONLY( object, #style, Integer::New( isolate, style ) );

	SetStyle( windowStyles, WS_BORDER );
	SetStyle( windowStyles, WS_CAPTION );
	SetStyle( windowStyles, WS_CHILD );
	SetStyle( windowStyles, WS_CHILDWINDOW );
	SetStyle( windowStyles, WS_CLIPCHILDREN );
	SetStyle( windowStyles, WS_CLIPSIBLINGS );
	SetStyle( windowStyles, WS_DISABLED );
	SetStyle( windowStyles, WS_DLGFRAME );
	SetStyle( windowStyles, WS_GROUP );
	SetStyle( windowStyles, WS_HSCROLL );
	SetStyle( windowStyles, WS_ICONIC );
	SetStyle( windowStyles, WS_MAXIMIZE );
	SetStyle( windowStyles, WS_MAXIMIZEBOX );
	SetStyle( windowStyles, WS_MINIMIZE );
	SetStyle( windowStyles, WS_MINIMIZEBOX );
	SetStyle( windowStyles, WS_OVERLAPPED );
	SetStyle( windowStyles, WS_OVERLAPPEDWINDOW );
	SetStyle( windowStyles, WS_POPUP );
	SetStyle( windowStyles, WS_POPUPWINDOW );
	SetStyle( windowStyles, WS_SIZEBOX );
	SetStyle( windowStyles, WS_SYSMENU );
	SetStyle( windowStyles, WS_TABSTOP );
	SetStyle( windowStyles, WS_THICKFRAME );
	SetStyle( windowStyles, WS_TILED );
	SetStyle( windowStyles, WS_TILEDWINDOW );
	SetStyle( windowStyles, WS_VISIBLE );
	SetStyle( windowStyles, WS_VSCROLL );

	SetStyle( windowExStyles, WS_EX_ACCEPTFILES );
	SetStyle( windowExStyles, WS_EX_APPWINDOW );
	SetStyle( windowExStyles, WS_EX_CLIENTEDGE );
	SetStyle( windowExStyles, WS_EX_COMPOSITED );
	SetStyle( windowExStyles, WS_EX_CONTEXTHELP );
	SetStyle( windowExStyles, WS_EX_CONTROLPARENT );
	SetStyle( windowExStyles, WS_EX_DLGMODALFRAME );
	SetStyle( windowExStyles, WS_EX_LAYERED );
	SetStyle( windowExStyles, WS_EX_LEFT );
	SetStyle( windowExStyles, WS_EX_LEFTSCROLLBAR );
	SetStyle( windowExStyles, WS_EX_LTRREADING );
	SetStyle( windowExStyles, WS_EX_MDICHILD );
	SetStyle( windowExStyles, WS_EX_NOACTIVATE );
	SetStyle( windowExStyles, WS_EX_NOINHERITLAYOUT );
	SetStyle( windowExStyles, WS_EX_NOPARENTNOTIFY );
#if(WINVER >= 0x0602)
	SetStyle( windowExStyles, WS_EX_NOREDIRECTIONBITMAP );
#endif
	SetStyle( windowExStyles, WS_EX_OVERLAPPEDWINDOW );
	SetStyle( windowExStyles, WS_EX_PALETTEWINDOW );
	SetStyle( windowExStyles, WS_EX_RIGHT );
	SetStyle( windowExStyles, WS_EX_RIGHTSCROLLBAR );
	SetStyle( windowExStyles, WS_EX_RTLREADING );
	SetStyle( windowExStyles, WS_EX_TOOLWINDOW );
	SetStyle( windowExStyles, WS_EX_TOPMOST );
	SetStyle( windowExStyles, WS_EX_TRANSPARENT );
	SetStyle( windowExStyles, WS_EX_WINDOWEDGE );


	SET_READONLY( styles, "windowEx", windowExStyles );
	SET_READONLY( styles, "class", classStyles );

	SetStyle( classStyles, CS_BYTEALIGNCLIENT );
	SetStyle( classStyles, CS_BYTEALIGNWINDOW );
	SetStyle( classStyles, CS_CLASSDC );
	SetStyle( classStyles, CS_DBLCLKS );
	SetStyle( classStyles, CS_DROPSHADOW );
	SetStyle( classStyles, CS_GLOBALCLASS );
	SetStyle( classStyles, CS_HREDRAW );
	SetStyle( classStyles, CS_NOCLOSE );
	SetStyle( classStyles, CS_OWNDC );
	SetStyle( classStyles, CS_PARENTDC );
	SetStyle( classStyles, CS_SAVEBITS );
	SetStyle( classStyles, CS_VREDRAW );

#endif
}

static void taskAsyncClosed( uv_handle_t* async ) {
	TaskObject* task = (TaskObject*)async->data;
	task->_this.Reset();
}


static void taskAsyncMsg( uv_async_t* handle ) {
	TaskObject *task = (TaskObject*)handle->data;
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	HandleScope scope( isolate );
	Local<Context> context = isolate->GetCurrentContext();

#if _WIN32
	if( task->moved ) {
		//lprintf( "Event moved to callback..." );
		Local<Value> argv[1];
		if( !task->cbMove.IsEmpty() ) {
			argv[0] = task->moveSuccess?True( isolate ):False( isolate );
			Local<Function> cb = task->cbMove.Get( isolate );
			task->cbMove.Reset();
			cb->Call( context, task->_this.Get( isolate ), 1, argv );
		}
		task->moved = FALSE;
	}
	if( task->styled ){
		Local<Value> argv[1];

		if( !task->cbStyle.IsEmpty() ){
			argv[0] = Integer::New( isolate, task->styleSuccess );
			Local<Function> cb = task->cbStyle.Get( isolate );
			task->cbStyle.Reset();
			cb->Call( context, task->_this.Get( isolate ), 1, argv );
		}

		if( task->cbStyle.IsEmpty() && !task->moveOpts.IsEmpty() ){
			Local<Object> moveOpts = task->moveOpts.Get( isolate );
			task->moveOpts.Reset();
			doMoveWindow( isolate, context, task, NULL, moveOpts );
		}
		task->styled = FALSE;
	}
#endif
	if( task->ending ) {
		if( !task->endCallback.IsEmpty() )
			task->endCallback.Get( isolate )->Call( context, task->_this.Get( isolate ), 0, NULL );
		task->ending = FALSE;
		task->ended = TRUE;
		// these is a chance output will still come in?
		uv_close( (uv_handle_t*)&task->async, taskAsyncClosed );
	}
	{
		struct taskObjectOutputItem* output;
		struct taskObjectOutputItem* output2;
		output = NULL;
		output2 = NULL;
		while( ( output = (struct taskObjectOutputItem *)DequeLink( &task->output ) )
			|| ( output2 = (struct taskObjectOutputItem*)DequeLink( &task->output2 ) )
			) {
			Local<Value> argv[1];
			Local<ArrayBuffer> ab;
			if( task->binary ) {
				if( output ) {
#if ( NODE_MAJOR_VERSION >= 14 )
					std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( isolate, output->size );
					memcpy( bs->Data(), output->buffer, output->size );
					ab = ArrayBuffer::New( isolate, bs );
#else
					ab = ArrayBuffer::New( isolate, (void*)output->buffer, output->size );
#endif
				} else if( output2 ) {
#if ( NODE_MAJOR_VERSION >= 14 )
					std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( isolate, output2->size );
					memcpy( bs->Data(), output2->buffer, output2->size );
					ab = ArrayBuffer::New( isolate, bs );
#else
					ab = ArrayBuffer::New( isolate, (void*)output2->buffer, output2->size );
#endif
				}

				PARRAY_BUFFER_HOLDER holder = GetHolder();
				holder->o.Reset( isolate, ab );
				holder->o.SetWeak<ARRAY_BUFFER_HOLDER>( holder, releaseBuffer, WeakCallbackType::kParameter );
				holder->buffer = (void*)(output ? output : output2 ? output2 : NULL);

				argv[0] = ab;
				if( output2 )
					task->inputCallback2.Get( isolate )->Call( context, task->_this.Get( isolate ), 1, argv );
				if( output )
					task->inputCallback.Get( isolate )->Call( context, task->_this.Get( isolate ), 1, argv );
			}
			else {
				MaybeLocal<String> buf = localStringExternal( isolate, (const char*)(output?output->buffer:output2->buffer)
							, (int)(output?output->size:output2->size), (const char*)(output?output:output2) );
				argv[0] = buf.ToLocalChecked();
				if( output2 )
					task->inputCallback2.Get( isolate )->Call( context, task->_this.Get( isolate ), 1, argv );
				if( output )
					task->inputCallback.Get( isolate )->Call( context, task->_this.Get( isolate ), 1, argv );
				if( output ) {
					Deallocate( struct taskObjectOutputItem*, output );
					output = NULL;
				}
				if( output2 ) {
					Deallocate( struct taskObjectOutputItem*, output2 );
					output2 = NULL;
				}
			}
			//task->buffer = NULL;
		}

	}
	{
		// This is hook into Node to dispatch Promises() that are created... all event loops should have this.
		class constructorSet* c = getConstructors( isolate );
		Local<Function>cb = Local<Function>::New( isolate, c->ThreadObject_idleProc );
		cb->Call( isolate->GetCurrentContext(), Null( isolate ), 0, NULL );
	}
}

static void CPROC getTaskInput( uintptr_t psvTask, PTASK_INFO pTask, CTEXTSTR buffer, size_t size ) {
	TaskObject *task = (TaskObject*)psvTask;
	{
		struct taskObjectOutputItem *output = NewPlus( struct taskObjectOutputItem, size );
		memcpy( (char*)output->buffer, buffer, size );
		output->size = size;
		EnqueLink( &task->output, output );
		if( !task->ended )
			uv_async_send( &task->async );
	}

}

static void CPROC getTaskInput2( uintptr_t psvTask, PTASK_INFO pTask, CTEXTSTR buffer, size_t size ) {
	TaskObject* task = (TaskObject*)psvTask;
	{
		struct taskObjectOutputItem* output = NewPlus( struct taskObjectOutputItem, size );
		memcpy( (char*)output->buffer, buffer, size );
		output->size = size;
		EnqueLink( &task->output2, output );
		if( !task->ended )
			uv_async_send( &task->async );
	}

}


static void CPROC getTaskEnd( uintptr_t psvTask, PTASK_INFO task_ended ) {
	TaskObject *task = (TaskObject*)psvTask;
	task->ending = true;
	task->exitCode = GetTaskExitCode( task_ended );
	//task->waiter = NULL;
	task->task = NULL;
	//closes async
	if( !task->ended )
		uv_async_send( &task->async );
}

static void 	readEnv( Isolate* isolate, Local<Context> context, Local<Object> headers, PLIST *env ) {
	Local<Array> props = headers->GetPropertyNames( context ).ToLocalChecked();
	for( uint32_t p = 0; p < props->Length(); p++ ) {
		Local<Value> name = props->Get( context, p ).ToLocalChecked();
		Local<Value> value = headers->Get( context, name ).ToLocalChecked();
		String::Utf8Value localName( isolate, name );
		String::Utf8Value localValue( isolate, value );
		const int nameLen = localName.length();
		const size_t len = nameLen + localValue.length() + 2;
		struct environmentValue* val = NewArray( struct environmentValue, 1 );
		val->field = NewArray( TEXTCHAR, len );
		StrCpy( val->field, *localName );
		val->value = val->field + nameLen + 1;
		StrCpy( val->value, *localValue );

		AddLink( env, val );
	}
}


void TaskObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Must specify url and optionally protocols or options for client." ) ) );
		return;
	}

	if( args.IsConstructCall() ) {
		TaskObject *newTask = new TaskObject();
		AddLink( &l.tasks, newTask );

		Local<Object> _this = args.This();
		Local<Object> moveOpts;
		Local<Object> styleOpts;
		//try {
			newTask->_this.Reset( isolate, _this );
			newTask->Wrap( _this );
		//}
		//catch( const char *ex1 ) {
		//	isolate->ThrowException( Exception::Error(
		//		String::NewFromUtf8( isolate, TranslateText( ex1 ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		//}


		{
			Local<String> optName;
			struct optionStrings *strings = getStrings( isolate );
			Local<Object> opts = args[0]->ToObject( args.GetIsolate()->GetCurrentContext() ).ToLocalChecked();
			String::Utf8Value *args = NULL;
			String::Utf8Value *bin = NULL;
			String::Utf8Value *work = NULL;
			bool end = false;
			bool input = false;
			bool input2 = false;
			bool hidden = false;
			bool firstArgIsArg = true;
			bool newGroup = false;
			bool newConsole = false;
			bool suspend = false;
			bool useBreak = false;
			bool useSignal = false;
			bool noWindow = false;
			bool noKill = false;
			bool asAdmin = false;
			bool noWait = true;
			bool detach = false;
			bool noInheritStdio = false;

			newTask->killAtExit = true;

			char **argArray = NULL;
			PLIST envList = NULL;
			int nArg;

			
			if( opts->Has( context, optName = strings->noWindowString->Get( isolate ) ).ToChecked() ) {
				if( GETV( opts, optName )->IsBoolean() ) {
					noWindow = GETV( opts, optName )->TOBOOL( isolate );
				}
			}
#ifdef _WIN32
			if( opts->Has( context, optName = strings->moveToString->Get( isolate ) ).ToChecked() ) {
				if( GETV( opts, optName )->IsObject() ) {
					moveOpts = GETV( opts, optName ).As<Object>();
				}
			}
			if( opts->Has( context, optName = strings->styleString->Get( isolate ) ).ToChecked() ){
				if( GETV( opts, optName )->IsObject() ){
					styleOpts = GETV( opts, optName ).As<Object>();
				}
			}
#endif
			if( opts->Has( context, optName = strings->hiddenString->Get( isolate ) ).ToChecked() ) {
				if( GETV( opts, optName )->IsBoolean() ) {
					hidden = GETV( opts, optName )->TOBOOL( isolate );
				}
			}
			if( opts->Has( context, optName = strings->noKillString->Get( isolate ) ).ToChecked() ) {
				if( GETV( opts, optName )->IsBoolean() ) {
					noKill = GETV( opts, optName )->TOBOOL( isolate );
					if( noKill )
						newTask->killAtExit = false;
					else
						newTask->killAtExit = true;
				}
			}
			if( opts->Has( context, optName = strings->noWaitString->Get( isolate ) ).ToChecked() ) {
				if( GETV( opts, optName )->IsBoolean() ) {
					noWait = GETV( opts, optName )->TOBOOL( isolate );
				}
			}
			if( opts->Has( context, optName = strings->detachedString->Get( isolate ) ).ToChecked() ) {
				if( GETV( opts, optName )->IsBoolean() ) {
					detach = GETV( opts, optName )->TOBOOL( isolate );
				}
			}
			if( opts->Has( context, optName = strings->adminString->Get( isolate ) ).ToChecked() ) {
				if( GETV( opts, optName )->IsBoolean() ) {
					asAdmin = GETV( opts, optName )->TOBOOL( isolate );
				}
			}
			if( opts->Has( context, optName = strings->useBreakString->Get( isolate ) ).ToChecked() ) {
				if( GETV( opts, optName )->IsBoolean() ) {
					useBreak = GETV( opts, optName )->TOBOOL( isolate );
				}
			}
			if( opts->Has( context, optName = strings->useSignalString->Get( isolate ) ).ToChecked() ) {
				//lprintf( "use Signal..." );
				if( GETV( opts, optName )->IsBoolean() ) {
					useSignal = GETV( opts, optName )->TOBOOL( isolate );
				}
			}
			
			if( opts->Has( context, optName = strings->groupString->Get( isolate ) ).ToChecked() ) {
				if( GETV( opts, optName )->IsBoolean() ) {
					newGroup = GETV( opts, optName )->TOBOOL( isolate );
				}
			}
			if( opts->Has( context, optName = strings->noInheritStdio->Get( isolate ) ).ToChecked() ) {
				if( GETV( opts, optName )->IsBoolean() ) {
					noInheritStdio = GETV( opts, optName )->TOBOOL( isolate );
				}
			}
			if( opts->Has( context, optName = strings->consoleString->Get( isolate ) ).ToChecked() ) {
				if( GETV( opts, optName )->IsBoolean() ) {
					newConsole = GETV( opts, optName )->TOBOOL( isolate );
				}
			}

			if( opts->Has( context, optName = strings->binString->Get( isolate ) ).ToChecked() ) {
				Local<Value> val;
				if( GETV( opts, optName )->IsString() )
					bin = new String::Utf8Value( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
			} else {
				isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "required option 'bin' missing." ) ) );
			}
			if( opts->Has( context, optName = strings->argString->Get( isolate ) ).ToChecked() ) {
				Local<Value> val;
				if( GETV( opts, optName )->IsString() ) {
					char **args2;
					args = new String::Utf8Value( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
					ParseIntoArgs( *args[0], &nArg, &argArray );

					args2 = NewArray( char*, nArg + 1 );
			
					int n;
					for( n = 0; n < nArg; n++ )
						args2[n] = argArray[n];
					args2[n] = NULL;
					Release( argArray );
					argArray = args2;
				} else if( GETV( opts, optName )->IsArray() ) {
					uint32_t n;
					Local<Array> arr = Local<Array>::Cast( GETV( opts, optName ) );

					argArray = NewArray( char *, arr->Length() + 1 );
					for( n = 0; n < arr->Length(); n++ ) {
						argArray[n] = StrDup( *String::Utf8Value( USE_ISOLATE( isolate ) GETN( arr, n )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() ) );
					}
					argArray[n] = NULL;
				}
			}
			if( opts->Has( context, optName = strings->workString->Get( isolate ) ).ToChecked() ) {
				Local<Value> val;
				if( GETV( opts, optName )->IsString() )
					work = new String::Utf8Value( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
			}
			if( opts->Has( context, optName = strings->envString->Get( isolate ) ).ToChecked() ) {
				Local<Value> val;
				Local<Object> env = GETV( opts, optName ).As<Object>();
				readEnv( isolate, context, env, &envList );
				//lprintf( "env params not supported(yet)" );
				/*
				if( GETV( opts, optName )->IsString() ) {
					args = new String::Utf8Value( GETV( opts, optName )->ToString() );
				}
				*/
			}
			if( opts->Has( context, optName = strings->binaryString->Get( isolate ) ).ToChecked() ) {
				Local<Value> val;
				if( GETV( opts, optName )->IsBoolean() ) {
					newTask->binary = GETV( opts, optName )->TOBOOL( isolate );
				}
			}
			if( opts->Has( context, optName = strings->inputString->Get( isolate ) ).ToChecked() ) {
				Local<Value> val;
				if( GETV( opts, optName )->IsFunction() ) {
					newTask->inputCallback.Reset( isolate, Local<Function>::Cast( GETV( opts, optName ) ) );
					input = true;
				}
			}
			if( opts->Has( context, optName = strings->inputString2->Get( isolate ) ).ToChecked() ) {
				Local<Value> val;
				if( GETV( opts, optName )->IsFunction() ) {
					newTask->inputCallback2.Reset( isolate, Local<Function>::Cast( GETV( opts, optName ) ) );
					input2 = true;
				}
			}
			if( opts->Has( context, optName = strings->firstArgIsArgString->Get( isolate ) ).ToChecked() ) {
				firstArgIsArg = GETV( opts, optName )->TOBOOL( isolate );
			}
			if( opts->Has( context, optName = strings->endString->Get( isolate ) ).ToChecked() ) {
				Local<Value> val;
				if( GETV( opts, optName )->IsFunction() ) {
					newTask->endCallback.Reset( isolate, Local<Function>::Cast( GETV( opts, optName ) ) );
					end = true;
				}
			}

			if( input2 || input || end ) {
				class constructorSet *c = getConstructors( isolate );
				uv_async_init( c->loop, &newTask->async, taskAsyncMsg );
				newTask->async.data = newTask;
			}
			else if( !noWait ) {
				class constructorSet* c = getConstructors( isolate );
				uv_async_init( c->loop, &newTask->async, taskAsyncMsg );
				newTask->async.data = newTask;
			}
			/*
			#define LPP_OPTION_DO_NOT_HIDE           1
// for services to launch normal processes (never got it to work; used to work in XP/NT?)
#define LPP_OPTION_IMPERSONATE_EXPLORER  2
#define LPP_OPTION_FIRST_ARG_IS_ARG      4
#define LPP_OPTION_NEW_GROUP             8
#define LPP_OPTION_NEW_CONSOLE          16
#define LPP_OPTION_SUSPEND              32
			*/

			// if the option is specified
			// if both input methods are specified, the task has no stdio handles from this anyway
			// if it's a new popup console, it doesn't get handles from me either.
			if( noInheritStdio || ( input && input2 ) || ( newConsole && noKill && !input && !input2 ) ) {
#ifdef _WIN32
				//lprintf( "setting handle no-inherit" );
				SetHandleInformation( GetStdHandle( STD_INPUT_HANDLE ), HANDLE_FLAG_INHERIT, 0 );
				SetHandleInformation( GetStdHandle( STD_OUTPUT_HANDLE ), HANDLE_FLAG_INHERIT, 0 );
				SetHandleInformation( GetStdHandle( STD_ERROR_HANDLE ), HANDLE_FLAG_INHERIT, 0 );
#elif defined( __LINUX__ )
				//lprintf( "setting handle no-inherit" );
				int flags;
				flags = fcntl(0, F_GETFD);
				if( flags >= 0 ) fcntl(0, F_SETFD, flags|FD_CLOEXEC);
				flags = fcntl(1, F_GETFD);
				if( flags >= 0 ) fcntl(1, F_SETFD, flags|FD_CLOEXEC);
				flags = fcntl(2, F_GETFD);
				if( flags >= 0 ) fcntl(2, F_SETFD, flags|FD_CLOEXEC);
#endif
			}
			//lprintf( "What is this? %d %d %d %d %d", ( end || input || input2 || !noWait ), end, input, input2, !noWait );
			newTask->task = LaunchPeerProgram_v2( bin?*bin[0]:NULL
				, work?*work[0]:NULL
				, argArray
				, ( firstArgIsArg? LPP_OPTION_FIRST_ARG_IS_ARG:0 )
				| ( hidden?0:LPP_OPTION_DO_NOT_HIDE)
				| (newGroup? LPP_OPTION_NEW_GROUP : 0)
				| (newConsole ? LPP_OPTION_NEW_CONSOLE : 0)
				| (suspend? LPP_OPTION_SUSPEND : 0)
				| ( useBreak? LPP_OPTION_USE_CONTROL_BREAK :0 )
				| ( noWindow ? LPP_OPTION_NO_WINDOW : 0 )
				| ( hidden ? 0 : LPP_OPTION_DO_NOT_HIDE )
				| ( useSignal ? LPP_OPTION_USE_SIGNAL:0 )
				| ( detach ? LPP_OPTION_DETACH : 0 )
				| ( asAdmin ? LPP_OPTION_ELEVATE : 0 )
				, input ? getTaskInput : NULL
				, input2 ? getTaskInput2 : NULL
				, (end||input||input2||!noWait) ? getTaskEnd : NULL
				, (uintptr_t)newTask 
				, envList
				DBG_SRC );

			// if the option is specified
			// if both input methods are specified, the task has no stdio handles from this anyway
			if( noInheritStdio || ( input && input2 ) || ( newConsole && noKill && !input && !input2 ) ) {
#ifdef _WIN32
				//lprintf( "Resetting handles" );
				SetHandleInformation( GetStdHandle( STD_INPUT_HANDLE ), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT );
				SetHandleInformation( GetStdHandle( STD_OUTPUT_HANDLE ), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT );
				SetHandleInformation( GetStdHandle( STD_ERROR_HANDLE ), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT );
#elif defined( __LINUX__ )
				//lprintf( "setting handle no-inherit" );
				int flags;
				flags = fcntl(0, F_GETFD);
				if( flags >= 0 ) fcntl(0, F_SETFD, flags & ~FD_CLOEXEC);
				flags = fcntl(1, F_GETFD);
				if( flags >= 0 ) fcntl(1, F_SETFD, flags & ~FD_CLOEXEC);
				flags = fcntl(2, F_GETFD);
				if( flags >= 0 ) fcntl(2, F_SETFD, flags & ~FD_CLOEXEC);
#endif
			}

#ifdef _WIN32
			if( newTask->task && !styleOpts.IsEmpty() ){
				// hold move options until style completes...
				if( !moveOpts.IsEmpty() ) newTask->moveOpts.Reset( isolate, moveOpts );
				doStyleWindow( isolate, context, newTask, NULL, styleOpts );
			} else if( newTask->task && !moveOpts.IsEmpty() )
				doMoveWindow( isolate, context, newTask, NULL, moveOpts );
#endif

		}

		args.GetReturnValue().Set( _this );
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		int argc = args.Length();
		Local<Value> *argv = new Local<Value>[argc];
		for( int n = 0; n < argc; n++ )
			argv[n] = args[n];

		class constructorSet* c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->TaskObject_constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
		delete[] argv;
	}


}

ATEXIT( terminateStartedTasks ) {
	TaskObject *task;
	INDEX idx;
	int start = timeGetTime();
	int running;
	//lprintf( "task_module atexit" );
	do {
		running = FALSE;
		LIST_FORALL( l.tasks, idx, TaskObject*, task ) {
			if( task->killAtExit && !task->ended && task->task ) {
				//lprintf( "Terminate task? %p %s %d %d %d", task->task, task->killAtExit ? "Kill" : "noKil", task->ended, task->stopped, task->killed );
				running = TRUE;
				if( !task->stopped ) {
					//lprintf( "Generate stop program to task" );
					task->stopped = true;
					StopProgram( task->task );
				} else if( !task->killed ) {
					//lprintf( "Terminating agressively" );
					task->killed = TRUE;
					TerminateProgramEx( task->task, TERMINATE_PROGRAM_CHAIN );
				}
			}
		}
		if( running ) WakeableSleep( 50 );
	} while( running && ( ( timeGetTime() - start ) < 250 ) );
	//lprintf( "Atexit terminate tasks finished" );
}

void TaskObject::loadLibrary( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	String::Utf8Value s( USE_ISOLATE(isolate) args[0]->ToString( args.GetIsolate()->GetCurrentContext() ).ToLocalChecked() );
	if( LoadFunction( *s, NULL ) )
		args.GetReturnValue().Set( True( isolate ) );
	args.GetReturnValue().Set( False( isolate ) );

}

void TaskObject::Write( const v8::FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	TaskObject* task = Unwrap<TaskObject>( args.This() );
	String::Utf8Value s( USE_ISOLATE( args.GetIsolate() ) args[0]->ToString( args.GetIsolate()->GetCurrentContext() ).ToLocalChecked() );
	pprintf( task->task, "%s", *s );
}

void TaskObject::End( const v8::FunctionCallbackInfo<Value>& args ) {
	TaskObject* task = Unwrap<TaskObject>( args.This() );
	if( task && task->task )
		StopProgram( task->task );
}

void TaskObject::Terminate( const v8::FunctionCallbackInfo<Value>& args ) {
	TaskObject* task = Unwrap<TaskObject>( args.This() );
	if( task && task->task ) {
		//TerminateProgram( task->task );
		TerminateProgramEx( task->task, TERMINATE_PROGRAM_CHAIN );
	}
}

void TaskObject::isRunning( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	TaskObject* task = Unwrap<TaskObject>( args.This() );
	if( task && task->task )
		args.GetReturnValue().Set( (!task->ended) ? True( isolate ) : False( isolate ) );
	else
		args.GetReturnValue().Set( False( isolate ) );
}

void TaskObject::getExitCode( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	TaskObject* task = Unwrap<TaskObject>( args.This() );
	args.GetReturnValue().Set( Integer::New( args.GetIsolate(), (int)task->exitCode ) );
		
}

#if _WIN32

static void dropConsole( const FunctionCallbackInfo<Value>& args ) {
	FreeConsole();
}

static void moveTaskWindowResult( uintptr_t psv, LOGICAL success ){
	TaskObject *task = (TaskObject*)psv;
	//lprintf( "result... send event?" );
	task->moved = TRUE;
	task->moveSuccess = success;
	if( !task->ended )
		uv_async_send( &task->async );
}


// is static in prototype above
void doMoveWindow( Isolate*isolate, Local<Context> context, TaskObject *task, HWND hWnd, Local<Object> opts ) {
	struct optionStrings *strings = getStrings( isolate );
	Local<String> optName;

	int timeout = 500, left = 0, top = 0, width = 1920, height = 1080, display = -1, monitor = -1;

	if( opts->Has( context, optName = strings->cbString->Get( isolate ) ).ToChecked() ) {
		if( GETV( opts, optName )->IsFunction() ) {
			// should be reset to empty when not in use...
			task->cbMove.Reset( isolate, Local<Function>::Cast( GETV( opts, optName ) ) );
		}
	}

	if( opts->Has( context, optName = strings->timeoutString->Get( isolate ) ).ToChecked() ) {
		if( GETV( opts, optName )->IsNumber() ) {
			// should be reset to empty when not in use...
			timeout = (int)GETV( opts, optName )->IntegerValue(isolate->GetCurrentContext()).FromMaybe(0);
		}
	}

	if( opts->Has( context, optName = strings->displayString->Get( isolate ) ).ToChecked() ) {
		if( GETV( opts, optName )->IsNumber() ) {
			// should be reset to empty when not in use...
			display = (int)GETV( opts, optName )->IntegerValue(isolate->GetCurrentContext()).FromMaybe(0);
		}
	} else if( opts->Has( context, optName = strings->monitorString->Get( isolate ) ).ToChecked() ) {
		if( GETV( opts, optName )->IsNumber() ) {
			// should be reset to empty when not in use...
			monitor = (int)GETV( opts, optName )->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( 0 );
		}
	} else {
		if( opts->Has( context, optName = strings->xString->Get( isolate ) ).ToChecked() ) {
			if( GETV( opts, optName )->IsNumber() ) {
				// should be reset to empty when not in use...
				left = (int)GETV( opts, optName )->IntegerValue(isolate->GetCurrentContext()).FromMaybe(0);
			}
		} 
		if( opts->Has( context, optName = strings->yString->Get( isolate ) ).ToChecked() ) {
			if( GETV( opts, optName )->IsNumber() ) {
				// should be reset to empty when not in use...
				top = (int)GETV( opts, optName )->IntegerValue(isolate->GetCurrentContext()).FromMaybe(0);
			}
		} 
		if( opts->Has( context, optName = strings->widthString->Get( isolate ) ).ToChecked() ) {
			if( GETV( opts, optName )->IsNumber() ) {
				// should be reset to empty when not in use...
				width = (int)GETV( opts, optName )->IntegerValue(isolate->GetCurrentContext()).FromMaybe(0);
			}
		} 
		if( opts->Has( context, optName = strings->heightString->Get( isolate ) ).ToChecked() ) {
			if( GETV( opts, optName )->IsNumber() ) {
				// should be reset to empty when not in use...
				height = (int)GETV( opts, optName )->IntegerValue(isolate->GetCurrentContext()).FromMaybe(0);
			}
		} 
	}
	if( task ){
		if( display >= 0 )
			MoveTaskWindowToDisplay( task->task, timeout, display, moveTaskWindowResult, (uintptr_t)task );
		else if( monitor >= 0 )
			MoveTaskWindowToMonitor( task->task, timeout, display, moveTaskWindowResult, (uintptr_t)task );
		else {
			MoveTaskWindow( task->task, timeout, left, top, width, height, moveTaskWindowResult, (uintptr_t)task );
		}
	} else{
		//ShowWindow( hWnd, SW_RESTORE );
		SetWindowPos( hWnd, NULL, left, top, width, height, SWP_NOZORDER | SWP_NOOWNERZORDER );
	}
}

static void styleTaskWindowResult( uintptr_t psv, int success ){
	TaskObject* task = (TaskObject*)psv;
	//lprintf( "result... send event?" );
	task->styled = TRUE;
	task->styleSuccess = success;
	if( !task->ended )
		uv_async_send( &task->async );
}

static void doStyleWindow( Isolate* isolate, Local<Context> context, TaskObject* task, HWND hWnd, Local<Object> opts ){
	struct optionStrings* strings = getStrings( isolate );
	Local<String> optName;

	int timeout = 500, window = -1, windowEx = -1, classStyle = -1;

	if( opts->Has( context, optName = strings->cbString->Get( isolate ) ).ToChecked() ){
		if( GETV( opts, optName )->IsFunction() ){
			// should be reset to empty when not in use...
			task->cbStyle.Reset( isolate, Local<Function>::Cast( GETV( opts, optName ) ) );
		}
	}

	if( opts->Has( context, optName = strings->timeoutString->Get( isolate ) ).ToChecked() ){
		if( GETV( opts, optName )->IsNumber() ){
			// should be reset to empty when not in use...
			timeout = (int)GETV( opts, optName )->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( 0 );
		}
	}

	if( opts->Has( context, optName = strings->windowString->Get( isolate ) ).ToChecked() ){
		if( GETV( opts, optName )->IsNumber() ){
			// should be reset to empty when not in use...
			window = (int)GETV( opts, optName )->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( 0 );
		}
	}
	if( opts->Has( context, optName = strings->windowExString->Get( isolate ) ).ToChecked() ){
		if( GETV( opts, optName )->IsNumber() ){
			// should be reset to empty when not in use...
			windowEx = (int)GETV( opts, optName )->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( 0 );
		}
	}
	if( opts->Has( context, optName = strings->classString->Get( isolate ) ).ToChecked() ){
		if( GETV( opts, optName )->IsNumber() ){
			// should be reset to empty when not in use...
			classStyle = (int)GETV( opts, optName )->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( 0 );
		}
	}
	if( task ){
		StyleTaskWindow( task->task, timeout, window, windowEx, classStyle, styleTaskWindowResult, (uintptr_t)task );
	} else{
	}

}

void TaskObject::refreshWindow( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	TaskObject* task = Unwrap<TaskObject>( args.This() );
	HWND hWnd = RefreshTaskWindow( task->task );
	Local<ArrayBuffer> ab;
	std::shared_ptr<BackingStore> bs = ArrayBuffer::NewBackingStore( isolate, sizeof( hWnd ) );
	memcpy( bs->Data(), &hWnd, sizeof( hWnd ) );
	ab = ArrayBuffer::New( isolate, bs );
	args.GetReturnValue().Set( ab );
}

void TaskObject::moveWindow( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	TaskObject* task = Unwrap<TaskObject>( args.This() );

	Local<Object> opts = args[0]->ToObject( args.GetIsolate()->GetCurrentContext() ).ToLocalChecked();
	doMoveWindow( isolate, context, task, NULL, opts );
}

struct monitor_data {
	Isolate* isolate;
	Local<Context> context;
	Local<Array> array;
	int arrayIndex;
	struct optionStrings* strings;

};

static BOOL addMonitor( HMONITOR hMonitor,
	HDC hDC_null,
	LPRECT pRect,
	LPARAM dwParam
) {
	struct monitor_data* data = (struct monitor_data*)dwParam;
	Local<Context> context = data->context;
	Local<Object> display = Object::New( data->isolate );
	MONITORINFOEX monitorInfo;
	int devNum = 0;
	monitorInfo.cbSize = sizeof( monitorInfo );
	GetMonitorInfo( hMonitor, &monitorInfo );
	
	for( int numStart = 0; monitorInfo.szDevice[numStart]; numStart++ ) {
		if( monitorInfo.szDevice[numStart] >= '0' && monitorInfo.szDevice[numStart] <= '9' ) {
			devNum = atoi( monitorInfo.szDevice + numStart );
			break;
		}
	}

	//SETV( display, data->strings->displayString->Get( data->isolate ), Number::New( data->isolate, data->arrayIndex+1 ) );
	SETV( display, data->strings->displayString->Get( data->isolate ), Number::New( data->isolate, devNum ) );
	SETV( display, data->strings->xString->Get( data->isolate ), Number::New( data->isolate, pRect->left ) );
	SETV( display, data->strings->yString->Get( data->isolate ), Number::New( data->isolate, pRect->top ) );
	SETV( display, data->strings->widthString->Get( data->isolate ), Number::New( data->isolate, pRect->right-pRect->left ) );
	SETV( display, data->strings->heightString->Get( data->isolate ), Number::New( data->isolate, pRect->bottom-pRect->top ) );
	//SETV( display, data->strings->currentString->Get( data->isolate ), True( data->isolate ) );
	//SETV( display, data->strings->primaryString->Get( isolate ), ( dev.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE ) ? True( isolate ) : False( isolate ) );

	data->array->Set( context, data->arrayIndex++, display );

	return TRUE;
}

void TaskObject::getDisplays( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	Local<Object> result = Object::New( isolate );

	Local<Array> array_monitors = Array::New(isolate);
	Local<Array> array = Array::New( isolate );
	int arrayIndex = 0;
	Local<Object> display;// = Array::New( isolate );
	struct monitor_data data;
	int i;
	DISPLAY_DEVICE dev;
	DEVMODE dm;
	struct optionStrings* strings = getStrings( isolate );

	SETV( result, strings->monitorString->Get( isolate ), array_monitors );
	SETV( result, strings->deviceString->Get( isolate ), array );
	data.strings = strings;
	data.isolate = isolate;
	data.context = context;
	data.array = array_monitors;
	data.arrayIndex = 0;
	dm.dmSize = sizeof( DEVMODE );
	dm.dmDriverExtra = 0;
	dev.cb = sizeof( DISPLAY_DEVICE );
	{
		// go ahead and try to find V devices too... not sure what they are, but probably won't get to use them.
		EnumDisplayMonitors( NULL
			, NULL
			, addMonitor
			, (LPARAM)&data
		);

		for( i = 0;
			// all devices
			EnumDisplayDevices( NULL
				, i
				, &dev
				, 0
			); i++ ) {
			int numStart;
			if( EnumDisplaySettings( dev.DeviceName, ENUM_CURRENT_SETTINGS, &dm ) ) {
				if( dm.dmPelsWidth && dm.dmPelsHeight ) {
					for( numStart = 0; dev.DeviceName[numStart]; numStart++ ) {
						if( dev.DeviceName[numStart] >= '0' && dev.DeviceName[numStart] <= '9' ) break;
					}
					//if( l.flags.bLogDisplayEnumTest )
					display = Object::New( isolate );
					SETV( display, strings->displayString->Get( isolate ), Number::New( isolate, atoi( dev.DeviceName + numStart ) ) );
					SETV( display, strings->xString->Get( isolate ), Number::New( isolate, dm.dmPosition.x ) );
					SETV( display, strings->yString->Get( isolate ), Number::New( isolate, dm.dmPosition.y ) );
					SETV( display, strings->widthString->Get( isolate ), Number::New( isolate, dm.dmPelsWidth ) );
					SETV( display, strings->heightString->Get( isolate ), Number::New( isolate, dm.dmPelsHeight ) );			
					SETV( display, strings->currentString->Get( isolate ), True( isolate ) );
					SETV( display, strings->primaryString->Get( isolate ), (dev.StateFlags& DISPLAY_DEVICE_PRIMARY_DEVICE)?True( isolate ):False(isolate) );
					array->Set( context, arrayIndex++, display );
				}
				//lprintf( "display(cur) %s is at %d,%d %dx%d", dev.DeviceName, dm.dmPosition.x, dm.dmPosition.y, dm.dmPelsWidth, dm.dmPelsHeight );
			} else if( EnumDisplaySettings( dev.DeviceName, ENUM_REGISTRY_SETTINGS, &dm ) ) {
				//if( l.flags.bLogDisplayEnumTest )
				if( dm.dmPelsWidth && dm.dmPelsHeight ) {
					for( numStart = 0; dev.DeviceName[numStart]; numStart++ ) {
						if( dev.DeviceName[numStart] >= '0' && dev.DeviceName[numStart] <= '9' ) break;
					}
					//if( l.flags.bLogDisplayEnumTest )
					display = Object::New( isolate );
					SETV( display, strings->displayString->Get( isolate ), Number::New( isolate, atoi( dev.DeviceName + numStart ) ) );
					SETV( display, strings->xString->Get( isolate ), Number::New( isolate, dm.dmPosition.x ) );
					SETV( display, strings->yString->Get( isolate ), Number::New( isolate, dm.dmPosition.y ) );
					SETV( display, strings->widthString->Get( isolate ), Number::New( isolate, dm.dmPelsWidth ) );
					SETV( display, strings->heightString->Get( isolate ), Number::New( isolate, dm.dmPelsHeight ) );
					SETV( display, strings->currentString->Get( isolate ), False( isolate ) );
					SETV( display, strings->primaryString->Get( isolate ), ( dev.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE ) ? True( isolate ) : False( isolate ) );
					array->Set( context, arrayIndex++, display );
				}
				//lprintf( "display(reg) %s is at %d,%d %dx%d", dev.DeviceName, dm.dmPosition.x, dm.dmPosition.y, dm.dmPelsWidth, dm.dmPelsHeight );
			} else {
				lprintf( "Found display name, but enum current settings failed? %d", GetLastError() );
				continue;
			}
		}
	}

	args.GetReturnValue().Set( result );
}

void TaskObject::getWindowTitle( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	TaskObject* task = Unwrap<TaskObject>( args.This() );
	char *title = GetWindowTitle( task->task );
	Local<String> result = String::NewFromUtf8( isolate, title ).ToLocalChecked();
	Deallocate( char*, title );
	args.GetReturnValue().Set( result );
}
	


#endif

void TaskObject::GetProcessList( const FunctionCallbackInfo<Value>& args ) {
#ifdef _WIN32
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	struct optionStrings* strings = getStrings( isolate );
	PLIST procs;
	struct command_line_result* proc;
	INDEX idx;
	if( args.Length() > 0 ) {
		String::Utf8Value s( USE_ISOLATE( isolate ) args[0]->ToString( args.GetIsolate()->GetCurrentContext() ).ToLocalChecked() );
		procs = GetProcessCommandLines( *s );
	} else
		procs = GetProcessCommandLines( NULL );
	Local<Array> result = Array::New( isolate );
	LIST_FORALL( procs, idx, struct command_line_result*, proc ) {
		Local<Object> info = Object::New( isolate );
		Local<Array> args = Array::New( isolate );
		char* bin = proc->data;
		size_t binChars = 0;
		size_t argStart;
		size_t argEnd;
		int arg = 0;
		if( bin[0] == '"' ) {
			binChars++;
			while( bin[binChars] != '"' )
				binChars++;
		} else 
			while( bin[binChars] && bin[binChars] != ' ' )
				binChars++;
		argStart = binChars;
		if( bin[0] == '"' ) {
			bin++;
			binChars--;
		}
		while( bin[argStart] ) {
			while( bin[argStart] == ' ' ) argStart++;
			argEnd = argStart + 1;
			if( bin[argStart] == '"' ) {
				argStart++;
				while( bin[argEnd] && bin[argEnd] != '"' ) argEnd++;
			}else
				while( bin[argEnd] && bin[argEnd] != ' ' ) argEnd++;
			args->Set( context, arg++, String::NewFromUtf8( isolate, bin + argStart, NewStringType::kNormal, (int)(argEnd - argStart) ).ToLocalChecked() );
			if( bin[argEnd] == '"' )
				argStart = argEnd+1;
			else
				argStart = argEnd;
		}
		info->Set( context, String::NewFromUtf8Literal( isolate, "id" ), Integer::New( isolate, proc->dwProcessId ) );
		info->Set( context, strings->binString->Get( isolate ), String::NewFromUtf8( isolate, bin, NewStringType::kNormal, (int)binChars ).ToLocalChecked() );
		info->Set( context, strings->binaryString->Get( isolate ), String::NewFromUtf8( isolate, proc->processName ).ToLocalChecked() );
		info->Set( context, strings->argString->Get( isolate ), args );
		result->Set( context, (uint32_t)idx, info );
	}
	ReleaseCommandLineResults( &procs );
	args.GetReturnValue().Set( result );
#else
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	struct optionStrings* strings = getStrings( isolate );
	PLIST procs;
	struct command_line_result* proc;
	INDEX idx;
	if( args.Length() > 0 ) {
		String::Utf8Value s( USE_ISOLATE( isolate ) args[0]->ToString( args.GetIsolate()->GetCurrentContext() ).ToLocalChecked() );
		procs = GetProcessCommandLines( *s );
	} else
		procs = GetProcessCommandLines( NULL );
	Local<Array> result = Array::New( isolate );
	//lprintf( "procs:%p", procs );
	LIST_FORALL( procs, idx, struct command_line_result*, proc ) {
		Local<Object> info = Object::New( isolate );
		Local<Array> args = Array::New( isolate );
		char* bin = proc->cmd[0];
		int arg;
		for( arg = 1; arg < proc->length; arg++ ) {
			if( proc->cmd[arg] )
				args->Set( context, arg-1, String::NewFromUtf8( isolate, proc->cmd[arg] ).ToLocalChecked() );
		}

		info->Set( context, String::NewFromUtf8Literal( isolate, "id" ), Integer::New( isolate, proc->dwProcessId ) );
		info->Set( context, strings->binString->Get( isolate ), String::NewFromUtf8( isolate, bin ).ToLocalChecked() );
		info->Set( context, strings->binaryString->Get( isolate ), String::NewFromUtf8( isolate, bin ).ToLocalChecked() );
		info->Set( context, strings->argString->Get( isolate ), args );
		result->Set( context, (uint32_t)idx, info );

	}	

	ReleaseCommandLineResults( &procs );
	args.GetReturnValue().Set( result );

	//lprintf( "Process List is not available on this platform." );
#endif
}


#ifdef _WIN32
struct handle_data {
	unsigned long process_id;
	HWND window_handle;
	PLIST handles;
};

static BOOL is_main_window( HWND handle ) {
	return GetWindow( handle, GW_OWNER ) == (HWND)0 && IsWindowVisible( handle );
}
static BOOL CALLBACK enum_windows_callback( HWND handle, LPARAM lParam ) {
	struct handle_data* data = (struct handle_data*)lParam;
	unsigned long process_id = 0;
	GetWindowThreadProcessId( handle, &process_id );
	//if( data->process_id == process_id ) lprintf( "Found a window for process: %p %d %p", handle, IsWindowVisible( handle),  GetWindow( handle, GW_OWNER ) );
	if( data->process_id != process_id || !is_main_window( handle ) ) {
		//if( data->process_id == process_id && IsWindowVisible( handle ) )
		//	AddLink( &data->handles, handle );
		return TRUE;
	}
	//AddLink( &data->handles, handle );
	data->window_handle = handle;
	return FALSE;
}
static PLIST find_main_windows( unsigned long process_id ) {
	struct handle_data data;
	data.handles = NULL;
	data.process_id = process_id;
	data.window_handle = 0;
	EnumWindows( enum_windows_callback, (LPARAM)&data );
	return data.handles;
}
static HWND find_main_window( unsigned long process_id ) {
	struct handle_data data;
	data.handles = NULL;
	data.process_id = process_id;
	data.window_handle = 0;
	EnumWindows( enum_windows_callback, (LPARAM)&data );
	return data.window_handle;
}

static void setProcessWindowStyles( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	if( !args[0]->IsInt32() ) return;
	int32_t id = (int32_t)args[0]->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( 0 );
	int64_t winStyles = args[1]->IsNumber() ? args[1]->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( -1 ) : -1;
	int64_t winStylesEx = args[2]->IsNumber() ? args[2]->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( -1 ) : -1;
	int64_t classStyles = args[3]->IsNumber() ? args[3]->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( -1 ) : -1;
	HWND hWnd = find_main_window( id );
	//lprintf( "Set Values: %08x %08x %08x", winStyles, winStylesEx, classStyles );
	if( winStyles != -1 )
		SetWindowLongPtr( hWnd, GWL_STYLE, winStyles );
	if( winStylesEx != -1 )
		SetWindowLongPtr( hWnd, GWL_EXSTYLE, winStylesEx );
	if( classStyles != -1 )
		SetClassLongPtr( hWnd, GCL_STYLE, classStyles );
	SetWindowPos( hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER );
	InvalidateRect( hWnd, NULL, TRUE );
}

static void getProcessWindowStyles( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	if( !args[0]->IsInt32() ) return;
	int32_t id = (int32_t)args[0]->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( 0 );
	HWND hWnd = find_main_window( id );
	int32_t winStyles = (int32_t)GetWindowLongPtr( hWnd, GWL_STYLE );
	int32_t winStylesEx = (int32_t)GetWindowLongPtr( hWnd, GWL_EXSTYLE );
	int32_t classStyles = (int32_t)GetClassLongPtr( hWnd, GCL_STYLE );
	Local<Object> styles = Object::New( isolate );
	SET_READONLY( styles, "window", Integer::New( isolate, winStyles ) );
	SET_READONLY( styles, "windowEx", Integer::New( isolate, winStylesEx ) );
	SET_READONLY( styles, "class", Integer::New( isolate, classStyles ) );
	args.GetReturnValue().Set( styles );
}

static void getProcessWindowPos( const FunctionCallbackInfo<Value>& args ){
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	if( !args[0]->IsInt32() ) return;
	int32_t id = (int32_t)args[0]->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( 0 );
	HWND hWnd = find_main_window( id );
	Local<Object> position = Object::New( isolate );
	RECT rect;
	GetWindowRect( hWnd, &rect );
	SETVAR( position, "x", Integer::New( isolate, rect.left ) );
	SETVAR( position, "y", Integer::New( isolate, rect.top ) );
	SETVAR( position, "width", Integer::New( isolate, rect.right-rect.left ) );
	SETVAR( position, "height", Integer::New( isolate, rect.bottom-rect.top ) );
	args.GetReturnValue().Set( position );
}

static void setProcessWindowPos( const FunctionCallbackInfo<Value>& args ){
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	if( !args[0]->IsInt32() ) return;
	int32_t id = (int32_t)args[0]->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( 0 );
	Local<Object> opts = Local<Object>::Cast( args[1] );
	HWND hWnd = find_main_window( id );
	doMoveWindow( isolate, context, NULL, hWnd, opts );
	/*
	PLIST handles = find_main_windows( id );
	HWND hWnd;
	INDEX idx;
	LIST_FORALL( handles, idx, HWND, hWnd )  {
		//lprintf( "Moving handle %p", hWnd );
		doMoveWindow( isolate, context, NULL, hWnd, opts );
	}
	*/

}

void TaskObject::styleWindow( const FunctionCallbackInfo<Value>& args ){
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	TaskObject* task = Unwrap<TaskObject>( args.This() );

	Local<Object> opts = args[0]->ToObject( args.GetIsolate()->GetCurrentContext() ).ToLocalChecked();
	doStyleWindow( isolate, context, task, NULL, opts );
}

void TaskObject::getProcessWindowPos( const FunctionCallbackInfo<Value>& args ){
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	TaskObject* task = Unwrap<TaskObject>( args.This() );
	HWND hWnd = RefreshTaskWindow( task->task );
	Local<Object> position = Object::New( isolate );
	RECT rect;
	GetWindowRect( hWnd, &rect );
	SETVAR( position, "x", Integer::New( isolate, rect.left ) );
	SETVAR( position, "y", Integer::New( isolate, rect.top ) );
	SETVAR( position, "width", Integer::New( isolate, rect.right - rect.left ) );
	SETVAR( position, "height", Integer::New( isolate, rect.bottom - rect.top ) );
	args.GetReturnValue().Set( position );
}

void TaskObject::setProcessWindowStyles( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	int64_t winStyles = args[1]->IsNumber() ? args[1]->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( -1 ) : -1;
	int64_t winStylesEx = args[2]->IsNumber() ? args[2]->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( -1 ) : -1;
	int64_t classStyles = args[3]->IsNumber() ? args[3]->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( -1 ) : -1;
	TaskObject* task = Unwrap<TaskObject>( args.This() );
	HWND hWnd = RefreshTaskWindow( task->task );
	if( winStyles != -1 )
		SetWindowLongPtr( hWnd, GWL_STYLE, winStyles );
	if( winStylesEx != -1 )
		SetWindowLongPtr( hWnd, GWL_EXSTYLE, winStylesEx );
	if( classStyles != -1 )
		SetClassLongPtr( hWnd, GCL_STYLE, classStyles );

	SetWindowPos( hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER );
	InvalidateRect( hWnd, NULL, TRUE );
}

void TaskObject::getProcessWindowStyles( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	TaskObject* task = Unwrap<TaskObject>( args.This() );
	HWND hWnd = RefreshTaskWindow( task->task );
	int32_t winStyles = (int32_t)GetWindowLongPtr( hWnd, GWL_STYLE );
	int32_t winStylesEx = (int32_t)GetWindowLongPtr( hWnd, GWL_EXSTYLE );
	int32_t classStyles = (int32_t)GetClassLongPtr( hWnd, GCL_STYLE );
	Local<Object> styles = Object::New( isolate );
	SET_READONLY( styles, "window", Integer::New( isolate, winStyles ) );
	SET_READONLY( styles, "windowEx", Integer::New( isolate, winStylesEx ) );
	SET_READONLY( styles, "class", Integer::New( isolate, classStyles ) );
	args.GetReturnValue().Set( styles );
}

static void getProcessWindowTitle( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	int32_t id = (int32_t)args[0]->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( 0 );
	HWND hWnd = find_main_window( id );
	char* str = NewArray( char, 256 );
	if( hWnd ) {
		GetWindowText( hWnd, str, 256 );
	} else
		StrCpy( str, "No Window" );

	Local<String> result = String::NewFromUtf8( isolate, str ).ToLocalChecked();
	Deallocate( char*, str );
	args.GetReturnValue().Set( result );
}


#endif



void TaskObject::StopProcess( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	if( args.Length() < 1 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Must specify process ID to terminate." ) ) );
		return;
	}
	int32_t id = (int32_t)args[0]->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( 0 );
	int32_t code = (int32_t)(args.Length() > 1 ? args[1]->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( 0 ) : (int64_t)0);


#ifdef _WIN32
	char* name = NULL;
	if( args.Length() > 2 ) {
		String::Utf8Value s( USE_ISOLATE( isolate ) args[2]->ToString( args.GetIsolate()->GetCurrentContext() ).ToLocalChecked() );
		name = StrDup( *s );
	}
	HWND hWndMain = find_main_window( id );
	if( hWndMain ) {
		TEXTCHAR title[256];
		GetWindowText( hWndMain, title, 256 );
		//lprintf( "Sending WM_CLOSE to %d %p %s", id, hWndMain, title );
		SendMessage( hWndMain, WM_CLOSE, 0, 0 );
	} else if( !( code & 2 ) ) {
		//lprintf( "Killing child %d? %d", id, code );
		//MessageBox( NULL, "pause", "pause", MB_OK );
		FreeConsole();
		BOOL a = AttachConsole( id );
		if( !a ) {
			DWORD dwError = GetLastError();
			lprintf( "Failed to attachConsole %d %d %d", a, dwError, id );
		}
		if( code == 0 )
			if( !GenerateConsoleCtrlEvent( CTRL_C_EVENT, id ) ) {
				DWORD error = GetLastError();
				lprintf( "Failed to send CTRL_C_EVENT %d %d", id, error );
			}// else lprintf( "Success sending ctrl C?" );
		else
			if( !GenerateConsoleCtrlEvent( CTRL_BREAK_EVENT, id ) ) {
				DWORD error = GetLastError();
				lprintf( "Failed to send CTRL_BREAK_EVENT %d %d", id, error );
			}// else lprintf( "Success sending ctrl break?" );
	}
	// this is pretty niche; was an attempt to handle when ctrl-break and ctrl-c events failed.
	else if( code & 2 ) {
		//lprintf ( "code?  %d %d %p", id, code, name );
		if( !name ) {
			lprintf( "To kill using signal, must include the process name as well as ID" );
			return;
		}
		char eventName[256];
		HANDLE hEvent;
		snprintf( eventName, 256, "Global\\%s(%d):exit", name, id );
		ReleaseEx( name DBG_SRC );
		hEvent = OpenEvent( EVENT_MODIFY_STATE, FALSE, eventName );
		//lprintf( "Signal process event: %s", eventName );
		if( hEvent != NULL ) {
			//lprintf( "Opened event:%p %s %d", hEvent, eventName, GetLastError() );
			if( !SetEvent( hEvent ) ) {
				lprintf( "Failed to set event? %d", GetLastError() );
			}
			CloseHandle( hEvent );
		}
	}
#else
	if( code )
		kill( id, SIGHUP );
	else
		kill( id, SIGINT );
#endif
}


void GetProcessId( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	THREAD_ID tid = GetMyThreadID();
	uint32_t pid = ((uint64_t)tid) >> 32;
	args.GetReturnValue().Set( Integer::New( isolate, pid ) );	
}

void GetProcessParentId( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	int32_t id = (int32_t)args[0]->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( 0 );
	if( id > 0 ) {
		int pid = GetProcessParent( id );
		args.GetReturnValue().Set( Integer::New( isolate, pid ) );	
	}
}

void TaskObject::KillProcess( const FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	if( args.Length() < 1 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8Literal( isolate, "Must specify process ID to terminate." ) ) );
		return;
	}
	int32_t id = (int32_t)args[0]->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( 0 );
	int64_t code = (int32_t)(args.Length() > 1 ? args[1]->IntegerValue( isolate->GetCurrentContext() ).FromMaybe( 0 ) : (int64_t)0);
#ifdef _WIN32
	HANDLE hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, id );
	if( hProcess ) {
		TerminateProcess( hProcess, (UINT)code );
		CloseHandle( hProcess );
	} else {
		DWORD dwError = GetLastError();
		lprintf( "Failed to open process %d:%d", id, dwError );
	}
#else
	kill( id, SIGKILL );
#endif
}