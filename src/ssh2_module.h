#include <libssh2.h>



struct client_ssh {
	PCLIENT pc;
	LIBSSH2_SESSION* session;
	PDATALIST buffers;
	PTHREAD waiter;
};



class SSH2_Object {

public:
    struct client_ssh *client;

	SSH2_Object();
	~SSH2_Object();

	static void Init( Isolate *isolate, Local<Object> exports );
	static void New( const v8::FunctionCallbackInfo<Value>& args  );
    static void Connect( const v8::FunctionCallbackInfo<Value>& args  );
};

