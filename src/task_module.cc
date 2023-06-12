
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
};


static struct local {
	PLIST tasks;
} l;

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
	}
	return check;
}

TaskObject::TaskObject():_this(), endCallback(), inputCallback(), inputCallback2()
{
    task = NULL;
    binary = false;
    ending = false;
    ended = false;
    exitCode = 0;
    killAtExit = false;
	output = CreateLinkQueue();
	output2 = CreateLinkQueue();

	//this[0] = _blankTask;
}

TaskObject::~TaskObject() {
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
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "write", TaskObject::Write );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "send", TaskObject::Write );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "end", TaskObject::End );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "terminate", TaskObject::Terminate );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "isRunning", TaskObject::isRunning );
	taskTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8Literal( isolate, "exitCode" )
		, FunctionTemplate::New( isolate, TaskObject::getExitCode )
		, Local<FunctionTemplate>() );
	taskTemplate->ReadOnlyPrototype();
	class constructorSet* c = getConstructors( isolate );
	c->TaskObject_constructor.Reset( isolate, taskTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	Local<Function> taskF;
	SET_READONLY( exports, "Task", taskF = taskTemplate->GetFunction( isolate->GetCurrentContext() ).ToLocalChecked() );
	SET_READONLY_METHOD( taskF, "loadLibrary", TaskObject::loadLibrary );
}

static void taskAsyncMsg( uv_async_t* handle ) {
	TaskObject *task = (TaskObject*)handle->data;
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	HandleScope scope( isolate );
	Local<Context> context = isolate->GetCurrentContext();

	if( task->ending ) {
		if( !task->endCallback.IsEmpty() )
			task->endCallback.Get( isolate )->Call( context, task->_this.Get( isolate ), 0, NULL );
		task->ending = FALSE;
		task->ended = TRUE;
		// these is a chance output will still come in?
		uv_close( (uv_handle_t*)&task->async, NULL );
	}
	{
		struct taskObjectOutputItem *output;
		struct taskObjectOutputItem* output2 = NULL;
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
			}
			//task->buffer = NULL;
		}

	}
	{
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

			newTask->killAtExit = true;

			char **argArray = NULL;
			PLIST envList = NULL;
			int nArg;

			
			if( opts->Has( context, optName = strings->useBreakString->Get( isolate ) ).ToChecked() ) {
				if( GETV( opts, optName )->IsBoolean() ) {
					useBreak = GETV( opts, optName )->TOBOOL( isolate );
				}
			}
			if( opts->Has( context, optName = strings->groupString->Get( isolate ) ).ToChecked() ) {
				if( GETV( opts, optName )->IsBoolean() ) {
					newGroup = GETV( opts, optName )->TOBOOL( isolate );
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
			/*
			#define LPP_OPTION_DO_NOT_HIDE           1
// for services to launch normal processes (never got it to work; used to work in XP/NT?)
#define LPP_OPTION_IMPERSONATE_EXPLORER  2
#define LPP_OPTION_FIRST_ARG_IS_ARG      4
#define LPP_OPTION_NEW_GROUP             8
#define LPP_OPTION_NEW_CONSOLE          16
#define LPP_OPTION_SUSPEND              32
			*/

			newTask->task = LaunchPeerProgram_v2( bin?*bin[0]:NULL
				, work?*work[0]:NULL
				, argArray
				, ( firstArgIsArg? LPP_OPTION_FIRST_ARG_IS_ARG:0 )
				| ( hidden?0:LPP_OPTION_DO_NOT_HIDE)
				| (newGroup? LPP_OPTION_NEW_GROUP : 0)
				| (newConsole ? LPP_OPTION_NEW_CONSOLE : 0)
				| (suspend? LPP_OPTION_SUSPEND : 0)
				| ( useBreak? LPP_OPTION_USE_CONTROL_BREAK :0 )
				, input ? getTaskInput : NULL
				, input2 ? getTaskInput2 : NULL
				, (end||input) ? getTaskEnd : NULL
				, (uintptr_t)newTask 
				, envList
				DBG_SRC );
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
	LIST_FORALL( l.tasks, idx, TaskObject *, task ) {
		if( task->killAtExit && ! task->ended && task->task )
			StopProgram( task->task );
	}
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
	if( task && task->task )
		TerminateProgram( task->task );
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