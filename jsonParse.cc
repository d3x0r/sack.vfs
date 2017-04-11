

#include <node.h>
#include <node_object_wrap.h>
#include <v8.h>
//#include <nan.h>

#include "src/sack.h"
#undef New

Local<Value> makeValue( Isolate *isolate, struct json_value_container *val ) {

	switch( val->result_value ) {
	case VALUE_UNDEFINED:
		return Undefined( isolate );
	case uninitialized:
	default:
		lprintf( "Parser faulted; should never have a uninitilized value." );
		return Undefined( isolate );
	case VALUE_NULL:
		return Null( isolate );
	case VALUE_TRUE:
		return True( isolate );
	case VALUE_FALSE:
		return False( isolate );
	case VALUE_STRING:
		return String::NewFromUtf8( isolate, GetText( val->string ) );
	case VALUE_NUMBER:
		if( val->float_result )
			return Number::New( isolate, val->result_d );
		else
			return Number::New( isolate, val->result_n );
	case VALUE_ARRAY:
		return Array::New( isolate );
	case VALUE_OBJECT:
		return Object::New( isolate );
	}

}



void buildObject( PDATALIST msg_data, Local<Object> o, Isolate *isolate ) {

	struct json_value_container *val;
	Local<Object> sub_o;
	INDEX idx;
	DATA_FORALL( (*msg_data), idx, struct json_value_container*, val )
	{
		switch( val->result_value ) {
	case VALUE_UNDEFINED:
		// skip undefined values (even if literally 'undefined' input
		break;
	case uninitialized:
		lprintf( "Parser faulted; should never have a uninitilized value." );
		break;
	case VALUE_NULL:
			o->Set( String::NewFromUtf8( isolate, GetText( val->name ) )
						, Null( isolate )
			 );
			break;
	case VALUE_TRUE:
			o->Set( String::NewFromUtf8( isolate, GetText( val->name ) )
						, True( isolate )
			 );
			break;
	case VALUE_FALSE:
			o->Set( String::NewFromUtf8( isolate, GetText( val->name ) )
						, False( isolate )
			 );
			break;
	case VALUE_STRING:
			o->Set( String::NewFromUtf8( isolate, GetText( val->name ) )
						, String::NewFromUtf8( isolate, GetText( val->string ) )
			 );
			break;
	case VALUE_NUMBER:
			if( val->float_result )
				o->Set( String::NewFromUtf8( isolate, GetText( val->name ) )
						, Number::New( isolate, val->result_d )
					);
			else
				o->Set( String::NewFromUtf8( isolate, GetText( val->name ) )
						, Number::New( isolate, val->result_n )
					 );
			break;
	case VALUE_OBJECT:
			o->Set( String::NewFromUtf8( isolate, GetText( val->name ) )
						, sub_o = Object::New( isolate )
			 );
			buildObject( val->contains, new_o, isolate );
			break;
		}
	}
		

}


Local<Value> ParseJSON(  Isolate *isolate, char *utf8String, size_t len) {
	PDATALIST parsed = NULL;
	Local<Object> o;// = Object::New( isolate );
	Local<Value> v;// = Object::New( isolate );
	json_parse_message( msg, len, &parsed );
	if( parsed->Cnt > 1 ) {
		lprintf( "Multiple values would result, invalid parse." );
		return null;
		// outside should always be a single value
	}

	struct json_value_container *val;
	val = GetDataLink( &parsed, 0 );
	if( val->contains ) {
		val = GetDataLink( &val->contains, 0 );
		if( val->name )
			o = Object::New(isolate);
		else
			o = Array::New(isolate);
	}
	else {
		v = 
		// this is illegal json
		// cannot result from a simple value?	
	}
	if( val->name )
		Local<Object> o = Object::New( isolate );
	
	buildObject( parsed, o, isolate );

	json_dispose_message( &parsed );
	if( o ) 
		return o;
}



void json_dispose_message( PDATALIST *msg_data )
{
	struct json_value_container *val;
	INDEX idx;
	DATA_FORALL( (*msg_data), idx, struct json_value_container*, val )
	{
		if( val->name ) LineRelease( val->name );
		if( val->string ) LineRelease( val->string );
		if( val->value_type == VALUE_OBJECT )
			json_dispose_message( &val->contains );
	}
	// quick method
	DeleteDataList( msg_data );

}

