#include "global.h"



class textWrapper : public node::ObjectWrap {
public:
	PTEXT segment;
	//Persistent<Object> state;
	textWrapper() : ObjectWrap() {
		segment = NULL;
	}
	~textWrapper() {
		LineRelease( segment );
	}
	void textWrapSelf( Isolate* isolate, textWrapper *_this, Local<Object> into ) {
		_this->Wrap( into );
		//_this->state.Reset( isolate, into );
	}
};



static void newText( const FunctionCallbackInfo<Value>& args ) {
	Isolate *isolate = args.GetIsolate();
	if( args.IsConstructCall() ) {
		textWrapper* obj = new textWrapper();
		obj->textWrapSelf( isolate, obj, args.This() );
		args.GetReturnValue().Set( args.This() );

	}
	else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		class constructorSet *c = getConstructors( isolate );
		Local<Function> cons = Local<Function>::New( isolate, c->textConstructor );
		Local<Value> *passArgs = new Local<Value>[args.Length()];
		int n;
		for( n = 0; n < args.Length(); n++ )
			passArgs[n] = args[n];
		args.GetReturnValue().Set( cons->NewInstance( isolate->GetCurrentContext(), n, passArgs ).ToLocalChecked() );
		delete passArgs;
	}

}

static void getTextNoReturn( const FunctionCallbackInfo<Value>& args ) {
	textWrapper *me = textWrapper::Unwrap<textWrapper>( args.This() );
	Isolate *isolate = args.GetIsolate();
	if( me->segment ) {
		if( me->segment->flags & TF_NORETURN )
			args.GetReturnValue().Set( True( isolate ) );
	}
	args.GetReturnValue().Set( False( isolate ) );
}

static void setTextNoReturn( const FunctionCallbackInfo<Value>& args ) {
	textWrapper *me = textWrapper::Unwrap<textWrapper>( args.This() );
	Isolate *isolate = args.GetIsolate();
	if( !me->segment )
		me->segment = SegCreate( 0 );
	if( args[0]->TOBOOL(isolate) )
		me->segment->flags |= TF_NORETURN;
	else
		me->segment->flags &= ~TF_NORETURN;
}


static void getTextParens( const FunctionCallbackInfo<Value>& args ) {
	textWrapper *me = textWrapper::Unwrap<textWrapper>( args.This() );
	Isolate *isolate = args.GetIsolate();
	if( me->segment ) {
		if( me->segment->flags & TF_PAREN )
			args.GetReturnValue().Set( True( isolate ) );
	}
	args.GetReturnValue().Set( False( isolate ) );
}

static void setTextParens( const FunctionCallbackInfo<Value>& args ) {
	textWrapper *me = textWrapper::Unwrap<textWrapper>( args.This() );
	Isolate *isolate = args.GetIsolate();
	if( !me->segment )
		me->segment = SegCreate( 0 );
	if( args[0]->TOBOOL(isolate) )
		me->segment->flags |= TF_PAREN;
	else
		me->segment->flags &= ~TF_PAREN;
}
static void getTextQuoted( const FunctionCallbackInfo<Value>& args ) {
	textWrapper *me = textWrapper::Unwrap<textWrapper>( args.This() );
	Isolate *isolate = args.GetIsolate();
	if( me->segment ) {
		if( me->segment->flags & TF_QUOTE )
			args.GetReturnValue().Set( True( isolate ) );
	}
	args.GetReturnValue().Set( False( isolate ) );
}

static void setTextQuoted( const FunctionCallbackInfo<Value>& args ) {
	textWrapper *me = textWrapper::Unwrap<textWrapper>( args.This() );
	Isolate *isolate = args.GetIsolate();
	if( !me->segment )
		me->segment = SegCreate( 0 );
	if( args[0]->TOBOOL(isolate) )
		me->segment->flags |= TF_QUOTE;
	else
		me->segment->flags &= ~TF_QUOTE;
}

static void getTextSQuoted( const FunctionCallbackInfo<Value>& args ) {
	textWrapper *me = textWrapper::Unwrap<textWrapper>( args.This() );
	Isolate *isolate = args.GetIsolate();
	if( me->segment ) {
		if( me->segment->flags & TF_SQUOTE )
			args.GetReturnValue().Set( True( isolate ) );
	}
	args.GetReturnValue().Set( False( isolate ) );
}

static void setTextSQuoted( const FunctionCallbackInfo<Value>& args ) {
	textWrapper *me = textWrapper::Unwrap<textWrapper>( args.This() );
	Isolate *isolate = args.GetIsolate();
	if( !me->segment )
		me->segment = SegCreate( 0 );
	if( args[0]->TOBOOL(isolate) )
		me->segment->flags |= TF_SQUOTE;
	else
		me->segment->flags &= ~TF_SQUOTE;
}

