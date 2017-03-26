:gcc -g  -c sqlite3.c -o sqlite3.o
g++ -g "-DTARGETNAME=""a.exe""" -IM:/sack/src/contrib/libressl/2.4.2/include -DFORCE_COLOR_MACROS -D__STATIC__ -DNEED_SHLAPI -DNEED_SHLOBJ -DUSE_SQLITE_INTERFACE -c sack.cc -o sack.o 
g++ -g -DFORCE_COLOR_MACROS -D__STATIC__ -DNEED_SHLAPI -DNEED_SHLOBJ -DUSE_SQLITE_INTERFACE wsTest.cc sack.o sqlite3.o -lwinmm -lole32 -lws2_32 -lrpcrt4 -liphlpapi -lodbc32 -lpsapi -L. -ltls.dll -lssl.dll -lcrypto.dll
:g++ -g "-DTARGETNAME=""a.exe""" -IM:/sack/src/contrib/libressl/2.4.2/include -DFORCE_COLOR_MACROS -D__STATIC__ -DNEED_SHLAPI -DNEED_SHLOBJ -DUSE_SQLITE_INTERFACE wsTest.cc sack.cc sqlite3.o -lwinmm -lole32 -lws2_32 -lrpcrt4 -liphlpapi -lodbc32 -lpsapi -L. -ltls.dll -lssl.dll -lcrypto.dll
:-D__NO_OPTIONS__ 