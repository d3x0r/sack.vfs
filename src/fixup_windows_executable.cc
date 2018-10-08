#define _WIN32_WINNT 0x601
#include "global.h"
//#include <sack.h>
#include <psapi.h>
//#include <winapi.h>
#include <TlHelp32.h>

#include "ntundoc.h"


struct fixup_table_entry {
	CTEXTSTR libname;
	CTEXTSTR import_libname;
	CTEXTSTR import_name;
	POINTER pointer;
	LOGICAL fixed;
};

struct fileHandleMap {
	FILE *file;
	HANDLE handle;
};

static struct fixup_local_data {
	PLIST handleMap;
	int usedHandles;
	HANDLE pipe[2];
	TEXTSTR leadinPath;
	size_t leadinLen;
	LOGICAL _debug;
	struct file_system_mounted_interface *rom;
	struct file_system_interface *fsi;
} fld;

void InitFS( const FunctionCallbackInfo<Value>& args ){
	// next open will be for internal hook code?
	// arg passed should be source VFS name to use...
	if( args.Length() ) {
		struct volume *vol;
		String::Utf8Value fName( args[0]->ToString() );
		char *mount_name = StrDup( *fName );
		size_t sz = 0;         
		char *key1, *key2;
		if( args.Length() > 1 ) {
			String::Utf8Value fName2( args[1]->ToString() );
			key1 = StrDup( *fName2 );
		} else key1 = NULL;
		if( args.Length() > 2 ) {
			String::Utf8Value fName3( args[2]->ToString() );
			key2 = StrDup( *fName3 );
		} else key2 = NULL;

	
		POINTER memory = OpenSpace( NULL, mount_name, &sz );
		vol = sack_vfs_use_crypt_volume( memory, sz, key1, key2 );
		fld.rom = sack_mount_filesystem( "self", fld.fsi, 100, (uintptr_t)vol, FALSE );
		Deallocate( char*, key1 );
		Deallocate( char*, key2 );
		Deallocate( char*, mount_name );
	}
}


static FILE* GetFileFromHandle( HANDLE h ) {
	INDEX idx;
	struct fileHandleMap *ent;
	LIST_FORALL( fld.handleMap, idx, struct fileHandleMap*, ent ) {
		if( ent->handle == h )
			return ent->file;
	}
	return NULL;
}

static HANDLE MapFileHandle( HANDLE h, FILE *f ) {
	if( f ) {
		struct fileHandleMap *ent = NewArray( struct fileHandleMap, 1 );
		ent->file = f;
		ent->handle = h;
		AddLink( &fld.handleMap, ent );
		return h;
	}
	else {
		INDEX idx;
		struct fileHandleMap *ent;
		LIST_FORALL( fld.handleMap, idx, struct fileHandleMap*, ent ) {
			if( ent->handle == h ) {
				DeleteLink( &fld.handleMap, ent );
				break;
			}
		}
		if( ent ) {
			f = ent->file;
			CloseHandle( h );
			Deallocate( struct fileHandleMap*, ent );
			return (HANDLE)f;
		}
		return NULL;
	}
	return INVALID_HANDLE_VALUE;
}


static HANDLE getHandle( void ) {
	if( !fld.usedHandles ) {
		fld.usedHandles = 1;
		if( !CreatePipe( fld.pipe, fld.pipe+1, NULL, 0 ) ) {
			// probably something like ENOHANDLES?
			return INVALID_HANDLE_VALUE;
		}
		return fld.pipe[0];
	}else {
		fld.usedHandles = 0;
		return fld.pipe[1];
	}
}

static HANDLE WINAPI CreateFileA_stub(   LPCSTR               lpFileName,
      DWORD                 dwDesiredAccess,
      DWORD                 dwShareMode,
      LPSECURITY_ATTRIBUTES lpSecurityAttributes,
      DWORD                 dwCreationDisposition,
      DWORD                 dwFlagsAndAttributes,
      HANDLE                hTemplateFile ) 
{
	TEXTSTR f = DupCharToText( lpFileName );
	DWORD dwError = ERROR_SUCCESS;
	LOGICAL trunc = FALSE;
	FILE * file;
	lprintf( "create fileA called %s cre %08x attr %08x shr %08x acc %08x %p", lpFileName, dwCreationDisposition, dwFlagsAndAttributes, dwShareMode, dwDesiredAccess, hTemplateFile );
	if( !(dwCreationDisposition & CREATE_ALWAYS) ) {
		if( dwCreationDisposition & CREATE_NEW ) {
			if( sack_exists( f ) ) {
			    SetLastError( ERROR_ALREADY_EXISTS );
			    return INVALID_HANDLE_VALUE;
			}
		}
		if( dwCreationDisposition & OPEN_ALWAYS ) {
			if( sack_exists( f ) ) {
			    dwError = ERROR_ALREADY_EXISTS;
			}
		}
		else if( dwCreationDisposition & OPEN_EXISTING) {
			if( !sack_exists( f ) ) {
				SetLastError( ERROR_FILE_NOT_FOUND );
				return INVALID_HANDLE_VALUE;
			}
		}
	}
	if( dwCreationDisposition & TRUNCATE_EXISTING ) {
		if( sack_exists( f ) ) 
			trunc = TRUE;
	}
	file = sack_fopen( 0, f, "w" );
	if( file )  {
		if( trunc ) sack_ftruncate( file );
		SetLastError( dwError );
		return MapFileHandle( getHandle(), file );
	}
	SetLastError( dwError );
	return INVALID_HANDLE_VALUE;
	return (!file) ? INVALID_HANDLE_VALUE:file;// CreateFileA( lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile );
}		