static void getTextFore( const FunctionCallbackInfo<Value>& args ) {
	textWrapper *me = textWrapper::Unwrap<textWrapper>( args.This() );
	Isolate *isolate = args.GetIsolate();
	if( me->segment ) {
		if( me->segment->format.flags.prior_foreground ) {
			args.GetReturnValue().Set( False( isolate ) );
		}else if( me->segment->format.flags.default_foreground ) {
			args.GetReturnValue().Set( True( isolate ) );
		}else
			args.GetReturnValue().Set( Number::New( isolate, me->segment->format.flags.foreground ) );
	}
	args.GetReturnValue().Set( False( isolate ) );
}

static void setTextFore( const FunctionCallbackInfo<Value>& args ) {
	textWrapper *me = textWrapper::Unwrap<textWrapper>( args.This() );
	Isolate *isolate = args.GetIsolate();
	if( !me->segment )
		me->segment = SegCreate( 0 );
	if( args[0]->IsBoolean() ) {
		if( args[0]->TOBOOL(isolate) ) {
			me->segment->format.flags.default_foreground = 1;
			me->segment->format.flags.prior_foreground = 0;
		}
		else {
			me->segment->format.flags.default_foreground = 0;
			me->segment->format.flags.prior_foreground = 1;
		}
	}
	else if( args[0]->IsNumber() ) {
		me->segment->format.flags.default_foreground = 0;
		me->segment->format.flags.prior_foreground = 0;
		me->segment->format.flags.foreground = (BIT_FIELD)args[0]->NumberValue( isolate->GetCurrentContext() ).FromMaybe( 0 );
	}
}

static void getTextBack( const FunctionCallbackInfo<Value>& args ) {
	textWrapper *me = textWrapper::Unwrap<textWrapper>( args.This() );
	Isolate *isolate = args.GetIsolate();
	if( me->segment ) {
		if( me->segment->format.flags.prior_background ) {
			args.GetReturnValue().Set( False( isolate ) );
		}
		else if( me->segment->format.flags.default_background ) {
			args.GetReturnValue().Set( True( isolate ) );
		}
		else
			args.GetReturnValue().Set( Number::New( isolate, me->segment->format.flags.background ) );
	}
	args.GetReturnValue().Set( False( isolate ) );
}

static void setTextBack( const FunctionCallbackInfo<Value>& args ) {
	textWrapper *me = textWrapper::Unwrap<textWrapper>( args.This() );
	Isolate *isolate = args.GetIsolate();
	if( !me->segment )
		me->segment = SegCreate( 0 );
	if( args[0]->IsBoolean() ) {
		if( args[0]->TOBOOL(isolate) ) {
			me->segment->format.flags.default_background = 1;
			me->segment->format.flags.prior_background = 0;
		}
		else {
			me->segment->format.flags.default_background = 0;
			me->segment->format.flags.prior_background = 1;
		}
	}
	else if( args[0]->IsNumber() ) {
		me->segment->format.flags.default_background = 0;
		me->segment->format.flags.prior_background = 0;
		me->segment->format.flags.background = (BIT_FIELD)args[0]->NumberValue( isolate->GetCurrentContext() ).FromMaybe( 0 );
	}
}

static void getTextBraced( const FunctionCallbackInfo<Value>& args ) {
	textWrapper *me = textWrapper::Unwrap<textWrapper>( args.This() );
	Isolate *isolate = args.GetIsolate();
	if( me->segment ) {
		if( me->segment->flags & TF_BRACE )
			args.GetReturnValue().Set( True( isolate ) );
	}
	args.GetReturnValue().Set( False( isolate ) );
}

static void setTextBraced( const FunctionCallbackInfo<Value>& args ) {
	textWrapper *me = textWrapper::Unwrap<textWrapper>( args.This() );
	Isolate *isolate = args.GetIsolate();
	if( !me->segment )
		me->segment = SegCreate( 0 );
	if( args[0]->TOBOOL(isolate) )
		me->segment->flags |= TF_BRACE;
	else
		me->segment->flags &= ~TF_BRACE;
}

static void getTextBracketed( const FunctionCallbackInfo<Value>& args ) {
	textWrapper *me = textWrapper::Unwrap<textWrapper>( args.This() );
	Isolate *isolate = args.GetIsolate();
	if( me->segment ) {
		if( me->segment->flags & TF_BRACKET )
			args.GetReturnValue().Set( True( isolate ) );
	}
	args.GetReturnValue().Set( False( isolate ) );
}

static void setTextBracketed( const FunctionCallbackInfo<Value>& args ) {
	textWrapper *me = textWrapper::Unwrap<textWrapper>( args.This() );
	Isolate *isolate = args.GetIsolate();
	if( !me->segment )
		me->segment = SegCreate( 0 );
	if( args[0]->TOBOOL(isolate) )
		me->segment->flags |= TF_BRACKET;
	else
		me->segment->flags &= ~TF_BRACKET;
}