static char * JSON_ObjectElementTypeStrings =
{
   "JSON_Element_Integer_8",
   "JSON_Element_Integer_16",
   "JSON_Element_Integer_32",
   "JSON_Element_Integer_64",
   "JSON_Element_Unsigned_Integer_8",
   "JSON_Element_Unsigned_Integer_16",
   "JSON_Element_Unsigned_Integer_32",
   "JSON_Element_Unsigned_Integer_64",
   "JSON_Element_String",
   "JSON_Element_CharArray",
   "JSON_Element_Float",
   "JSON_Element_Double",
   "JSON_Element_Array",  // result will fill a PLIST
   "JSON_Element_Object",
   "JSON_Element_ObjectPointer",
   "JSON_Element_List",
   "JSON_Element_Text",  // ptext type
   "JSON_Element_PTRSZVAL",  
   "JSON_Element_PTRSZVAL_BLANK_0",
	"JSON_Element_UserRoutine",
	"JSON_Element_Raw_Object", // unparsed object remainder.  Includes bounding { } object indicator for re-parsing
   //JSON_Element_StaticText,  // text type; doesn't happen very often.
};


static char *json_value_type_strings_[] = {
	"VALUE_UNDEFINED"
	, "uninitialized"
	, "VALUE_NULL"
	, "VALUE_TRUE"
	, "VALUE_FALSE"
	, "VALUE_STRING"
	, "VALUE_NUMBER"
	, "VALUE_OBJECT"
	, "VALUE_ARRAY"
};

static char *json_value_type_strings = json_value_type_strings_+1;

