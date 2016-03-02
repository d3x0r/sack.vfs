cmake_minimum_required(VERSION 3.0)

set( SACK_BASE M:/sack )

project (vfs_module)

include_directories(${CMAKE_JS_INC} ${SACK_BASE}/include ${SACK_BASE}/src/contrib/sha1lib )
ADD_DEFINITIONS( -DNO_SACK_INTERFACE -D__STATIC__ -D__NO_OPTIONS__ -D__NO_INTERFACE_SUPPORT__ )

include_directories( "../node_modules/nan" )

macro( COPY_CPLUSPLUS FILE_LIST )
FOREACH(SOURCE ${ARGN} )
   if( ${SOURCE} MATCHES "${SACK_BASE}/(.*)\\.c$" )
      set( FILEOK 1 )
      set( BASENAME ${CMAKE_MATCH_1} )
      set( FILEEXT .cpp )
   elseif( ${SOURCE} MATCHES "${SACK_BASE}/(.*)\\.h$" )
      set( FILEOK 1 )
      set( BASENAME ${CMAKE_MATCH_1} )
      set( FILEEXT .h )
   elseif( ${SOURCE} MATCHES "(.*)\\.c$" )
      set( FILEOK 1 )
      set( BASENAME ${CMAKE_MATCH_1} )
      set( FILEEXT .cpp )
   elseif( ${SOURCE} MATCHES "(.*)\\.h$" )
      set( FILEOK 1 )
      set( BASENAME ${CMAKE_MATCH_1} )
      set( FILEEXT .h )
   else()
      set( FILEOK 0 )
      set( BASENAME "" )
   endif()
   
   if( FILEOK )
       get_source_file_property(SOURCE_FOLDER ${SOURCE} FOLDER)
       
       if( ${SOURCE} MATCHES "^${PROJECT_SOURCE_DIR}.*" )
          if( NOT ${SOURCE_FOLDER} MATCHES "NOTFOUND" )
            #message( "err folder : ${SOURCE_FOLDER} " )
            SOURCE_GROUP( ${SOURCE_FOLDER} FILES ${SOURCE} )
            SOURCE_GROUP( ${SOURCE_FOLDER} FILES ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} )
          endif()
          message( "copy ${source} to ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT}" )
          add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT}
                              DEPENDS ${SOURCE}
                              COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SOURCE} ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} 
                              COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} 
                              )
       
       else( ${SOURCE} MATCHES "^${PROJECT_SOURCE_DIR}.*" )
         if( NOT ${SOURCE_FOLDER} MATCHES "NOTFOUND" )
           #message( "err folder : ${SOURCE_FOLDER} " )
           SOURCE_GROUP( ${SOURCE_FOLDER} FILES ${PROJECT_SOURCE_DIR}/${SOURCE} )
           SOURCE_GROUP( ${SOURCE_FOLDER} FILES ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} )
         endif()
         add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT}
                             DEPENDS ${SOURCE}
                             COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SOURCE} ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} 
                             COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} 
                             )
       endif()
     set( ${FILE_LIST} ${${FILE_LIST}} ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} )
   else()
      message( "bad source reference ${source}" )
   endif()
ENDFOREACH(SOURCE)
endmacro( COPY_CPLUSPLUS )

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    ADD_DEFINITIONS( -D__64__ )
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )


COPY_CPLUSPLUS( EXTRA_SOURCES 
	${SACK_BASE}/src/utils/virtual_file_system/vfs.c 
	${SACK_BASE}/src/utils/virtual_file_system/vfs_internal.h
        ${SACK_BASE}/src/salty_random_generator/salty_generator.c
        ${SACK_BASE}/src/contrib/sha1lib/sha1.c
        ${SACK_BASE}/src/contrib/sha2lib/sha2.c
	${SACK_BASE}/src/deadstart/deadstart_core.c
	${SACK_BASE}/src/sysloglib/syslog.c
	${SACK_BASE}/src/systemlib/system.c
	${SACK_BASE}/src/systemlib/oswin.c
	${SACK_BASE}/src/systemlib/taskinfo.h
	${SACK_BASE}/src/timerlib/timers.c
	${SACK_BASE}/src/idlelib/idle.c
	${SACK_BASE}/src/procreglib/names.c
	${SACK_BASE}/src/procreglib/registry.h
	${SACK_BASE}/src/memlib/sharemem.c
	${SACK_BASE}/src/memlib/memory_operations.c
	${SACK_BASE}/src/memlib/sharestruc.h
	${SACK_BASE}/src/filesyslib/winfiles.c
	${SACK_BASE}/src/filesyslib/filescan.c
	${SACK_BASE}/src/filesyslib/pathops.c
	${SACK_BASE}/src/filesyslib/filesys_local.h
	${SACK_BASE}/src/typelib/typecode.c
	${SACK_BASE}/src/typelib/text.c
	${SACK_BASE}/src/typelib/binarylist.c
	${SACK_BASE}/src/typelib/sets.c
	${SACK_BASE}/src/deadstart/deadstart_lib.c
)

add_library(${PROJECT_NAME} SHARED 
	vfs_module.cc
        ${EXTRA_SOURCES}
        )

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS "TARGETNAME=\"${PROJECT_NAME}.node\"" )
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})
if( WIN32 )
   target_link_libraries(${PROJECT_NAME} winmm ws2_32 )
endif( WIN32 )