static HANDLE WINAPI CreateFileW_stub(   LPCWSTR               lpFileName,
      DWORD                 dwDesiredAccess,
      DWORD                 dwShareMode,
      LPSECURITY_ATTRIBUTES lpSecurityAttributes,
      DWORD                 dwCreationDisposition,
      DWORD                 dwFlagsAndAttributes,
      HANDLE                hTemplateFile ) {

	TEXTSTR fDel = DupWideToText( lpFileName );
lprintf( "input filename: %s", fDel );
LogBinary( lpFileName, 64 );
	TEXTSTR f = fDel;
	if( fld.leadinPath ) {
		if( StrCmpEx( f, fld.leadinPath, fld.leadinLen ) == 0 ) {
			f += fld.leadinLen;
		}
	}
//lprintf( "path in is %s", 
	TEXTSTR fTmp = pathrchr( (TEXTSTR)f );// + 1;
	DWORD dwError = ERROR_SUCCESS;
	LOGICAL trunc = FALSE;
	FILE * file;
	if( fld._debug ) 
		lprintf( "create fileW called %s cre %08x attr %08x shr %08x acc %08x %p", f
			, dwCreationDisposition, dwFlagsAndAttributes
			, dwShareMode, dwDesiredAccess, hTemplateFile );
	if( !fTmp ) 
		fTmp = f; 
	else
		fTmp += 1;
	TEXTRUNE c = GetUtfChar( (CTEXTSTR*)&fTmp );
	if( fld._debug ) 
		lprintf( "c = %d", c );

	if( dwDesiredAccess & FILE_READ_ATTRIBUTES ) {
		// this is like a stat mode...
		return MapFileHandle( getHandle(), (FILE*)1 );

	}
	if( /*c == L'' ||*/ c && c == 916 && !GetUtfChar( (CTEXTSTR*)&fTmp ) ) {
		if( !fld.leadinPath ) {
			fld.leadinPath = StrDup( f );
			fTmp = pathrchr( fld.leadinPath );
			fTmp++;
			fTmp[0] = 0;
			fld.leadinLen = fTmp - fld.leadinPath;
		}else {
			lprintf( "new leadin? %s", f );
		}
		if( fld._debug ) 
			lprintf( "is this 916?" );        
		//file = sack_fopen( 0, "inject.js", "r" );
		return MapFileHandle( getHandle(), NULL );
		
	}
lprintf( "..." );
	if( f[0] ) {
		if( !(dwCreationDisposition & CREATE_ALWAYS) ) {
			if( dwCreationDisposition & CREATE_NEW ) {
				if( sack_exists( f ) ) {
				    SetLastError( ERROR_ALREADY_EXISTS );
				    return INVALID_HANDLE_VALUE;
				}
			}
			if( dwCreationDisposition & OPEN_ALWAYS ) {
				if( sack_exists( f ) ) {
				    dwError = ERROR_ALREADY_EXISTS;
				}
			}
			else if( dwCreationDisposition & OPEN_EXISTING) {
				if( !sack_exists( f ) ) {
					SetLastError( ERROR_FILE_NOT_FOUND );
					return INVALID_HANDLE_VALUE;
				}
			}
		}
		if( dwCreationDisposition & TRUNCATE_EXISTING ) {
			if( sack_exists( f ) ) 
				trunc = TRUE;
		}
		file = sack_fopen( 0, f, "r" );
	}
	else
		file = NULL;

	if( file ) {
		if( trunc ) sack_ftruncate( file );
		SetLastError( dwError );
		return MapFileHandle( getHandle(), file );
	}
	else {
		dwError = ERROR_FILE_NOT_FOUND;
lprintf( "..." );
		HANDLE h = CreateFileW( lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile );
		//lprintf( "hmm...%p %s", h, fDel );
		return h;
	}
	SetLastError( dwError );
	return INVALID_HANDLE_VALUE;
	return (!file) ? INVALID_HANDLE_VALUE : file;// CreateFileW( lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile );
}

static HANDLE WINAPI CreateFileMappingA_stub(
	HANDLE hFile,
	LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
	DWORD flProtect,
	DWORD dwMaximumSizeHigh,
	DWORD dwMaximumSizeLow,
	LPCSTR lpName
) {
	lprintf( "CreateFilemappingW : %s", lpName );
	return CreateFileMappingA( hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName );
}

