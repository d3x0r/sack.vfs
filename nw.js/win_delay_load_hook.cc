/*
 * When this file is linked to a DLL, it sets up a delay-load hook that
 * intervenes when the DLL is trying to load 'node.exe' or 'iojs.exe'
 * dynamically. Instead of trying to locate the .exe file it'll just return
 * a handle to the process image.
 *
 * This allows compiled addons to work when node.exe or iojs.exe is renamed.
 */

#if defined _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <delayimp.h>
#include <string.h>

static HMODULE node_exe = NULL;
static HMODULE node_dll = NULL;
static HMODULE nw_dll = NULL;
static BOOL inited = FALSE;

static FARPROC WINAPI load_exe_hook(unsigned int event, DelayLoadInfo* info) {
  if (event == dliNotePreGetProcAddress) {
    FARPROC ret = NULL;
    if (node_exe) {
        ret = GetProcAddress(node_exe, info->dlp.szProcName);
        if (ret)
            return ret;
    }
    if (node_dll) {
        ret = GetProcAddress(node_dll, info->dlp.szProcName);
        if (ret)
            return ret;
    }
    ret = GetProcAddress(nw_dll, info->dlp.szProcName);
    return ret;
  }
  if (event == dliStartProcessing) {
    if (!inited) {
      node_exe = GetModuleHandle("node.exe");
      node_dll = GetModuleHandle("node.dll");
      nw_dll = GetModuleHandle("nw.dll");
      inited = 1;
    }
    return NULL;
  }
  if (event != dliNotePreLoadLibrary)
    return NULL;

  if (_stricmp(info->szDll, "nw.exe") != 0 &&
      _stricmp(info->szDll, "node.exe") != 0)
    return NULL;

  return (FARPROC) node_dll;
}

decltype(__pfnDliNotifyHook2) __pfnDliNotifyHook2 = load_exe_hook;

#if 0
BOOL WINAPI DllMain(HINSTANCE blah, DWORD dwReason, LPVOID reserved ) {
    DebugBreak();
    return TRUE;
}
#endif

#endif