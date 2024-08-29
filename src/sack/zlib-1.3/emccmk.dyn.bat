



@SET ZLIB_SRCS=  adler32.c compress.c crc32.c  uncompr.c deflate.c trees.c  ^
           zutil.c inflate.c inftrees.c  inffast.c infback.c  
:#           infblock.c infcodes.c infutil.c


# under arm we don't really NEED direct FILEIO stuffs.
:SET( ZLIB_SRCS ${ZLIB_SRCS}   gzclose.c gzlib.c gzread.c gzwrite.c )

set CFLAGS=%COMMON_CFLAGS% 
set CFLAGS=%CFLAGS%  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference

@set SRCS=%ZLIB_SRCS%



del libz* 

call emcc -g -D_DEBUG -s WASM=1 -s SIDE_MODULE=1   %CFLAGS% %SRCS%
rename a.out.wasm libz.wasm.so
rename a.out.wasm.map libz.wasm.so.map
rename a.out.wast libz.wast

call emcc -O3 -s WASM=1 -s SIDE_MODULE=1   %CFLAGS% %SRCS%
rename a.out.wasm libz.o.wasm.so

copy libz* ..\..\..\amalgamate\wasmgui\libs