static HANDLE WINAPI CreateFileMappingW_stub(
	HANDLE hFile,
	LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
	DWORD flProtect,
	DWORD dwMaximumSizeHigh,
	DWORD dwMaximumSizeLow,
	LPCWSTR lpName
) {
	lprintf( "CreateFilemappingW : %s", lpName );
	return CreateFileMappingW( hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName );
}

static HANDLE WINAPI OpenFileMappingW_stub(
	DWORD dwDesiredAccess,
	BOOL bInheritHandle,
	LPCWSTR lpName
) {
	lprintf( "OpenFilemappingW : %s", lpName );
	return OpenFileMappingW( dwDesiredAccess, bInheritHandle, lpName );
}


static HFILE WINAPI OpenFile_stub(
	LPCSTR lpFileName,
	LPOFSTRUCT lpReOpenBuff,
	UINT uStyle
) {
	lprintf( "OpenFile : %s", lpFileName );
	return (HFILE)OpenFile( lpFileName, lpReOpenBuff, uStyle );
}

static BOOL WINAPI ReadFile_stub(
	HANDLE hFile,
	LPVOID lpBuffer,
	DWORD nNumberOfBytesToRead,
	LPDWORD lpNumberOfBytesRead,
	LPOVERLAPPED lpOverlapped
) {
	FILE *f = GetFileFromHandle( hFile );
	if( f ) {
		size_t read;
		if( lpOverlapped ) {
			size_t pos = (uintptr_t)lpOverlapped->Pointer;
			sack_fseek( f, pos, SEEK_SET );
		}
		//lprintf( "read some of the file... %d", nNumberOfBytesToRead );
		read = sack_fread( lpBuffer, 1, nNumberOfBytesToRead, GetFileFromHandle( hFile) );
		if( lpNumberOfBytesRead ) lpNumberOfBytesRead[0] = (DWORD)read;
		if( lpOverlapped ) {
			if( lpOverlapped->hEvent ) SetEvent( lpOverlapped->hEvent );
			lpOverlapped->Internal = STATUS_WAIT_0;
			lprintf( "Overlap conditions to satisfy?" );
		}
	}else{
		lprintf( "this was an untraced read to %p", hFile );
		return ReadFile( hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped );
	}
	return TRUE;
	//return ReadFile( hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped );
}

static BOOL WINAPI WriteFile_stub(
	HANDLE hFile,
	LPVOID lpBuffer,
	DWORD nNumberOfBytesToRead,
	LPDWORD lpNumberOfBytesRead,
	LPOVERLAPPED lpOverlapped
) {
	FILE *f = GetFileFromHandle( hFile );
	if( f ) {
		size_t read;
		if( lpOverlapped ) {
			size_t pos = (uintptr_t)lpOverlapped->Pointer;
			sack_fseek( f, pos, SEEK_SET );
		}
		read = sack_fwrite( lpBuffer, 1, nNumberOfBytesToRead, f );
		lprintf( "this was a write" );
		if( lpNumberOfBytesRead ) lpNumberOfBytesRead[0] = (DWORD)read;
		if( lpOverlapped ) {
			if( lpOverlapped->hEvent ) SetEvent( lpOverlapped->hEvent );
			lpOverlapped->Internal = STATUS_WAIT_0;
			lprintf( "Overlap conditions to satisfy?" );
		}
		return TRUE;
	}
	else {
		lprintf( "this was an untraced write to %p", hFile );
		//CreateFileMapping
		return WriteFile( hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped );
	}

}

static HANDLE WINAPI FindFirstFileExA_stub(
	LPCSTR lpFileName,
	FINDEX_INFO_LEVELS fInfoLevelId,
	LPVOID lpFindFileData,
	FINDEX_SEARCH_OPS fSearchOp,
	LPVOID lpSearchFilter,
	DWORD dwAdditionalFlags
) {
	lprintf( "findfirstA" );
	return FindFirstFileExA( lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags );
}
static HANDLE WINAPI FindFirstFileA_stub(
	LPCSTR lpFileName,
	LPWIN32_FIND_DATAA lpFindFileData
){
	lprintf( "findfirstfileA" );
	return FindFirstFileA( lpFileName, lpFindFileData );
}

static HANDLE WINAPI FindFirstFileExW_stub(
	LPCWSTR lpFileName,
	FINDEX_INFO_LEVELS fInfoLevelId,
	LPVOID lpFindFileData,
	FINDEX_SEARCH_OPS fSearchOp,
	LPVOID lpSearchFilter,
	DWORD dwAdditionalFlags
) {
	lprintf( "findfirstW" );
	return FindFirstFileExW( lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags );
}

