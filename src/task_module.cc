
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
	uv_loop_t* loop;
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
		check->workString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "work" ) );
		check->binString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "bin" ) );
		check->argString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "args" ) );
		check->envString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "env" ) );
		check->binaryString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "binary" ) );
		check->inputString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "input" ) );
		check->endString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "end" ) );
		check->firstArgIsArgString = new Eternal<String>( isolate, String::NewFromUtf8( isolate, "firstArgIsArg" ) );
	}
	return check;
}

static TaskObject _blankTask;

TaskObject::TaskObject() {
	memcpy( this, &_blankTask, sizeof( *this ) );
}

TaskObject::~TaskObject() {
	DeleteLink( &l.tasks, this );
	if( task && !ended ) {
		StopProgram( task );
	}
}

void InitTask( Isolate *isolate, Handle<Object> exports ) {
	if( !l.loop )
		l.loop = uv_default_loop();

	Local<FunctionTemplate> taskTemplate;
	taskTemplate = FunctionTemplate::New( isolate, TaskObject::New );
	taskTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.task" ) );
	taskTemplate->InstanceTemplate()->SetInternalFieldCount( 1 );  // need 1 implicit constructor for wrap
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "write", TaskObject::Write );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "send", TaskObject::Write );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "end", TaskObject::End );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "terminate", TaskObject::Terminate );
	NODE_SET_PROTOTYPE_METHOD( taskTemplate, "isRunning", TaskObject::isRunning );
	taskTemplate->ReadOnlyPrototype();
	TaskObject::constructor.Reset( isolate, taskTemplate->GetFunction() );
	SET_READONLY( exports, "Task", taskTemplate->GetFunction() );
}

static void taskAsyncMsg( uv_async_t* handle ) {
	TaskObject *task = (TaskObject*)handle->data;
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	HandleScope scope( isolate );

	if( task->ending ) {
		if( !task->endCallback.IsEmpty() )
			task->endCallback.Get( isolate )->Call( task->_this.Get( isolate ), 0, NULL );
		task->ending = FALSE;
		task->ended = TRUE;
		// these is a chance output will still come in?
		uv_close( (uv_handle_t*)&task->async, NULL );
	}
	if( task->buffer ) {
		Local<Value> argv[1];
		Local<ArrayBuffer> ab;
		if( task->binary ) {
			ab = ArrayBuffer::New( isolate, (void*)task->buffer, task->size );
			argv[0] = ab;
			task->inputCallback.Get( isolate )->Call( task->_this.Get( isolate ), 1, argv );
		}
		else {
			MaybeLocal<String> buf = localString( isolate, (const char*)task->buffer, (int)task->size );
			argv[0] = buf.ToLocalChecked();
			task->inputCallback.Get( isolate )->Call( task->_this.Get( isolate ), 1, argv );
		}

		task->buffer = NULL;
	}
	if( task->waiter ) {
		WakeThread( task->waiter );
	}
}

static void CPROC getTaskInput( uintptr_t psvTask, PTASK_INFO pTask, CTEXTSTR buffer, size_t size ) {
	TaskObject *task = (TaskObject*)psvTask;
	//if( !task->inputCallback.IsEmpty() ) 
	{
		task->buffer = buffer;
		task->size = size;
		task->waiter = MakeThread();
		uv_async_send( &task->async );
		while( task->buffer ) {
			WakeableSleep( 200 );
		}
	}

}

static void CPROC getTaskEnd( uintptr_t psvTask, PTASK_INFO task_ended ) {
	TaskObject *task = (TaskObject*)psvTask;
	task->ending = true;
	task->exitCode = GetTaskExitCode( task_ended );
	task->waiter = NULL;
	task->task = NULL;
	//closes async
	uv_async_send( &task->async );
}

void TaskObject::New( const v8::FunctionCallbackInfo<Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	int argc = args.Length();
	if( argc == 0 ) {
		isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "Must specify url and optionally protocols or options for client." ) ) );
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
				String::NewFromUtf8( isolate, TranslateText( ex1 ) ) ) );
		}


		{
			Local<String> optName;
			struct optionStrings *strings = getStrings( isolate );
			Local<Object> opts = args[0]->ToObject();
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

			char **argArray;
			int nArg;

			if( opts->Has( optName = strings->binString->Get( isolate ) ) ) {
				Local<Value> val;
				if( opts->Get( optName )->IsString() )
					bin = new String::Utf8Value( USE_ISOLATE( isolate ) opts->Get( optName )->ToString() );
			} else {
				isolate->ThrowException( Exception::Error( String::NewFromUtf8( isolate, "required option 'bin' missing." ) ) );			
			}
			if( opts->Has( optName = strings->argString->Get( isolate ) ) ) {
				Local<Value> val;
				if( opts->Get( optName )->IsString() ) {
					char **args2;
					args = new String::Utf8Value( USE_ISOLATE( isolate ) opts->Get( optName )->ToString() );
					ParseIntoArgs( *args[0], &nArg, &argArray );

					args2 = NewArray( char*, nArg + 1 );
					int n;
					for( n = 0; n < nArg; n++ )
						args2[n] = argArray[n];
					args2[n] = NULL;
					Release( argArray );
					argArray = args2;
				} else if( opts->Get( optName )->IsArray() ) {
					uint32_t n;
					Local<Array> arr = Local<Array>::Cast( opts->Get( optName ) );

					argArray = NewArray( char *, arr->Length() + 1 );
					for( n = 0; n < arr->Length(); n++ ) {
						argArray[n] = StrDup( *String::Utf8Value( USE_ISOLATE( isolate ) arr->Get( n )->ToString() ) );
					}
					argArray[n] = NULL;
				}
			}
			if( opts->Has( optName = strings->workString->Get( isolate ) ) ) {
				Local<Value> val;
				if( opts->Get( optName )->IsString() )
					work = new String::Utf8Value( USE_ISOLATE( isolate ) opts->Get( optName )->ToString() );
			}
			if( opts->Has( optName = strings->envString->Get( isolate ) ) ) {
				Local<Value> val;
				lprintf( "env params not supported(yet)" );
				/*
				if( opts->Get( optName )->IsString() ) {
					args = new String::Utf8Value( opts->Get( optName )->ToString() );
				}
				*/
			}
			if( opts->Has( optName = strings->binaryString->Get( isolate ) ) ) {
				Local<Value> val;
				if( opts->Get( optName )->IsBoolean() ) {
					newTask->binary = opts->Get( optName )->BooleanValue();
				}
			}
			if( opts->Has( optName = strings->inputString->Get( isolate ) ) ) {
				Local<Value> val;
				if( opts->Get( optName )->IsFunction() ) {
					newTask->inputCallback.Reset( isolate, Handle<Function>::Cast( opts->Get( optName ) ) );
					input = true;
				}
			}
			if( opts->Has( optName = strings->firstArgIsArgString->Get( isolate ) ) ) {
				firstArgIsArg = opts->Get( optName )->BooleanValue();
			}
			if( opts->Has( optName = strings->endString->Get( isolate ) ) ) {
				Local<Value> val;
				if( opts->Get( optName )->IsFunction() ) {
					newTask->endCallback.Reset( isolate, Handle<Function>::Cast( opts->Get( optName ) ) );
					end = true;
				}
			}

			if( input || end ) {
				uv_async_init( l.loop, &newTask->async, taskAsyncMsg );
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

void TaskObject::Write( const v8::FunctionCallbackInfo<Value>& args ) {
	//Isolate* isolate = args.GetIsolate();
	TaskObject* task = Unwrap<TaskObject>( args.This() );
	String::Utf8Value s( args[0]->ToString() );
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
