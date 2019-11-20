cmake_minimum_required(VERSION 3.6)

project( UnusedProjectName )

ADD_DEFINITIONS( -DNODE_GYP_MODULE_NAME=sack_vfs )

if( MAKE_GUI )
include( CMakeLists.gui.txt )
else()
include( CMakeLists.vfs.txt )
endif( MAKE_GUI )


