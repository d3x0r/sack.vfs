cmake_minimum_required(VERSION 3.21)
# limited by ubuntu 18 to 3.10
#cmake_minimum_required(VERSION 3.10)

project( UnusedProjectName )

ADD_DEFINITIONS( -DNODE_GYP_MODULE_NAME=sack_vfs )

if( MAKE_GUI )
include( CMakeLists.gui.txt )
else()
include( CMakeLists.vfs.txt )
endif( MAKE_GUI )

ADD_DEFINITIONS( -DV8_VERSION=${V8_VERSION})
ADD_DEFINITIONS( -DV8_MAJOR=${V8_MAJOR})
ADD_DEFINITIONS( -DV8_MINOR=${V8_MINOR})
ADD_DEFINITIONS( -DV8_PATCH=${V8_PATCH})
ADD_DEFINITIONS( -DV8_BUILD=${V8_BUILD})