static TEXTSTR Extract( CTEXTSTR name )
{
	if( sack_existsEx( name, fld.rom ) )
	{
		FILE *file;
#ifdef DEBUG_LIBRARY_LOADING

		lprintf( "%s exists...", name );
#endif
		file = sack_fopenEx( 0, name, "rb", fld.rom );
		if( file )
		{
			CTEXTSTR path = ExpandPath( "*/tmp" );
			TEXTCHAR* tmpnam = NewArray( TEXTCHAR, 256 );
			size_t sz = sack_fsize( file );
			FILE *tmp;
#ifdef DEBUG_LIBRARY_LOADING
			lprintf( "library is %d %s", sz, name );
#endif
			MakePath( path );
			snprintf( tmpnam, 256, "%s/%s", path, name );
			tmp = sack_fopenEx( 0, tmpnam, "wb", sack_get_default_mount() );
#ifdef DEBUG_LIBRARY_LOADING
			lprintf( "Loading %s(%p)", tmpnam, tmp );
#endif
			if( sz && tmp )
			{
				size_t written, read ;
				POINTER data = NewArray( uint8_t, sz );
				read = sack_fread( data, sz, 1, file );
				written = sack_fwrite( data, sz, 1, tmp );
				sack_fclose( tmp );
				Release( data );
			}
			sack_fclose( file );
			return tmpnam;
		}
	}
	return NULL;
}



static HMODULE WINAPI LoadLibraryW_stub( LPCWSTR lpLibFileName ) {
	TEXTSTR a = DupWideToText( lpLibFileName );
	if( sack_exists( a ) ) {
		TEXTSTR b = Extract( a );
		return LoadLibrary( b );
	}
	lprintf( "LoadLibraryW %s", a );
	return LoadLibraryW( lpLibFileName );
}
static HMODULE WINAPI LoadLibraryExW_stub( LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags ) {
	lprintf( "LoadLibraryExW" );
	return LoadLibraryExW( lpLibFileName, hFile, dwFlags );
}
static HMODULE WINAPI LoadLibraryA_stub( LPCSTR lpLibFileName ) {
	lprintf( "LoadLibraryA" );
	return LoadLibraryA( lpLibFileName );

}

static BOOL WINAPI FindNextFileW_stub(
	HANDLE hFindFile,
	LPWIN32_FIND_DATAW lpFindFileData
) {
	lprintf( "FindNextFileW" );
	return FindNextFileW( hFindFile, lpFindFileData );

}


static BOOL WINAPI FindNextFileA_stub(
	HANDLE hFindFile,
	LPWIN32_FIND_DATAA lpFindFileData
) {
	lprintf( "FindNextFileA" );
	return FindNextFileA( hFindFile, lpFindFileData );
}

static BOOL FindClose_stub( HANDLE hFile ) {
	lprintf( "Find close..." );
	return FindClose( hFile );
}

static BOOL WINAPI CloseHandle_stub( HANDLE hFile ) {
	lprintf( "closefile stub called %p", hFile );
	if( hFile != INVALID_HANDLE_VALUE ) {
		// MapFileHandle closes the file handle too when unmapping
		FILE *f = (FILE*)MapFileHandle( hFile, NULL );
		if( f ) {
			if( (uintptr_t)f == 1 )
				return TRUE;
			return sack_fclose( f );
		}
		//lprintf( "this wasn't one of mine...." );
		return CloseHandle( hFile ); 
	}
	return FALSE;
}


static DWORD WINAPI GetFullPathNameW_stub(
	LPCWSTR lpFileName,
	DWORD nBufferLength,
	LPWSTR lpBuffer,
	LPWSTR * lpFilePart
)
{
	lprintf( "GetFullPathNameW" );
	return GetFullPathNameW( lpFileName, nBufferLength, lpBuffer, lpFilePart );
}


static DWORD WINAPI GetLongPathNameW_stub(
	LPCWSTR lpszShortPath,
	LPWSTR lpszLongPath,
	DWORD cchBuffer
)
{
	lprintf( "GetLongPathName" );
	return GetLongPathNameW(lpszShortPath, lpszLongPath, cchBuffer);
}


static DWORD WINAPI GetShortPathNameW_stub(
	LPCWSTR lpszShortPath,
	LPWSTR lpszLongPath,
	DWORD cchBuffer
)
{
	lprintf( "GetShortPathName" );
	return GetShortPathNameW( lpszShortPath, lpszLongPath, cchBuffer );
}

static BOOL WINAPI SetFileAttributesW_stub( LPCWSTR lpFileName,
	_In_ DWORD dwFileAttributes )
{
	lprintf( "SetFileAttributesW" );
	return SetFileAttributesW( lpFileName, dwFileAttributes );
}
static BOOL WINAPI GetFileAttributesExW_stub( 
	LPCWSTR lpFileName,
	GET_FILEEX_INFO_LEVELS fInfoLevelId,
	LPVOID lpFileInformation
)
{
	lprintf( "GetFileAttributesExW.... (missing logging?)" );
	return GetFileAttributesExW( lpFileName, fInfoLevelId, lpFileInformation );
}

