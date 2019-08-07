cmake_minimum_required(VERSION 3.6)

project( UnusedProjectName )

if( MAKE_GUI )
include( CMakeLists.gui.txt )
else()
include( CMakeLists.vfs.txt )
endif( MAKE_GUI )


