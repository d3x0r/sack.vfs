
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
	Eternal<String> *endString;
	Eternal<String> *firstArgIsArgString;
};


static struct local {
	PLIST tasks;
} l;

v8::Persistent<v8::Function> TaskObject::constructor;

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
		check->workString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "work", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->binString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "bin", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->argString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "args", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->envString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "env", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->binaryString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "binary", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->inputString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "input", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->endString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "end", v8::NewStringType::kNormal ).ToLocalChecked() );
		check->firstArgIsArgString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "firstArgIsArg", v8::NewStringType::kNormal ).ToLocalChecked() );
	}
	return check;
}

TaskObject::TaskObject():_this(), endCallback(), inputCallback() 
{
    task = NULL;
    binary = false;
    ending = false;
    ended = false;
    exitCode = 0;
    killAtExit = false;
	output = CreateLinkQueue();
    
	//this[0] = _blankTask;
}

TaskObject::~TaskObject() {
	DeleteLink( &l.tasks, this );
	if( task && !ended ) {
		StopProgram( task );
	}
}

void InitTask( Isolate *isolate, Local<Object> exports ) {

	Local<FunctionTemplate> taskTemplate;
	taskTemplate = FunctionTemplate::New( isolate, TaskObject::New );
	taskTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.task", v8::NewStringType::kNormal ).ToLocalChecked() );
	taskTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "write", TaskObject::Write );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "send", TaskObject::Write );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "end", TaskObject::End );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "terminate", TaskObject::Terminate );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "isRunning", TaskObject::isRunning );
	taskTemplate->ReadOnlyPrototype();
	TaskObject::constructor.Reset( isolate, taskTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
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
		while( output = (struct taskObjectOutputItem *)DequeLink( &task->output ) ) {
			Local<Value> argv[1];
			Local<ArrayBuffer> ab;
			if( task->binary ) {
				ab = ArrayBuffer::New( isolate, (void*)output->buffer, output->size );
				argv[0] = ab;
				task->inputCallback.Get( isolate )->Call( context, task->_this.Get( isolate ), 1, argv );
			}
			else {
				MaybeLocal<String> buf = localStringExternal( isolate, (const char*)output->buffer, (int)output->size, (const char*)output );
				argv[0] = buf.ToLocalChecked();
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
	//if( !task->inputCallback.IsEmpty() ) 
	{
		struct taskObjectOutputItem *output = NewPlus( struct taskObjectOutputItem, size );
		//task->buffer = NewArray( char, size );
		memcpy( (char*)output->buffer, buffer, size );
		output->size = size;
		//output->waiter = MakeThread();
		EnqueLink( &task->output, output );
		uv_async_send( &task->async );
		//while( task->buffer ) {
		//	WakeableSleep( 200 );
		//}
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

void TaskObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Must specify url and optionally protocols or options for client.", v8::NewStringType::kNormal ).ToLocalChecked() ) );
		return;
	}

	if( args.IsConstructCall() ) {
		TaskObject *newTask = new TaskObject();
		AddLink( &l.tasks, newTask );

		Local<Object> _this = args.This();
		try {
			newTask->_this.Reset( isolate, _this );
			newTask->Wrap( _this );
		}
		catch( const char *ex1 ) {
			isolate->ThrowException( Exception::Error(
				String::NewFromUtf8( isolate, TranslateText( ex1 ), v8::NewStringType::kNormal ).ToLocalChecked() ) );
		}


		{
			Local<String> optName;
			struct optionStrings *strings = getStrings( isolate );
			Local<Object> opts = args[0]->ToObject( args.GetIsolate()->GetCurrentContext() ).ToLocalChecked();
			String::Utf8Value *args = NULL;
			String::Utf8Value *bin = NULL;
			String::Utf8Value *work = NULL;
			bool end = false;
			bool input = false;
			bool hidden = false;
			bool firstArgIsArg = true;
			bool newGroup = false;
			bool newConsole = false;
			bool suspend = false;

			newTask->killAtExit = true;

			char **argArray = NULL;
			int nArg;

			if( opts->Has( context, optName = strings->binString->Get( isolate ) ).ToChecked() ) {
				Local<Value> val;
				if( GETV( opts, optName )->IsString() )
					bin = new String::Utf8Value( USE_ISOLATE( isolate ) GETV( opts, optName )->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
			} else {
				isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "required option 'bin' missing.", v8::NewStringType::kNormal ).ToLocalChecked() ) );
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
				lprintf( "env params not supported(yet)" );
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

			if( input || end ) {
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

			newTask->task = LaunchPeerProgramExx( bin?*bin[0]:NULL
				, work?*work[0]:NULL
				, argArray
				, ( firstArgIsArg? LPP_OPTION_FIRST_ARG_IS_ARG:0 )
				| ( hidden?0:LPP_OPTION_DO_NOT_HIDE)
				| (newGroup? LPP_OPTION_NEW_GROUP : 0)
				| (newConsole ? LPP_OPTION_NEW_CONSOLE : 0)
				| (suspend? LPP_OPTION_SUSPEND : 0)
				, input ? getTaskInput : NULL
				, (end||input) ? getTaskEnd : NULL
				, (uintptr_t)newTask DBG_SRC );

		}

		args.GetReturnValue().Set( _this );
	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		int argc = args.Length();
		Local<Value> *argv = new Local<Value>[argc];
		for( int n = 0; n < argc; n++ )
			argv[n] = args[n];

		Local<Function> cons = Local<Function>::New( isolate, constructor );
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), argc, argv ).ToLocalChecked() );
		delete[] argv;
	}


}

ATEXIT( terminateStartedTasks ) {
	TaskObject *task;
	INDEX idx;
	LIST_FORALL( l.tasks, idx, TaskObject *, task ) {
		if( task->killAtExit && ! task->ended )
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
	args.GetReturnValue().Set( (!task->ended) ? True( isolate ) : False( isolate ) );
}