static DWORD WINAPI GetCurrentDirectoryW_stub( DWORD nBufferLength, LPWSTR lpBuffer ) {
	lprintf( "Directory: %s", DupWideToText( lpBuffer ));
	lpBuffer[0] = '\\';
	lpBuffer[1] = '\\';
	lpBuffer[2] = '.';
	lpBuffer[3] = '\\';
	lpBuffer[4] = 'c';
	lpBuffer[5] = ':';
	lpBuffer[6] = '\\';
	lpBuffer[7] = 'a';
	lpBuffer[8] = '\\';
	lpBuffer[9] = 'b';
	lpBuffer[10] = '\\';
	lpBuffer[11] = '\\';
	lpBuffer[12] = 0;

	//lpBuffer[0] = '.';
	//lpBuffer[1] = 0;
	lprintf( "return current directory..." );
	return GetCurrentDirectoryW( nBufferLength, lpBuffer );
	return 10;
}
static BOOL WINAPI SetCurrentDirectoryW_stub( LPCWSTR lpBuffer ) {
	BOOL x = SetCurrentDirectoryW( lpBuffer );
	lprintf( "SET current directory..." );
	return 2;
}

static DWORD WINAPI GetFileAttributesW_stub( LPCWSTR lpFileName ) {
	//GetFileAttributesW();
	lprintf( "GetFileAttributesW" );
	return GetFileAttributesW( lpFileName );
}

static BOOL WINAPI GetFileInformationByHandleEx_stub( HANDLE hFile,
	FILE_INFO_BY_HANDLE_CLASS FileInformationClass, LPVOID lpFileInformation,DWORD dwBufferSize
) {
	lprintf( "getfileinformationbyhandleEx" );
	return GetFileInformationByHandleEx( hFile, FileInformationClass, lpFileInformation, dwBufferSize );
}
static BOOL WINAPI GetFileInformationByHandle_stub( HANDLE hFile,
	LPBY_HANDLE_FILE_INFORMATION lpFileInformation ) {
	lprintf( "getfileinformationbyhandle" );
	return  GetFileInformationByHandle( hFile, lpFileInformation );
}

static DWORD WINAPI GetFileType_stub( HANDLE hFile ) {
	FILE *f = GetFileFromHandle( hFile );
	if( f ) {
		//lprintf( "getFileType: %p", hFile );  // stdin stdout probably
		return FILE_TYPE_DISK;
	}
	else {
		DWORD dwVal = GetFileType( hFile );
		//2 = FILE_TYPE_CHAR
		//lprintf( "getFileType: %p %d", hFile, dwVal );  // stdin stdout probably
		return dwVal;
	}
}

static LPVOID WINAPI MapViewOfFile_stub( HANDLE hFileMappingObject,
	DWORD dwDesiredAccess,
	DWORD dwFileOffsetHigh,
	DWORD dwFileOffsetLow,
	SIZE_T dwNumberOfBytesToMap ) {
	//SetFilePointerEx
	lprintf( "MapViewOfFile..." );
	return MapViewOfFile( hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, dwNumberOfBytesToMap );
}

static LPVOID WINAPI MapViewOfFileEx_stub( HANDLE hFileMappingObject,
	 DWORD dwDesiredAccess,
	DWORD dwFileOffsetHigh,
	DWORD dwFileOffsetLow,
	SIZE_T dwNumberOfBytesToMap,
	LPVOID lpBaseAddress ) {
	//SetFilePointerEx
	lprintf( "MapViewOfFileEx..." );
	return MapViewOfFileEx( hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, dwNumberOfBytesToMap, lpBaseAddress );
}

static BOOL WINAPI SetFilePointerEx_stub( HANDLE hFile,
	LARGE_INTEGER liDistanceToMove,
	PLARGE_INTEGER lpNewFilePointer,
	DWORD dwMoveMethod ) {
	//SetFilePointerEx
	lprintf( "SetFilePointerEx" );
	return SetFilePointerEx( hFile, liDistanceToMove, lpNewFilePointer, dwMoveMethod );
}


static BOOL WINAPI SetFilePointer_stub( HANDLE hFile,
	LONG lDistanceToMove,
	PLONG lpDistanceToMoveHigh,
	DWORD dwMoveMethod ) {
	//SetFilePointerEx
	lprintf( "SetFilePointer" );
	return SetFilePointer( hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod );
}

typedef NTSTATUS( NTAPI *sNtQueryInformationFile )
(HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID FileInformation,
	ULONG Length,
	FILE_INFORMATION_CLASS FileInformationClass);