static void getText( const FunctionCallbackInfo<Value>& args ) {
	textWrapper *me = textWrapper::Unwrap<textWrapper>( args.This() );
	Isolate *isolate = args.GetIsolate();
	if( me->segment ) {
		args.GetReturnValue().Set( String::NewFromUtf8( isolate, GetText( me->segment ), NewStringType::kNormal, (int)GetTextSize( me->segment ) ).ToLocalChecked() );
	}
	args.GetReturnValue().Set( Null( isolate ) );
}

static void setText( const FunctionCallbackInfo<Value>& args ) {
	textWrapper *me = textWrapper::Unwrap<textWrapper>( args.This() );
	Isolate *isolate = args.GetIsolate();
	String::Utf8Value text( USE_ISOLATE( isolate ) args[0]->ToString(isolate->GetCurrentContext()).ToLocalChecked() );

	if( !me->segment )
		me->segment = SegCreateFromCharLen( *text, text.length() );
	else {
		PTEXT saveText = me->segment;
		me->segment = SegCreateFromCharLen( *text, text.length() );
		me->segment->flags = saveText->flags;
		me->segment->format = saveText->format;
		me->segment->Next = saveText->Next;
		me->segment->Prior = saveText->Prior;
		LineRelease( SegGrab( saveText ) );
	}
}

PTEXT isTextObject( Isolate *isolate, Local<Value> object ) {
	if( object->IsObject() ) {
		class constructorSet *c = getConstructors( isolate );
		Local<FunctionTemplate> tpl = c->pTextTemplate.Get( isolate );
		Local<Object> locObj;
		if( tpl->HasInstance( locObj = object->ToObject(isolate->GetCurrentContext()).ToLocalChecked() ) ) {
			textWrapper *me = textWrapper::Unwrap<textWrapper>( locObj );
			if( me )
				return me->segment;
		}
	}
	return NULL;
}

void textObjectInit( Isolate *isolate, Local<Object> exports ) {
	Local<FunctionTemplate> textTemplate;
	class constructorSet *c = getConstructors( isolate );

	textTemplate = FunctionTemplate::New( isolate, newText );
	c->pTextTemplate.Reset( isolate, textTemplate );
	textTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.Text", v8::NewStringType::kNormal ).ToLocalChecked() );
	textTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); // 1 internal field for wrap

	textTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "foreground", v8::NewStringType::kNormal ).ToLocalChecked()
		, FunctionTemplate::New( isolate, getTextFore )
		, FunctionTemplate::New( isolate, setTextFore )
		, DontDelete );
	textTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "background", v8::NewStringType::kNormal ).ToLocalChecked()
		, FunctionTemplate::New( isolate, getTextBack )
		, FunctionTemplate::New( isolate, setTextBack )
		, DontDelete );

	textTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "noReturn", v8::NewStringType::kNormal ).ToLocalChecked()
		, FunctionTemplate::New( isolate, getTextNoReturn )
		, FunctionTemplate::New( isolate, setTextNoReturn )
		, DontDelete );

	textTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "x", v8::NewStringType::kNormal ).ToLocalChecked()
		, FunctionTemplate::New( isolate, getTextParens )
		, FunctionTemplate::New( isolate, setTextParens )
		, DontDelete );
	textTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "y", v8::NewStringType::kNormal ).ToLocalChecked()
		, FunctionTemplate::New( isolate, getTextParens )
		, FunctionTemplate::New( isolate, setTextParens )
		, DontDelete );

	textTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "parenthesis", v8::NewStringType::kNormal ).ToLocalChecked()
		, FunctionTemplate::New( isolate, getTextParens )
		, FunctionTemplate::New( isolate, setTextParens )
		, DontDelete );
	textTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "quoted", v8::NewStringType::kNormal ).ToLocalChecked()
		, FunctionTemplate::New( isolate, getTextQuoted )
		, FunctionTemplate::New( isolate, setTextQuoted )
		, DontDelete );
	textTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "singleQuoted", v8::NewStringType::kNormal ).ToLocalChecked()
		, FunctionTemplate::New( isolate, getTextSQuoted )
		, FunctionTemplate::New( isolate, setTextSQuoted )
		, DontDelete );
	textTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "braced", v8::NewStringType::kNormal ).ToLocalChecked()
		, FunctionTemplate::New( isolate, getTextBraced )
		, FunctionTemplate::New( isolate, setTextBraced )
		, DontDelete );
	textTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "bracketed", v8::NewStringType::kNormal ).ToLocalChecked()
		, FunctionTemplate::New( isolate, getTextBracketed )
		, FunctionTemplate::New( isolate, setTextBracketed )
		, DontDelete );
	textTemplate->PrototypeTemplate()->SetAccessorProperty( String::NewFromUtf8( isolate, "text", v8::NewStringType::kNormal ).ToLocalChecked()
		, FunctionTemplate::New( isolate, getText )
		, FunctionTemplate::New( isolate, setText )
		, DontDelete );

	c->textConstructor.Reset( isolate, textTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
	SET_READONLY( exports, "Text", textTemplate->GetFunction(isolate->GetCurrentContext()).ToLocalChecked() );
}