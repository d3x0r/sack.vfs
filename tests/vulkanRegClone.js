

var sack= require( ".." );

var path = sack.registry.get( "HKLM/SOFTWARE/LunarG/VulkanSDK/VK_SDK_PATHs" );
var paths = path.split(';');
console.log( "SDK Installed at:", paths[0] );
sack.registry.set( "HKLM/SOFTWARE/WOW6432Node/LunarG/VulkanSDK/VK_SDK_PATHs", paths[0] );