//extern "C" sNtQueryInformationFile pNtQueryInformationFile;
static  NTSTATUS  NTAPI pNtQueryInformationFile_stub( HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID FileInformation,
	ULONG Length,
	FILE_INFORMATION_CLASS FileInformationClass)
{
	FILE *f = (FILE*)MapFileHandle( FileHandle, NULL );
	if( f ) {
		if( (uintptr_t)f == 1 ) {

			return TRUE;
		}
	}
	return 0;// pNtQueryInformationFile( FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass );
}

static BOOL WINAPI DuplicateHandle_stub( HANDLE hSourceProcessHandle,
	HANDLE hSourceHandle,
	HANDLE hTargetProcessHandle,
	LPHANDLE lpTargetHandle,
	DWORD dwDesiredAccess,
	BOOL bInheritHandle,
	DWORD dwOptions ) {
	//SetFilePointerEx
	lprintf( "DuplicateHandle" );
	return DuplicateHandle( hSourceProcessHandle, hSourceHandle, hTargetProcessHandle, 
			lpTargetHandle, dwDesiredAccess, 
			bInheritHandle, dwOptions );
}

static BOOL WINAPI GetFileSize_stub( HANDLE hFile,
	LPDWORD lpFileSizeHigh ) {
	//SetFilePointerEx
	lprintf( "GetFileSize" );
	return GetFileSize( hFile, lpFileSizeHigh );
}

static BOOL WINAPI GetFileSizeEx_stub( HANDLE hFile,
	PLARGE_INTEGER lpFileSizeHigh ) {
	//SetFilePointerEx
	lprintf( "GetFileSizeEx" );
	return GetFileSizeEx( hFile, lpFileSizeHigh );
}

static BOOL WINAPI CreateDirectoryW_stub( LPCWSTR lpPathName,
	 LPSECURITY_ATTRIBUTES lpSecurityAttributes ) {
	//SetFilePointerEx
	lprintf( "CreateDirectoryW" );
	return CreateDirectoryW( lpPathName, lpSecurityAttributes );
}
static FILE * fopen_stub( const char*a, const char *b ) {
	lprintf( "fopen hook." );
		return sack_fopen( 0, a, b );
}
static int fclose_stub( FILE *a ) {
	lprintf( "fclose hook." );
		return sack_fclose( a );
}

static size_t fread_stub( char *a, size_t b, int c, FILE *d ) {
	lprintf( "fclose hook." );
		return sack_fread( a,b,c,d );
}

static size_t fwrite_stub( const char *a, int b, int c, FILE *d ) {
	lprintf( "fclose hook." );
		return sack_fwrite( a,b,c,d );
}

static int _wfsopen_stub( const char *a, int b, int c, FILE *d ) {
	lprintf( "_wfsopen hook." );
		return sack_fwrite( a, b, c, d );
}

static int _wsopen_stub( const wchar_t *a, int b, int c, int d ) {
	lprintf( "_wsopen hook." );
		return _wsopen( a, b, c, d );
}

static HFILE _lopen_stub( LPCSTR lpPathName, int iReadWrite ) {
	lprintf( "_lopen hook." );
		return _lopen( lpPathName, iReadWrite );
}
static int _open_osfhandle_stub( intptr_t _OSFileHandle,
	int      _Flags ) {
	lprintf( "_open_osfhandle hook" );
		return _open_osfhandle( _OSFileHandle, _Flags );
}


static void FakeAbort( void )
{
	TerminateProcess( GetCurrentProcess(), 0 );
}