// puts the current collected value into the element; assumes conversion was correct
static void FillDataToElement( struct json_context_object_element *element
							    , size_t object_offset
								, struct json_value_container *val
								, POINTER msg_output )
{
	if( !val->name )
		return;
	// remove name; indicate that the value has been used.
	//LineRelease( val->name );
	//val->name = NULL;
	switch( element->type )
	{
	case JSON_Element_String:
		if( element->count )
		{
		}
		else if( element->count_offset != JSON_NO_OFFSET )
		{
		}
		else
		{
			switch( val->result_value )
			{
			case VALUE_NULL:
				((CTEXTSTR*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = NULL;
				break;
			case VALUE_STRING:
				((CTEXTSTR*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = StrDup( GetText( val->string ) );
				break;
			default:
				lprintf( WIDE("Expected a string, but parsed result was a %s"), val->result_value );
				break;
			}
		}
      break;
	case JSON_Element_Integer_64:
	case JSON_Element_Integer_32:
	case JSON_Element_Integer_16:
	case JSON_Element_Integer_8:
	case JSON_Element_Unsigned_Integer_64:
	case JSON_Element_Unsigned_Integer_32:
	case JSON_Element_Unsigned_Integer_16:
	case JSON_Element_Unsigned_Integer_8:
		if( element->count )
		{
		}
		else if( element->count_offset != JSON_NO_OFFSET )
		{
		}
		else
		{
			switch( val->result_value )
			{
			case VALUE_TRUE:
				switch( element->type )
				{
				case JSON_Element_String:
				case JSON_Element_CharArray:
				case JSON_Element_Float:
				case JSON_Element_Double:
				case JSON_Element_Array:
				case JSON_Element_Object:
				case JSON_Element_ObjectPointer:
				case JSON_Element_List:
				case JSON_Element_Text:
				case JSON_Element_PTRSZVAL:
				case JSON_Element_PTRSZVAL_BLANK_0:
				case JSON_Element_UserRoutine:
				case JSON_Element_Raw_Object:
					lprintf( "Uhandled element conversion." );
					break;

				case JSON_Element_Integer_64:
				case JSON_Element_Unsigned_Integer_64:
					((int8_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 1;
               break;
				case JSON_Element_Integer_32:
				case JSON_Element_Unsigned_Integer_32:
					((int16_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 1;
               break;
				case JSON_Element_Integer_16:
				case JSON_Element_Unsigned_Integer_16:
					((int32_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 1;
               break;
				case JSON_Element_Integer_8:
				case JSON_Element_Unsigned_Integer_8:
					((int64_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 1;
					break;
				}
				break;
			case VALUE_FALSE:
				switch( element->type )
				{
				case JSON_Element_String:
				case JSON_Element_CharArray:
				case JSON_Element_Float:
				case JSON_Element_Double:
				case JSON_Element_Array:
				case JSON_Element_Object:
				case JSON_Element_ObjectPointer:
				case JSON_Element_List:
				case JSON_Element_Text:
				case JSON_Element_PTRSZVAL:
				case JSON_Element_PTRSZVAL_BLANK_0:
				case JSON_Element_UserRoutine:
				case JSON_Element_Raw_Object:
					lprintf( "Uhandled element conversion." );
					break;

				case JSON_Element_Integer_64:
				case JSON_Element_Unsigned_Integer_64:
					((int8_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 0;
               break;
				case JSON_Element_Integer_32:
				case JSON_Element_Unsigned_Integer_32:
					((int16_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 0;
               break;
				case JSON_Element_Integer_16:
				case JSON_Element_Unsigned_Integer_16:
					((int32_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 0;
               break;
				case JSON_Element_Integer_8:
				case JSON_Element_Unsigned_Integer_8:
					((int64_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 0;
					break;
				}
				break;
			case VALUE_NUMBER:
				if( val->float_result )
				{
					lprintf( WIDE("warning received float, converting to int") );
					((int64_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = (int64_t)val->result_d;
				}
				else
				{
					((int64_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = val->result_n;
				}
				break;
			default:
				lprintf( WIDE("Expected a string, but parsed result was a %d"), val->result_value );
				break;
			}
		}
		break;

	case JSON_Element_Float:
	case JSON_Element_Double:
		if( element->count )
		{
		}
		else if( element->count_offset != JSON_NO_OFFSET )
		{
		}
		else
		{
			switch( val->result_value )
			{
			case VALUE_NUMBER:
				if( val->float_result )
				{
					if( element->type == JSON_Element_Float )
						((float*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = (float)val->result_d;
					else
						((double*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = val->result_d;

				}
				else
				{
               // this is probably common (0 for instance)
					lprintf( WIDE("warning received int, converting to float") );
					if( element->type == JSON_Element_Float )
						((float*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = (float)val->result_n;
					else
						((double*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = (double)val->result_n;

				}
				break;
			default:
				lprintf( WIDE("Expected a float, but parsed result was a %d"), val->result_value );
				break;
			}

		}

		break;
	case JSON_Element_PTRSZVAL_BLANK_0:
	case JSON_Element_PTRSZVAL:
		if( element->count )
		{
		}
		else if( element->count_offset != JSON_NO_OFFSET )
		{
		}
		else
		{
			switch( val->result_value )
			{
			case VALUE_NUMBER:
				if( val->float_result )
				{
					lprintf( WIDE("warning received float, converting to int (uintptr_t)") );
					((uintptr_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = (uintptr_t)val->result_d;
				}
				else
				{
					// this is probably common (0 for instance)
					((uintptr_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = (uintptr_t)val->result_n;
				}
				break;
			}
		}
		break;
	}
}


LOGICAL json_decode_message( PDATALIST msg_data )
{

	PLIST elements = format->elements;
	struct json_context_object_element *element;

	// message is allcoated +7 bytes in case last part is a uint8_t type
	// all integer stores use uint64_t type to store the collected value.
	if( !msg_output )
		msg_output = (*_msg_output)
			= NewArray( uint8_t, format->object_size + 7  );

	LOGICAL first_token = TRUE;

				if( first_token )
				{
					// begin an object.
					// content is
					elements = format->members;
					PushLink( &element_lists, elements );
					first_token = 0;
					//n++;
				}
				else
				{
					INDEX idx;
					// begin a sub object, we should have just had a name for it
					// since this will be the value of that name.
					// this will eet 'element' to NULL (fall out of loop) or the current one to store values into
					DATA_FORALL( elements, idx, struct json_context_object_element *, element )
					{
						if( StrCaseCmp( element->name, GetText( val.name ) ) == 0 )
						{
							if( ( element->type == JSON_Element_Object )
								|| ( element->type == JSON_Element_ObjectPointer ) )
							{
								if( element->object )
								{
									// save prior element list; when we return to this one?
									struct json_parse_context *old_context = New( struct json_parse_context );
									old_context->context = parse_context;
									old_context->elements = elements;
									old_context->object = format;
									PushLink( &context_stack, old_context );
									format = element->object;
									elements = element->object->members;
									//n++;
									break;
								}
								else
								{
									lprintf( WIDE("Error; object type does not have an object") );
									status = FALSE;
								}
							}
							else
							{
								lprintf( WIDE("Incompatible value expected object type, type is %d"), element->type );
							}
						}
					}
				}
#endif
	return FALSE;
}

