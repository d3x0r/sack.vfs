
#include <sack.h>
#include <TlHelp32.h>

#include "ntundoc.h"


struct fixup_table_entry {
	CTEXTSTR libname;
	CTEXTSTR import_libname;
	CTEXTSTR import_name;
	POINTER pointer;
	LOGICAL fixed;
};


static void FakeAbort( void );

struct fileHandleMap {
	FILE *file;
	HANDLE handle;
};

static struct fixup_local_data {
	PLIST handleMap;
	int usedHandles;
	HANDLE pipe[2];
} fld;


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
		struct fileHandleMap *ent = New( struct fileHandleMap );
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
	TEXTSTR f = DupWideToText( lpFileName );
	DWORD dwError = ERROR_SUCCESS;
	LOGICAL trunc = FALSE;
	FILE * file;
	lprintf( "create fileW called %S cre %08x attr %08x shr %08x acc %08x %p", lpFileName
		, dwCreationDisposition, dwFlagsAndAttributes
		, dwShareMode, dwDesiredAccess, hTemplateFile );
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

	if( file ) {
		if( trunc ) sack_ftruncate( file );
		SetLastError( dwError );
		return MapFileHandle( getHandle(), file );
	}
	else {
		dwError = ERROR_FILE_NOT_FOUND;
		HANDLE h = CreateFileW( lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile );
		lprintf( "hmm...%p", h );
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
) {
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


static HMODULE WINAPI LoadLibraryW_stub( LPCWSTR lpLibFileName ) {
	TEXTSTR a = DupWideToText( lpLibFileName );
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
	lprintf( "closefile stub called" );
	if( hFile != INVALID_HANDLE_VALUE ) {
		FILE *f = (FILE*)MapFileHandle( hFile, NULL );
		if( f )
			return sack_fclose( f );
		lprintf( "this wasn't one of mine...." );
		return CloseHandle( hFile ); 
	}
	return FALSE;
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
	return GetFileAttributesExW( lpFileName, fInfoLevelId, lpFileInformation );
}

static DWORD WINAPI GetCurrentDirectoryW_stub( DWORD nBufferLength, LPWSTR lpBuffer ) {
	lpBuffer[0] = '.';
	lpBuffer[1] = 0;
	lprintf( "return current directory..." );
	return 1;
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

static BOOL WINAPI GetFileInformationByHandle_stub( HANDLE hFile,
	LPBY_HANDLE_FILE_INFORMATION lpFileInformation ) {
	lprintf( "getfileinformationbyhandleW" );
	return  GetFileInformationByHandle( hFile, lpFileInformation );
}

static DWORD WINAPI GetFileType_stub( HANDLE hFile ) {
	FILE *f = GetFileFromHandle( hFile );
	if( f ) {
		lprintf( "getFileType: %p", hFile );  // stdin stdout probably
		return FILE_TYPE_DISK;
	}
	else {
		DWORD dwVal = GetFileType( hFile );
		//2 = FILE_TYPE_CHAR
		lprintf( "getFileType: %p %d", hFile, dwVal );  // stdin stdout probably
		return dwVal;
	}
}

static BOOL WINAPI SetFilePointerEx_stub( HANDLE hFile,
	LARGE_INTEGER liDistanceToMove,
	PLARGE_INTEGER lpNewFilePointer,
	DWORD dwMoveMethod ) {
	//SetFilePointerEx
	lprintf( "SetFilePointerEx" );
	return SetFilePointerEx( hFile, liDistanceToMove, lpNewFilePointer, dwMoveMethod );
}

static struct fixup_table_entry fixup_entries[] = { { "libgcc_s_dw2-1.dll", "msvcrt.dll", "abort", (POINTER)FakeAbort } 
					, { "kernel32.dll", "kernel32.dll", "CreateFileA", CreateFileA_stub }
					, { "kernel32.dll", "kernel32.dll", "CreateFileW", CreateFileW_stub }
					, { "kernel32.dll", "kernel32.dll", "ReadFile", ReadFile_stub }
	,{ "kernel32.dll", "kernel32.dll", "LoadLibraryA", LoadLibraryA_stub }
	,{ "kernel32.dll", "kernel32.dll", "LoadLibraryW", LoadLibraryW_stub }
	,{ "kernel32.dll", "kernel32.dll", "LoadLibraryW", LoadLibraryExW_stub }
	,{ "kernel32.dll", "kernel32.dll", "WriteFile", WriteFile_stub }
	,{ "kernel32.dll", "kernel32.dll", "CloseHandle", CloseHandle_stub }
	,{ "kernel32.dll", "kernel32.dll", "CreateFileMappingA", CreateFileMappingA_stub }
	,{ "kernel32.dll", "kernel32.dll", "CreateFileMappingW", CreateFileMappingW_stub }
	,{ "kernel32.dll", "kernel32.dll", "FindFirstFileExA", FindFirstFileExA_stub }
	,{ "kernel32.dll", "kernel32.dll", "FindFirstFileA", FindFirstFileA_stub }
	,{ "kernel32.dll", "kernel32.dll", "FindFirstFileExW", FindFirstFileExW_stub }
	,{ "kernel32.dll", "kernel32.dll", "FindNextFileA", FindNextFileA_stub }
	,{ "kernel32.dll", "kernel32.dll", "FindNextFileW", FindNextFileW_stub }
	,{ "kernel32.dll", "kernel32.dll", "FindClose", FindClose_stub }
	,{ "kernel32.dll", "kernel32.dll", "GetFileAttributesW", GetFileAttributesW_stub }
	,{ "kernel32.dll", "kernel32.dll", "GetLongPathNameW", GetLongPathNameW_stub }
	,{ "kernel32.dll", "kernel32.dll", "GetShortPathNameW", GetShortPathNameW_stub }
	,{ "kernel32.dll", "kernel32.dll", "GetCurrentDirectoryW", GetCurrentDirectoryW_stub }
	,{ "kernel32.dll", "kernel32.dll", "GetFileAttributesExW", GetFileAttributesExW_stub }
	,{ "kernel32.dll", "kernel32.dll", "SetFileAttributesExW", SetFileAttributesW_stub }
	,{ "kernel32.dll", "kernel32.dll", "GetFileInformationByHandle", GetFileInformationByHandle_stub }
	,{ "kernel32.dll", "kernel32.dll", "GetFileType", GetFileType_stub }
	,{ "kernel32.dll", "kernel32.dll", "SetFilePointerEx", SetFilePointerEx_stub }

	, {NULL }
					};


static void FakeAbort( void )
{
	TerminateProcess( GetCurrentProcess(), 0 );
}

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


uintptr_t ConvertVirtualToPhysical( PIMAGE_SECTION_HEADER sections, int nSections, uintptr_t base )
{
	int n;
	for( n = 0; n < nSections; n++ )
	{
		if( base >= sections[n].VirtualAddress && base < sections[n].VirtualAddress + sections[n].SizeOfRawData )
			return base - sections[n].VirtualAddress + sections[n].PointerToRawData;
	}
	return 0;
}

PRELOAD( FixLink )
{
	POINTER base = (POINTER)GetModuleHandle(NULL);
	struct fixup_table_entry *entry = fixup_entries;
	LOGICAL gcclib = TRUE;
	POINTER source_memory = base;
#if 0
	char file[256];
	size_t dwSize;
	GetModuleFileName( NULL, file, 256 );
	POINTER disk_memory = OpenSpace( NULL, file, &dwSize );
#endif
	//printf( "Load %s (%d:%d)\n", name, generation, level );
	{
#if 0
		PIMAGE_DOS_HEADER static_source_dos_header = (PIMAGE_DOS_HEADER)disk_memory;
		PIMAGE_NT_HEADERS static_source_nt_header = (PIMAGE_NT_HEADERS)Seek( disk_memory, static_source_dos_header->e_lfanew );
#endif
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
			long FPISections = source_dos_header->e_lfanew
				+ sizeof( DWORD ) + sizeof( IMAGE_FILE_HEADER )
				+ source_nt_header->FileHeader.SizeOfOptionalHeader;
			PIMAGE_SECTION_HEADER source_section;
			PIMAGE_IMPORT_DESCRIPTOR real_import_base;
			PIMAGE_SECTION_HEADER source_import_section = NULL;
			PIMAGE_SECTION_HEADER source_text_section = NULL;
#if 0

			PIMAGE_DATA_DIRECTORY static_dir = (PIMAGE_DATA_DIRECTORY)static_source_nt_header->OptionalHeader.DataDirectory;
			long static_FPISections = static_source_dos_header->e_lfanew
				+ sizeof( DWORD ) + sizeof( IMAGE_FILE_HEADER )
				+ static_source_nt_header->FileHeader.SizeOfOptionalHeader;
			PIMAGE_SECTION_HEADER static_source_section;
			PIMAGE_IMPORT_DESCRIPTOR static_real_import_base;
			PIMAGE_SECTION_HEADER static_source_import_section = NULL;
			PIMAGE_SECTION_HEADER static_source_text_section = NULL;
#endif
			uintptr_t dwSize = 0;
			//uintptr_t newSize;
			source_section = (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections );
#if 0
			static_source_section = (PIMAGE_SECTION_HEADER)Seek( disk_memory, static_FPISections );
#endif
			for( int n = 0; n < source_nt_header->FileHeader.NumberOfSections; n++ )
			{
				uintptr_t newSize = (source_section[n].VirtualAddress) + source_section[n].SizeOfRawData;
				if( newSize > dwSize )
					dwSize = newSize;
			}
#if 0
			uintptr_t diskBias = ((uintptr_t)source_import_section->VirtualAddress - (uintptr_t)source_import_section->PointerToRawData);

			{
				//uintptr_t source_address = ConvertVirtualToPhysical( source_import_section, source_nt_header->FileHeader.NumberOfSections, dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress );
				static_real_import_base = (PIMAGE_IMPORT_DESCRIPTOR)Seek( disk_memory, static_dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress - diskBias );
			}
#endif

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
#if 0
					uintptr_t *dwFuncStatic;
					uintptr_t *dwTargetFuncStatic;
					PIMAGE_IMPORT_BY_NAME import_desc2;
#endif
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
					lprintf( "import %s", dll_name );
					//char * function_name = (char*) Seek( import_base, import_base[n]. - source_import_section->VirtualAddress );
					//printf( "thing %s\n", dll_name );
#if __WATCOMC__ && __WATCOMC__ < 1200
					dwFunc = (uintptr_t*)Seek( real_import_base, real_import_base[n].OrdinalFirstThunk - dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress );
#else
					dwFunc = (uintptr_t*)Seek( real_import_base, real_import_base[n].OriginalFirstThunk - dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress );
#endif
					dwTargetFunc = (uintptr_t*)Seek( base, real_import_base[n].FirstThunk );
#if 0
					uintptr_t diskaddress = ConvertVirtualToPhysical( source_import_section, source_nt_header->FileHeader.NumberOfSections, static_dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress );
					dwTargetFuncStatic = (uintptr_t*)Seek( static_real_import_base, real_import_base[n].FirstThunk - dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress - diskBias );
#endif
					for( f = 0; dwFunc[f]; f++ )
					{
						if( dwFunc[f] & ( (uintptr_t)1 << ( ( sizeof( uintptr_t ) * 8 ) - 1 ) ) )
						{
							lprintf( "Oridinal is %d", dwFunc[f] & 0xFFFF );
							//dwTargetFunc[f] = (uintptr_t)LoadFunction( dll_name, (CTEXTSTR)(dwFunc[f] & 0xFFFF) );
						}
						else
						{
							import_desc = (PIMAGE_IMPORT_BY_NAME)Seek( source_memory, dwFunc[f] );
							//import_desc2 = (PIMAGE_IMPORT_BY_NAME)Seek( disk_memory, dwTargetFunc[f] );
	
//////lprintf( "Addr of targetfunc is %p %p", dwFunc, !IsBadReadPtr( dwFunc + f*sizeof(void*), sizeof(void*) )?dwFunc[f]:NULL );
//lprintf( "Addr of targetfunc is %p %p", dwTargetFunc, !IsBadReadPtr( dwTargetFunc + f*sizeof(void*), sizeof(void*) )?dwTargetFunc[f]:NULL );
		////					//lprintf( " sub thing2 %p  %p", import_desc2->Name, import_desc2->Hint );
							lprintf( " sub thing %s", import_desc->Name );
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
#ifdef __64__
										lprintf( "No solution for 64bit; these are all 0 in a 64bit program" );
#else
										lprintf( "Fixed abort...%s  %p to %p", entry[e].import_name, dwTargetFunc + f, entry->pointer );
										dwTargetFunc[f] = (uintptr_t)entry[e].pointer;
#endif
										break;
									}
								}
							}
#if 0
							else
							{
			{
				DWORD old_protect;
				VirtualProtect( (POINTER)( dwTargetFunc + f )
						, 4
						, PAGE_EXECUTE_READWRITE
						, &old_protect );
								if( StrCmp( (CTEXTSTR)import_desc->Name, "malloc" ) == 0 )
								{
									dwTargetFunc[f] = (uintptr_t)MyAlloc;
									//return;
								}
								if( StrCmp( (CTEXTSTR)import_desc->Name, "free" ) == 0 )
								{
									dwTargetFunc[f] = (uintptr_t)MyFree;
									//return;
								}
								if( StrCmp( (CTEXTSTR)import_desc->Name, "realloc" ) == 0 )
								{
									dwTargetFunc[f] = (uintptr_t)MyRealloc;
									//return;
								}

				VirtualProtect( (POINTER)( dwTargetFunc + f )
						, 4
						, old_protect
						, &old_protect );
			}
							}
#endif
						}
					}
				}
			}
		}
	}
}

