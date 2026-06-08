// Standalone Vulkan transparency probe — no Dawn, no node, no sack.
//
// Goal: prove (or disprove) that on this Windows + driver + GPU combo,
// a WS_EX_NOREDIRECTIONBITMAP HWND + a Vulkan swap chain created with
// VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR will let DWM composite per-
// pixel alpha through to the desktop.
//
// What it does:
//   1. CreateWindowEx with WS_EX_NOREDIRECTIONBITMAP (no LAYERED).
//   2. Create Vulkan instance + surface (VK_KHR_win32_surface).
//   3. Pick first physical device + queue family that supports present.
//   4. Query VkSurfaceCapabilitiesKHR — log supportedCompositeAlpha.
//   5. Create swap chain with PRE_MULTIPLIED if supported, else fall
//      back to whichever supported mode is closest to alpha-honoring.
//   6. Each frame: clear the acquired image to (0.2, 0.0, 0.0, 0.4)
//      — semi-transparent dark red. Present.
//
// What you see tells you the answer:
//   * Desktop visible through the window with reddish tint  → works.
//     Conclusion: Vulkan + NOREDIRECTIONBITMAP IS a valid alpha path on
//     this system; Dawn must be doing something different.
//   * Solid opaque rectangle (red or otherwise)             → does NOT work.
//     Conclusion: the path is genuinely unsupported; transparency
//     requires DirectComposition (D3D) or LAYERED (GDI).
//
// Build (from this folder, in a VS Developer Command Prompt):
//   cl /std:c++17 /EHsc /I"..\..\..\..\build\Dawn_External-prefix\src\Dawn_External\third_party\vulkan-headers\src\include" test_alpha.cpp user32.lib gdi32.lib
//
// Run:
//   test_alpha.exe
//   Move it over a bright background (browser, etc.). ESC to quit.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

// -------- dynamic vulkan dispatch ----------------------------------------
// Avoid linking vulkan-1.lib so this builds with stock cl.exe on any box.

static HMODULE g_vkDll = nullptr;

#define VK_FUNCS_GLOBAL(X) \
	X(vkGetInstanceProcAddr) \
	X(vkCreateInstance) \
	X(vkEnumerateInstanceExtensionProperties)

#define VK_FUNCS_INSTANCE(X) \
	X(vkDestroyInstance) \
	X(vkEnumeratePhysicalDevices) \
	X(vkGetPhysicalDeviceProperties) \
	X(vkGetPhysicalDeviceQueueFamilyProperties) \
	X(vkCreateWin32SurfaceKHR) \
	X(vkDestroySurfaceKHR) \
	X(vkGetPhysicalDeviceSurfaceSupportKHR) \
	X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
	X(vkGetPhysicalDeviceSurfaceFormatsKHR) \
	X(vkGetPhysicalDeviceSurfacePresentModesKHR) \
	X(vkCreateDevice) \
	X(vkGetDeviceProcAddr)

#define VK_FUNCS_DEVICE(X) \
	X(vkDestroyDevice) \
	X(vkGetDeviceQueue) \
	X(vkCreateSwapchainKHR) \
	X(vkDestroySwapchainKHR) \
	X(vkGetSwapchainImagesKHR) \
	X(vkAcquireNextImageKHR) \
	X(vkQueuePresentKHR) \
	X(vkQueueSubmit) \
	X(vkQueueWaitIdle) \
	X(vkCreateCommandPool) \
	X(vkDestroyCommandPool) \
	X(vkAllocateCommandBuffers) \
	X(vkBeginCommandBuffer) \
	X(vkEndCommandBuffer) \
	X(vkCmdPipelineBarrier) \
	X(vkCmdClearColorImage) \
	X(vkCreateSemaphore) \
	X(vkDestroySemaphore) \
	X(vkCreateFence) \
	X(vkDestroyFence) \
	X(vkWaitForFences) \
	X(vkResetFences)

#define X(name) static PFN_##name name = nullptr;
VK_FUNCS_GLOBAL(X)
VK_FUNCS_INSTANCE(X)
VK_FUNCS_DEVICE(X)
#undef X