static struct fixup_table_entry fixup_entries[] = { { "libgcc_s_dw2-1.dll", "msvcrt.dll", "abort", (POINTER)FakeAbort } 
					, { "kernel32.dll", "kernel32.dll", "CreateFileA", CreateFileA_stub }
					, { "kernel32.dll", "kernel32.dll", "CreateFileW", CreateFileW_stub }
					, { "kernel32.dll", "kernel32.dll", "ReadFile", ReadFile_stub }
	,{ "kernel32.dll", "kernel32.dll", "LoadLibraryA", LoadLibraryA_stub }
	,{ "kernel32.dll", "kernel32.dll", "LoadLibraryW", LoadLibraryW_stub }
	,{ "kernel32.dll", "kernel32.dll", "LoadLibraryW", LoadLibraryExW_stub }
	,{ "kernel32.dll", "kernel32.dll", "WriteFile", WriteFile_stub }
	,{ "kernel32.dll", "kernel32.dll", "OpenFile", OpenFile_stub }
	,{ "kernel32.dll", "kernel32.dll", "CloseHandle", CloseHandle_stub }
	,{ "kernel32.dll", "kernel32.dll", "CreateFileMappingA", CreateFileMappingA_stub }
	,{ "kernel32.dll", "kernel32.dll", "CreateFileMappingW", CreateFileMappingW_stub }
	,{ "kernel32.dll", "kernel32.dll", "OpenFileMappingW", OpenFileMappingW_stub }
	,{ "kernel32.dll", "kernel32.dll", "FindFirstFileExA", FindFirstFileExA_stub }
	,{ "kernel32.dll", "kernel32.dll", "FindFirstFileA", FindFirstFileA_stub }
	,{ "kernel32.dll", "kernel32.dll", "FindFirstFileExW", FindFirstFileExW_stub }
	,{ "kernel32.dll", "kernel32.dll", "FindNextFileA", FindNextFileA_stub }
	,{ "kernel32.dll", "kernel32.dll", "FindNextFileW", FindNextFileW_stub }
	,{ "kernel32.dll", "kernel32.dll", "FindClose", FindClose_stub }
	,{ "kernel32.dll", "kernel32.dll", "GetFileAttributesW", GetFileAttributesW_stub }
	,{ "kernel32.dll", "kernel32.dll", "GetLongPathNameW", GetLongPathNameW_stub }
	,{ "kernel32.dll", "kernel32.dll", "GetShortPathNameW", GetShortPathNameW_stub }
	,{ "kernel32.dll", "kernel32.dll", "GetFullPathNameW", GetFullPathNameW_stub }
	,{ "kernel32.dll", "kernel32.dll", "GetCurrentDirectoryW", GetCurrentDirectoryW_stub }
	,{ "kernel32.dll", "kernel32.dll", "GetFileAttributesExW", GetFileAttributesExW_stub }
	,{ "kernel32.dll", "kernel32.dll", "SetFileAttributesExW", SetFileAttributesW_stub }
	,{ "kernel32.dll", "kernel32.dll", "GetFileInformationByHandle", GetFileInformationByHandle_stub }
	,{ "kernel32.dll", "kernel32.dll", "GetFileType", GetFileType_stub }
	,{ "kernel32.dll", "kernel32.dll", "SetFilePointerEx", SetFilePointerEx_stub }
	,{ "kernel32.dll", "kernel32.dll", "SetFilePointer", SetFilePointer_stub }
	,{ "kernel32.dll", "kernel32.dll", "CreateDirectoryW", CreateDirectoryW_stub }
	,{ "kernel32.dll", "kernel32.dll", "GetFileInformationByHandleEx", GetFileInformationByHandleEx_stub }
	,{ "kernel32.dll", "kernel32.dll", "GetFileSize", GetFileSize_stub }
	,{ "kernel32.dll", "kernel32.dll", "GetFileSizeEx", GetFileSizeEx_stub }
	,{ "kernel32.dll", "kernel32.dll", "MapViewOfFile", MapViewOfFile_stub }
	,{ "kernel32.dll", "kernel32.dll", "DuplicateHandle", DuplicateHandle_stub }
#define NTDLL(n) { "ntdll.dll", "ntdll.dll", #n, n##_stub }
	, NTDLL( pNtQueryInformationFile )
#define MSC(n)	{ "msvcrt.dll", "msvcrt.dll", #n, n##_stub }
	,{ "msvcrt.dll", "msvcrt.dll", "fopen", fopen_stub }
	,MSC(fclose)
	, MSC(fread)
	,MSC(fwrite)
	,MSC(_wfsopen)
	,MSC( _wsopen )
	,MSC( _lopen )
	,MSC( _open_osfhandle )
	
	, {NULL }
					};

static void *MyAlloc( size_t size )
{
	return Allocate( size );
}

static void MyFree( void *p )
{
	Deallocate( void *, p );
}
static void * MyRealloc( void *p, size_t size )
{
	return Reallocate( p, size );
}

#define Seek(a,b) (((uintptr_t)a)+(b))



