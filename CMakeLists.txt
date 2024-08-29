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