static bool loadVulkan() {
	g_vkDll = LoadLibraryA( "vulkan-1.dll" );
	if( !g_vkDll ) { printf( "vulkan-1.dll not found\n" ); return false; }
	vkGetInstanceProcAddr
		= (PFN_vkGetInstanceProcAddr)GetProcAddress( g_vkDll, "vkGetInstanceProcAddr" );
	if( !vkGetInstanceProcAddr ) return false;
	#define X(name) \
		name = (PFN_##name)vkGetInstanceProcAddr( nullptr, #name ); \
		if( !name ) { printf( "missing %s\n", #name ); return false; }
	VK_FUNCS_GLOBAL(X)
	#undef X
	return true;
}

static bool loadInstanceProcs( VkInstance inst ) {
	#define X(name) \
		name = (PFN_##name)vkGetInstanceProcAddr( inst, #name ); \
		if( !name ) { printf( "missing instance proc %s\n", #name ); return false; }
	VK_FUNCS_INSTANCE(X)
	#undef X
	return true;
}

static bool loadDeviceProcs( VkDevice dev ) {
	#define X(name) \
		name = (PFN_##name)vkGetDeviceProcAddr( dev, #name ); \
		if( !name ) { printf( "missing device proc %s\n", #name ); return false; }
	VK_FUNCS_DEVICE(X)
	#undef X
	return true;
}

// -------- window ---------------------------------------------------------

static bool g_quit = false;

static LRESULT CALLBACK wndProc( HWND h, UINT m, WPARAM w, LPARAM l ) {
	switch( m ) {
		case WM_CLOSE:    g_quit = true; return 0;
		case WM_KEYDOWN:  if( w == VK_ESCAPE ) g_quit = true; return 0;
		case WM_DESTROY:  PostQuitMessage( 0 ); return 0;
	}
	return DefWindowProcW( h, m, w, l );
}

static HWND createWindow( int w, int h ) {
	WNDCLASSEXW wc = {};
	wc.cbSize        = sizeof( wc );
	wc.lpfnWndProc   = wndProc;
	wc.hInstance     = GetModuleHandleW( nullptr );
	// IDC_ARROW is a TCHAR macro; explicit MAKEINTRESOURCEW so this
	// compiles whether UNICODE is defined or not.
	wc.hCursor       = LoadCursorW( nullptr, MAKEINTRESOURCEW( 32512 ) );
	wc.lpszClassName = L"VkAlphaTest";
	// Null brush — no GDI background paint. (Won't actually be called
	// because of NOREDIRECTIONBITMAP, but defensive anyway.)
	wc.hbrBackground = (HBRUSH)GetStockObject( NULL_BRUSH );
	RegisterClassExW( &wc );

	const DWORD exStyle = WS_EX_NOREDIRECTIONBITMAP;
	const DWORD style   = WS_OVERLAPPEDWINDOW | WS_VISIBLE;

	// Adjust for non-client area so client area is exactly w×h.
	RECT r = { 0, 0, w, h };
	AdjustWindowRectEx( &r, style, FALSE, exStyle );

	HWND hwnd = CreateWindowExW(
		exStyle, L"VkAlphaTest", L"Vulkan Alpha Test (ESC to quit)",
		style, 200, 200,
		r.right - r.left, r.bottom - r.top,
		nullptr, nullptr, wc.hInstance, nullptr );

	LONG_PTR actual = GetWindowLongPtrW( hwnd, GWL_EXSTYLE );
	printf( "HWND=%p exStyle=0x%lX  NOREDIRECTIONBITMAP=%d LAYERED=%d\n",
		hwnd, (long)actual,
		(int)( ( actual & WS_EX_NOREDIRECTIONBITMAP ) != 0 ),
		(int)( ( actual & WS_EX_LAYERED            ) != 0 ) );
	return hwnd;
}

// -------- helpers --------------------------------------------------------

#define VK_CHECK(expr) do { \
	VkResult _r = (expr); \
	if( _r != VK_SUCCESS ) { printf( "%s failed: %d\n", #expr, (int)_r ); exit( 1 ); } \
} while( 0 )

static const char* compositeAlphaName( VkCompositeAlphaFlagBitsKHR f ) {
	switch( f ) {
		case VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR:          return "OPAQUE";
		case VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR:  return "PRE_MULTIPLIED";
		case VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR: return "POST_MULTIPLIED";
		case VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR:         return "INHERIT";
		default:                                          return "?";
	}
}

// -------- main -----------------------------------------------------------

int main() {
	if( !loadVulkan() ) return 1;

	// Instance with the surface extensions.
	const char* instExts[] = { VK_KHR_SURFACE_EXTENSION_NAME
	                         , VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	VkApplicationInfo app = {};
	app.sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app.apiVersion       = VK_API_VERSION_1_0;
	app.pApplicationName = "test_alpha";
	VkInstanceCreateInfo ici = {};
	ici.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	ici.pApplicationInfo        = &app;
	ici.enabledExtensionCount   = 2;
	ici.ppEnabledExtensionNames = instExts;
	VkInstance inst = VK_NULL_HANDLE;
	VK_CHECK( vkCreateInstance( &ici, nullptr, &inst ) );
	if( !loadInstanceProcs( inst ) ) return 1;

	HWND hwnd = createWindow( 800, 600 );

	// Surface.
	VkWin32SurfaceCreateInfoKHR sci = {};
	sci.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	sci.hinstance = GetModuleHandleW( nullptr );
	sci.hwnd      = hwnd;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VK_CHECK( vkCreateWin32SurfaceKHR( inst, &sci, nullptr, &surface ) );

	// Pick first physical device with a present-capable queue family.
	uint32_t gpuCount = 0;
	vkEnumeratePhysicalDevices( inst, &gpuCount, nullptr );
	std::vector<VkPhysicalDevice> gpus( gpuCount );
	vkEnumeratePhysicalDevices( inst, &gpuCount, gpus.data() );

	VkPhysicalDevice gpu = VK_NULL_HANDLE;
	uint32_t          presentQueueIdx = 0;
	for( auto g : gpus ) {
		uint32_t qfCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( g, &qfCount, nullptr );
		std::vector<VkQueueFamilyProperties> qfs( qfCount );
		vkGetPhysicalDeviceQueueFamilyProperties( g, &qfCount, qfs.data() );
		for( uint32_t i = 0; i < qfCount; i++ ) {
			VkBool32 present = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR( g, i, surface, &present );
			if( present && ( qfs[ i ].queueFlags & VK_QUEUE_GRAPHICS_BIT ) ) {
				gpu = g;
				presentQueueIdx = i;
				break;
			}
		}
		if( gpu ) break;
	}
	if( !gpu ) { printf( "no suitable physical device\n" ); return 1; }

	VkPhysicalDeviceProperties pdp = {};
	vkGetPhysicalDeviceProperties( gpu, &pdp );
	printf( "GPU: %s  (api %u.%u.%u)\n",
		pdp.deviceName,
		VK_VERSION_MAJOR( pdp.apiVersion ),
		VK_VERSION_MINOR( pdp.apiVersion ),
		VK_VERSION_PATCH( pdp.apiVersion ) );

	// Surface capabilities — log composite alpha.
	VkSurfaceCapabilitiesKHR caps = {};
	VK_CHECK( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( gpu, surface, &caps ) );
	printf( "supportedCompositeAlpha = 0x%X (", (unsigned)caps.supportedCompositeAlpha );
	bool sep = false;
	for( VkCompositeAlphaFlagBitsKHR bit : { VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
	                                       , VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR
	                                       , VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR
	                                       , VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR } ) {
		if( caps.supportedCompositeAlpha & bit ) {
			printf( "%s%s", sep ? "|" : "", compositeAlphaName( bit ) );
			sep = true;
		}
	}
	printf( ")\n" );

	// Pick our preferred composite alpha (prefer PRE_MULTIPLIED).
	VkCompositeAlphaFlagBitsKHR chosenAlpha
		= ( caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR )
		    ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR
		: ( caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR )
		    ? VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
		: ( caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR )
		    ? VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR
		:   VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	printf( "chosenCompositeAlpha = %s\n", compositeAlphaName( chosenAlpha ) );

	// Pick BGRA8/UNORM (common). Fall back to whatever's first.
	uint32_t fmtCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR( gpu, surface, &fmtCount, nullptr );
	std::vector<VkSurfaceFormatKHR> formats( fmtCount );
	vkGetPhysicalDeviceSurfaceFormatsKHR( gpu, surface, &fmtCount, formats.data() );
	VkSurfaceFormatKHR fmt = formats[ 0 ];
	for( auto &f : formats ) {
		if( f.format == VK_FORMAT_B8G8R8A8_UNORM
		 && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) {
			fmt = f; break;
		}
	}
	printf( "format = %d  colorSpace = %d\n", (int)fmt.format, (int)fmt.colorSpace );

	// Logical device.
	float prio = 1.0f;
	VkDeviceQueueCreateInfo qci = {};
	qci.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	qci.queueFamilyIndex = presentQueueIdx;
	qci.queueCount       = 1;
	qci.pQueuePriorities = &prio;
	const char* devExts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	VkDeviceCreateInfo dci = {};
	dci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	dci.queueCreateInfoCount    = 1;
	dci.pQueueCreateInfos       = &qci;
	dci.enabledExtensionCount   = 1;
	dci.ppEnabledExtensionNames = devExts;
	VkDevice dev = VK_NULL_HANDLE;
	VK_CHECK( vkCreateDevice( gpu, &dci, nullptr, &dev ) );
	if( !loadDeviceProcs( dev ) ) return 1;
	VkQueue queue = VK_NULL_HANDLE;
	vkGetDeviceQueue( dev, presentQueueIdx, 0, &queue );

	// Swap chain.
	VkExtent2D extent = caps.currentExtent;
	if( extent.width == 0xFFFFFFFF ) { extent.width = 800; extent.height = 600; }
	uint32_t imageCount = caps.minImageCount + 1;
	if( caps.maxImageCount > 0 && imageCount > caps.maxImageCount )
		imageCount = caps.maxImageCount;

	VkSwapchainCreateInfoKHR scci = {};
	scci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	scci.surface          = surface;
	scci.minImageCount    = imageCount;
	scci.imageFormat      = fmt.format;
	scci.imageColorSpace  = fmt.colorSpace;
	scci.imageExtent      = extent;
	scci.imageArrayLayers = 1;
	scci.imageUsage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT
	                      | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	scci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	scci.preTransform     = caps.currentTransform;
	scci.compositeAlpha   = chosenAlpha;
	scci.presentMode      = VK_PRESENT_MODE_FIFO_KHR;  // always supported
	scci.clipped          = VK_TRUE;

	VkSwapchainKHR swap = VK_NULL_HANDLE;
	VK_CHECK( vkCreateSwapchainKHR( dev, &scci, nullptr, &swap ) );
	printf( "swap chain created\n" );

	std::vector<VkImage> images;
	{
		uint32_t n = 0;
		vkGetSwapchainImagesKHR( dev, swap, &n, nullptr );
		images.resize( n );
		vkGetSwapchainImagesKHR( dev, swap, &n, images.data() );
		printf( "%u swap chain images\n", n );
	}

	// Per-frame command machinery.
	VkCommandPool pool = VK_NULL_HANDLE;
	VkCommandPoolCreateInfo pci = {};
	pci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pci.queueFamilyIndex = presentQueueIdx;
	pci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VK_CHECK( vkCreateCommandPool( dev, &pci, nullptr, &pool ) );

	VkSemaphore acquireSem = VK_NULL_HANDLE, renderDoneSem = VK_NULL_HANDLE;
	VkSemaphoreCreateInfo ssi = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	vkCreateSemaphore( dev, &ssi, nullptr, &acquireSem );
	vkCreateSemaphore( dev, &ssi, nullptr, &renderDoneSem );
	VkFence frameFence = VK_NULL_HANDLE;
	VkFenceCreateInfo fci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	vkCreateFence( dev, &fci, nullptr, &frameFence );

	// The clear color we want to show. PRE_MULTIPLIED means RGB is
	// premultiplied by alpha — so (0.2, 0, 0, 0.4) really means
	// "dark red at 40% opacity over the desktop."
	VkClearColorValue clearColor = {};
	clearColor.float32[ 0 ] = 0.2f;
	clearColor.float32[ 1 ] = 0.0f;
	clearColor.float32[ 2 ] = 0.0f;
	clearColor.float32[ 3 ] = 0.4f;

	printf( "running... move the window over a bright background.\n"
	        "ESC to quit.\n" );

	while( !g_quit ) {
		MSG msg;
		while( PeekMessageW( &msg, nullptr, 0, 0, PM_REMOVE ) ) {
			TranslateMessage( &msg );
			DispatchMessageW( &msg );
		}
		if( g_quit ) break;

		vkWaitForFences( dev, 1, &frameFence, VK_TRUE, UINT64_MAX );
		vkResetFences  ( dev, 1, &frameFence );

		uint32_t imageIndex = 0;
		VkResult ar = vkAcquireNextImageKHR( dev, swap, UINT64_MAX,
			acquireSem, VK_NULL_HANDLE, &imageIndex );
		if( ar != VK_SUCCESS && ar != VK_SUBOPTIMAL_KHR ) {
			printf( "acquire failed: %d\n", (int)ar );
			break;
		}

		VkCommandBuffer cmd = VK_NULL_HANDLE;
		VkCommandBufferAllocateInfo cbai = {};
		cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cbai.commandPool        = pool;
		cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cbai.commandBufferCount = 1;
		vkAllocateCommandBuffers( dev, &cbai, &cmd );

		VkCommandBufferBeginInfo bbi = {};
		bbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		bbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer( cmd, &bbi );

		// UNDEFINED → TRANSFER_DST_OPTIMAL for clear.
		VkImageMemoryBarrier toClear = {};
		toClear.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		toClear.srcAccessMask       = 0;
		toClear.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
		toClear.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
		toClear.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		toClear.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toClear.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toClear.image               = images[ imageIndex ];
		toClear.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		vkCmdPipelineBarrier( cmd,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &toClear );

		VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		vkCmdClearColorImage( cmd, images[ imageIndex ],
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range );

		// TRANSFER_DST_OPTIMAL → PRESENT_SRC.
		VkImageMemoryBarrier toPresent = toClear;
		toPresent.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		toPresent.dstAccessMask = 0;
		toPresent.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		toPresent.newLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		vkCmdPipelineBarrier( cmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0, 0, nullptr, 0, nullptr, 1, &toPresent );

		vkEndCommandBuffer( cmd );

		VkPipelineStageFlags wait = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo si = {};
		si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		si.waitSemaphoreCount   = 1;
		si.pWaitSemaphores      = &acquireSem;
		si.pWaitDstStageMask    = &wait;
		si.commandBufferCount   = 1;
		si.pCommandBuffers      = &cmd;
		si.signalSemaphoreCount = 1;
		si.pSignalSemaphores    = &renderDoneSem;
		VK_CHECK( vkQueueSubmit( queue, 1, &si, frameFence ) );

		VkPresentInfoKHR pi = {};
		pi.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		pi.waitSemaphoreCount = 1;
		pi.pWaitSemaphores    = &renderDoneSem;
		pi.swapchainCount     = 1;
		pi.pSwapchains        = &swap;
		pi.pImageIndices      = &imageIndex;
		vkQueuePresentKHR( queue, &pi );

		// Quick + dirty — wait idle so the command buffer is reusable.
		// Real apps double-buffer command buffers; this is a probe.
		vkQueueWaitIdle( queue );
	}

	vkQueueWaitIdle( queue );
	vkDestroyFence       ( dev, frameFence,    nullptr );
	vkDestroySemaphore   ( dev, renderDoneSem, nullptr );
	vkDestroySemaphore   ( dev, acquireSem,    nullptr );
	vkDestroyCommandPool ( dev, pool,          nullptr );
	vkDestroySwapchainKHR( dev, swap,          nullptr );
	vkDestroyDevice      ( dev,                nullptr );
	vkDestroySurfaceKHR  ( inst, surface,      nullptr );
	vkDestroyInstance    ( inst,               nullptr );
	FreeLibrary( g_vkDll );
	return 0;
}