static void FixupImage( POINTER base )
{
	struct fixup_table_entry *entry = fixup_entries;
	LOGICAL gcclib = TRUE;
	POINTER source_memory = base;
	//printf( "Load %s (%d:%d)\n", name, generation, level );
	{
		PIMAGE_DOS_HEADER source_dos_header = (PIMAGE_DOS_HEADER)source_memory;
		PIMAGE_NT_HEADERS source_nt_header = (PIMAGE_NT_HEADERS)Seek( source_memory, source_dos_header->e_lfanew );
//		lprintf( "v1 v2 %p %p", static_source_nt_header->OptionalHeader.ImageBase, source_nt_header->OptionalHeader.ImageBase );
		if( source_dos_header->e_magic != IMAGE_DOS_SIGNATURE )
		{
			lprintf( "Basic signature check failed; not a library" );
			return;
		}

		if( source_nt_header->FileHeader.SizeOfOptionalHeader )
		{
			if( source_nt_header->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC )
			{
				//lprintf( "Optional header signature is incorrect... %x = %x",source_nt_header->OptionalHeader.Magic, IMAGE_NT_OPTIONAL_HDR_MAGIC );
			}
		}
		{
			//int n;

			PIMAGE_DATA_DIRECTORY dir = (PIMAGE_DATA_DIRECTORY)source_nt_header->OptionalHeader.DataDirectory;
			PIMAGE_EXPORT_DIRECTORY exp_dir = (PIMAGE_EXPORT_DIRECTORY)Seek( source_memory, dir[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress );
			const char *dll_name = (const char*) Seek( source_memory, exp_dir->Name );
			if( strcmp( dll_name, TARGETNAME ) == 0 ) {
				// skiping self.
				return;
			}
			lprintf( " ------------- Loading %s -------------", dll_name );

			long FPISections = source_dos_header->e_lfanew
				+ sizeof( DWORD ) + sizeof( IMAGE_FILE_HEADER )
				+ source_nt_header->FileHeader.SizeOfOptionalHeader;
			PIMAGE_SECTION_HEADER source_section;
			PIMAGE_IMPORT_DESCRIPTOR real_import_base;
			PIMAGE_SECTION_HEADER source_import_section = NULL;
			PIMAGE_SECTION_HEADER source_text_section = NULL;
			uintptr_t dwSize = 0;
			//uintptr_t newSize;
			source_section = (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections );
			for( int n = 0; n < source_nt_header->FileHeader.NumberOfSections; n++ )
			{
				uintptr_t newSize = (source_section[n].VirtualAddress) + source_section[n].SizeOfRawData;
				if( newSize > dwSize )
					dwSize = newSize;
			}

			// compute size of total of sections
			// mark a few known sections for later processing
			// ------------- Go through the sections and move to expected virtual offsets
			real_import_base = (PIMAGE_IMPORT_DESCRIPTOR)Seek( source_memory, dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress  );
			if( real_import_base &&  dir[IMAGE_DIRECTORY_ENTRY_IMPORT].Size )
			{
				int n;
				for( n = 0; real_import_base[n].Characteristics; n++ )
				{
					const char * dll_name;
					int f;
					uintptr_t *dwFunc;
					uintptr_t *dwTargetFunc;
					PIMAGE_IMPORT_BY_NAME import_desc;
					if( real_import_base[n].Name )
						dll_name = (const char*) Seek( real_import_base, real_import_base[n].Name - dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress/*source_import_section->VirtualAddress*/ );
					if( gcclib )
					{
						int e;
						for( e = 0; entry[e].import_libname; e++ )
						if( !StrCaseCmp( dll_name, entry[e].import_libname ) )
						{
							//lprintf( "skipping %s", dll_name );
							break;
						}
						if( !entry[e].import_libname )
							continue;
					}
					//lprintf( "import %s", dll_name );
					//char * function_name = (char*) Seek( import_base, import_base[n]. - source_import_section->VirtualAddress );
#if __WATCOMC__ && __WATCOMC__ < 1200
					dwFunc = (uintptr_t*)Seek( real_import_base, real_import_base[n].OrdinalFirstThunk - dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress );
#else
					dwFunc = (uintptr_t*)Seek( real_import_base, real_import_base[n].OriginalFirstThunk - dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress );
#endif
					dwTargetFunc = (uintptr_t*)Seek( base, real_import_base[n].FirstThunk );
					for( f = 0; dwFunc[f]; f++ )
					{
						if( dwFunc[f] & ( (uintptr_t)1 << ( ( sizeof( uintptr_t ) * 8 ) - 1 ) ) )
						{
							//lprintf( "Oridinal is %d", dwFunc[f] & 0xFFFF );
						}
						else
						{
							import_desc = (PIMAGE_IMPORT_BY_NAME)Seek( source_memory, dwFunc[f] );
							lprintf( " sub thing %s %s", dll_name, import_desc->Name );
							if( gcclib )
							{
								int e;
								for( e = 0; entry[e].import_libname; e++ ) {
									if( StrCmp( (CTEXTSTR)import_desc->Name, entry[e].import_name ) == 0 )
									{
										DWORD old_protect;
										VirtualProtect( (POINTER)(dwTargetFunc + f)
											, 4
											, PAGE_EXECUTE_READWRITE
											, &old_protect );
										lprintf( "(FIXED) %s", import_desc->Name );
										dwTargetFunc[f] = (uintptr_t)entry[e].pointer;
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
}



PRELOAD( FixLink )
{
	MessageBox( NULL, "STOP", "PAUSE", MB_OK );
	fld._debug = 1;
	fld.fsi = sack_get_filesystem_interface( "sack_shmem" );
	//POINTER base = (POINTER)GetModuleHandle(NULL);
	//FixupImage( base );

	{
		DWORD pid = GetCurrentProcessId();
		HANDLE hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pid);
		HMODULE *modules = NULL;
		DWORD cb = 0;
		DWORD needed = 0;
		if( EnumProcessModules( hProcess, modules, cb,&needed ) ) {
			needed /= sizeof( HMODULE );
			modules = NewArray( HMODULE, needed );		
			EnumProcessModules( hProcess, modules, needed*sizeof(HMODULE),&cb );
			for( cb = 0; cb < needed; cb++ ) {
				//lprintf( "Fixup: %p", modules[cb] );
				FixupImage( modules[cb] );
			}
		}
		Deallocate( HMODULE*, modules );
	}

}

