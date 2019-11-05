
#include "../global.h"


Persistent<Function> VulkanObject::constructor;

static void initOptions( void ) {
	char tmp[256];
	int n = 0;
	SACK_GetProfileInt( NULL, "SACK/Video Render/Log Render Timing", 0 );
	SACK_GetProfileInt( NULL, "SACK/Video Render/360 view", 0 );

	SACK_GetProfileInt( NULL, "SACK/Image Library/Scale", 10 ); // if 0, use inverse scale
	SACK_GetProfileInt( NULL, "SACK/Image Library/Inverse Scale", 2 );  // if 0, default to 1.

	SACK_GetProfileInt( NULL, "SACK/Video Render/Number of Displays", 0/*l.flags.bView360*/ ? 6 : 1 );
	SACK_GetProfileInt( NULL, "SACK/Video Render/Force Aspect 1.0", ( 1/*nDisplays*/ == 1 ) ? 0 : 1 );

	//tnprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Display is topmost", n + 1 );
	tnprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Display is topmost", 1 );
	SACK_GetProfileInt( NULL, tmp, 0 );

	tnprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Use Custom Position", n + 1 );
	SACK_GetProfileInt( NULL, tmp, 0/*l.flags.bView360*/ ? 1 : 0 );

	tnprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Use Display", n + 1 );
	SACK_GetProfileInt( NULL, tmp, 1/*nDisplays*/>1 ? n + 1 : 0 );

	tnprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Camera Type", n + 1 );
	SACK_GetProfileInt( NULL, tmp, ( 1/*nDisplays*/ == 6 ) ? n : 2 );

	/*
				tnprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Position/x", n + 1 );
				camera->x = SACK_GetProfileIntEx( GetProgramName(), tmp, (
					nDisplays == 4
					? n == 0 ? 0 : n == 1 ? 400 : n == 2 ? 0 : n == 3 ? 400 : 0
					: nDisplays == 6
					? n == 0 ? ( ( screen_w * 0 ) / 4 ) : n == 1 ? ( ( screen_w * 1 ) / 4 ) : n == 2 ? ( ( screen_w * 1 ) / 4 ) : n == 3 ? ( ( screen_w * 1 ) / 4 ) : n == 4 ? ( ( screen_w * 2 ) / 4 ) : n == 5 ? ( ( screen_w * 3 ) / 4 ) : 0
					: nDisplays == 1
					? 0
					: 0 ), TRUE );
				tnprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Position/y", n + 1 );
				camera->y = SACK_GetProfileIntEx( GetProgramName(), tmp, (
					nDisplays == 4
					? n == 0 ? 0 : n == 1 ? 0 : n == 2 ? 300 : n == 3 ? 300 : 0
					: nDisplays == 6
					? n == 0 ? ( ( screen_h * 1 ) / 3 ) : n == 1 ? ( ( screen_h * 0 ) / 3 ) : n == 2 ? ( ( screen_h * 1 ) / 3 ) : n == 3 ? ( ( screen_h * 2 ) / 3 ) : n == 4 ? ( ( screen_h * 1 ) / 3 ) : n == 5 ? ( ( screen_h * 1 ) / 3 ) : 0
					: nDisplays == 1
					? 0
					: 0 ), TRUE );
				tnprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Position/width", n + 1 );
				camera->w = SACK_GetProfileIntEx( GetProgramName(), tmp, (
					nDisplays == 4
					? 400
					: nDisplays == 6
					? ( screen_w / 4 )
					: nDisplays == 1
					? 800
					: 0 ), TRUE );
				tnprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Position/height", n + 1 );
				camera->h = SACK_GetProfileIntEx( GetProgramName(), tmp, (
					nDisplays == 4
					? 300
					: nDisplays == 6
					? ( screen_h / 3 )
					: nDisplays == 1
					? 600
					: 0 ), TRUE );
		*/
				/*
				tnprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Position/Direction", n+1 );
				camera->direction = SACK_GetProfileIntEx( GetProgramName(), tmp, (
				nDisplays==4
				?n==0?0:n==1?1:n==2?2:n==3?3:0
				:nDisplays==6
				?n // this is natural 0=left, 2=forward, 1=up, 3=down, 4=right, 5=back
				:nDisplays==1
				?0
				:0), TRUE );
				*/

	/*
		l.flags.bLogMessageDispatch = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/log message dispatch", 0, TRUE );
		l.flags.bLogFocus = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/log focus event", 0, TRUE );
		l.flags.bLogKeyEvent = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/log key event", 0, TRUE );
		l.flags.bLogMouseEvent = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/log mouse event", 0, TRUE );
		l.flags.bLayeredWindowDefault = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/Default windows are layered", LAYERED_DEFAULT, TRUE ) ? TRUE : FALSE;
		l.flags.bLogWrites = SACK_GetOptionIntEx( option, GetProgramName(), "SACK/Video Render/Log Video Output", 0, TRUE );
     */
}



void VulkanObject::Init( Isolate* isolate, Local<Object> exports ) {
	Local<Context> context = isolate->GetCurrentContext();

	Local<FunctionTemplate> vulkanTemplate;

	// Prepare constructor template
	vulkanTemplate = FunctionTemplate::New( isolate, New );
	vulkanTemplate->SetClassName( localStringExternal( isolate, "sack.Vulkan" ) );
	vulkanTemplate->InstanceTemplate()->SetInternalFieldCount( 1 ); /* one internal for wrap */
	constructor.Reset( isolate, vulkanTemplate->GetFunction(context).ToLocalChecked() );

	// Prototype
	NODE_SET_PROTOTYPE_METHOD( vulkanTemplate, "frameBuffer", getFrameBuffer );

	SET_READONLY( exports, "Vulkan", vulkanTemplate->GetFunction(context).ToLocalChecked() );

	//SET_READONLY( vulkanTemplate->GetFunction(), "getDisplay", Function::New( isolate, RenderObject::getDisplay ) );
}

VulkanObject::VulkanObject(  ) {
}

VulkanObject::~VulkanObject() {
}

void VulkanObject::New( const FunctionCallbackInfo<Value>& args ) {
}

void VulkanObject::getFrameBuffer( const FunctionCallbackInfo<Value>& args ) {
}
