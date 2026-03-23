//----------------------------------------------------------
// CALL32.C
//
// This creates a DLL for 16-bit Visual Basic programs to
// call 32-bit DLLs on Windows NT 3.1.  It uses the 
// Generic Thunks feature of the WOW subsystem on Windows
// NT to load and call 32 bit DLLs.  This file should
// be compile into a 16-bit DLL.
//
// Writted by Peter Golde.
//----------------------------------------------------------

#include <windows.h>
#include <windowsx.h>
#include "vbapi.h"

// Disable some warnings that won't go away.
#pragma warning(disable: 4704 4785)

// Error codes we return.
#define ERR_CANTLOADLIBRARY  30001
#define ERR_CANTFINDFUNCTION 30002
#define ERR_INVALIDPARMSTRING 30003 
#define ERR_NOTNT 30004 
#define ERR_INVALIDHWND 30005
#define ERR_OOM  7

// Test for the error messages.
char * szCantLoadLibrary = "Can't load DLL: \"%s\"\n(error=%d)";
char * szCantFindFunction = "Can't find specified function";
char * szInvalidParmString = "Invalid parameter definition string";
char * szNotNT = "Not running on Windows NT";
char * szInvalidHwnd = "Invalid window handle";

// This structure describes a function which has been 
// registered with us.
typedef struct {
  DWORD hinst;        // 32-bit instance handle of library
  LPVOID lpfunc;      // 32-bit function address of function
  DWORD dwAddrXlat;   // bit mask of params: 1 indicates arg is address
  DWORD dwHwndXlat;   // bit mask of params: 1 indicates arg is 16-bit hwnd
  DWORD nParams;      // number of parameters
} PROC32ENTRY;
  
// rgProc32Entry points to an array of PROC32ENTRY functions, which
// is grown as needed.  The value returned by Declare32 is an
// index into this array.
int cRegistered = 0;  // number of registered functions.
int cAlloc = 0;       // number of alloced PROC32ENTRY structures.
PROC32ENTRY FAR * rgProc32Entry = 0;  // array of PROC32ENTRY structures.
#define CALLOCGROW 10 // number of entries to grow rgProc32Entry by

// These are the addresses of the Generic Thunk functions in 
// the WOW KERNEL.  
BOOL fGotProcs = FALSE;    // Did we successfully get the addresses?
DWORD (FAR PASCAL *CallProc32W)() = 0;
BOOL (FAR PASCAL *FreeLibrary32W)(DWORD) = 0;
LPVOID (FAR PASCAL *GetProcAddress32W)(DWORD, LPCSTR) = 0;
DWORD (FAR PASCAL *LoadLibraryEx32W)(LPSTR, DWORD, DWORD) = 0;
LPVOID lpvGetLastError = 0;   // address of 32-bit GetLastError.

//-----------------------------------------------------
// XlatHwnd
//   Translates a 16-bit HWND into a 32-bit HWND.
//   The HWND must be one in our 16-bit process.
//   NULL is translated to NULL and doesn't cause
//   and error.
//
//   Unfortunately, WOW does not export a function
//   for doing this, so our procedure is as follows:
//   We do 16-bit SetCapture call to the window
//   to set the capture, and then a 32-bit GetCapture
//   call to get the 32-bit equivalent handle.  The
//   capture is then restored to what it was beforehand.
//
//   May cause VB runtime error, and hence never return.
//-----------------------------------------------------
static void PASCAL NEAR XlatHwnd
(
  DWORD FAR * phwnd   // Points to 16-bit HWND, on return
                      // points to 32-bit HWND.
)
{
  HWND hwnd16 = LOWORD(*phwnd);  // 16-bit hwnd
  HWND hwndCapturePrev;          // window that has the capture
  DWORD hwnd32;                  // 32-bit hwnd
  static LPVOID lpvGetCapture;   // Address of 32-bit GetCapture

  // Check for valid 16-bit handle.   
  if (*phwnd != (DWORD)(LONG)(SHORT)hwnd16) 
    goto BadHwnd;
  if (hwnd16 != NULL && !IsWindow(hwnd16))
    goto BadHwnd;
  
  // Get Address of 32-bit GetCapture
  if (! lpvGetCapture) {
    DWORD hinstUser = LoadLibraryEx32W("user32", 0, 0);
    if (hinstUser) {
      lpvGetCapture = GetProcAddress32W(hinstUser, "GetCapture");
      FreeLibrary32W(hinstUser);
    }
    if (!lpvGetCapture) {
      VBSetErrorMessage(ERR_NOTNT, szNotNT);
      VBRuntimeError(ERR_NOTNT);
    }
  }
  
  // Set capture to window, get capture to get 32-bit handle. 
  // Be sure to restore capture afterward.
  // NULL isn't translated
  if (hwnd16) {
    hwndCapturePrev = SetCapture(hwnd16);
    hwnd32 = ((DWORD (FAR PASCAL *)(LPVOID, DWORD, DWORD)) CallProc32W) (lpvGetCapture, (DWORD)0, (DWORD)0);
    if (hwndCapturePrev)
      SetCapture(hwndCapturePrev);
    else
      ReleaseCapture();
    if (!hwnd32) 
      goto BadHwnd;
  }    

  // Success - done.    
  *phwnd = hwnd32;
  return;  

BadHwnd:
  // Error: couldn't translate HWND or bad HWND passed.
  VBSetErrorMessage(ERR_INVALIDHWND, szInvalidHwnd);
  VBRuntimeError(ERR_INVALIDHWND);
}

  
//-----------------------------------------------------
// MungeArgs
//   Modify the args array so it can be passed to
//   to CallProc32W.  This uses the PROC32ENTRY structure
//   to set up the arg list correctly on the stack
//   so CallProc32W can be call.  HWND translation is
//   performed.  The frame is changed as follows:
//           In:                 Out:
//            unused              number of params
//   dwArgs-> unused              address xlat mask
//            PROC32ENTRY index   32-bit function address.
//            argument            argument, possible HWND xlated
//            argument            argument, possible HWND xlated
//            ...                 ...
//-----------------------------------------------------
void PASCAL NEAR _loadds MungeArgs
(
  LPDWORD dwArgs           // Points to arg list
)
{
  PROC32ENTRY FAR * pentry = & rgProc32Entry[dwArgs[1]];
  int iArg = 2;
  DWORD dwHwndXlat;
  
  dwArgs[-1] = pentry->nParams;
  dwArgs[0] = pentry->dwAddrXlat;
  dwArgs[1] = (DWORD) pentry->lpfunc;
  dwHwndXlat = pentry->dwHwndXlat;
  while (dwHwndXlat) {
    if (dwHwndXlat & 1) 
      XlatHwnd(& dwArgs[iArg]);
    ++iArg;
    dwHwndXlat >>= 1;
  }  
} 

//-----------------------------------------------------
// Call32
//   This function is called by VB applications directly.
//   Arguments to the function are also on the stack 
//   (iProc is the PROC32ENTRY index).  We correctly
//   set up the stack frame, then JUMP to CallProc32W,
//   which eventually returns to the user.
//-----------------------------------------------------
void _export PASCAL Call32(long iProc)
{
  __asm {
  
  pop     cx              // dx = callers DS
  pop     bp              // restore BP
  
  mov     bx, sp          // bx = sp on entry
  sub     sp, 8           // 2 additional words
  mov     ax, ss:[bx]     // ax = return address offst
  mov     dx, ss:[bx+2]   // dx = return address segment
  mov     ss:[bx-8], ax
  mov     ss:[bx-6], dx
  push    ds              // Save our DS
  push    ss
  push    bx              // Push pointer to args
  mov     ds, cx          // Restore caller's DS
  call    MungeArgs       // Munge the args
  pop     es              // es is our DS
  jmp    far ptr es:[CallProc32W] // Jump to the call thunker
  }
}  
  
//-----------------------------------------------------
// Declare32
//   This function is called directly from VB.
//   It allocates and fills in a PROC32ENTRY structure
//   so that we can call the 32 bit function.
//-----------------------------------------------------
long _export PASCAL Declare32
(
  LPSTR lpstrName,       // function name
  LPSTR lpstrLib,        // function library
  LPSTR lpstrArg         // string indicating arg types
)
{
  DWORD hinst;           // 32-bit DLL instance handle
  LPVOID lpfunc;         // 32-bit function pointer
  DWORD dwAddrXlat;      // address xlat mask
  DWORD dwHwndXlat;      // hwnd xlat mask
  DWORD nParams;         // number of params
  CHAR szBuffer[128];    // scratch buffer

  // First time called, get the addresses of the Generic Thunk
  // functions.  Raise VB runtime error if can't (probably because
  // we're not running on NT).  
  if (!fGotProcs) {
    HINSTANCE hinstKernel;    // Instance handle of WOW KERNEL.DLL
    DWORD hinstKernel32;      // Instance handle of Win32 KERNEL32.DLL
    
    hinstKernel = LoadLibrary("KERNEL");
    if (hinstKernel < HINSTANCE_ERROR) {
      VBSetErrorMessage(ERR_NOTNT, szNotNT);
      VBRuntimeError(ERR_NOTNT);
    }
    
    CallProc32W = (DWORD (FAR PASCAL *)()) GetProcAddress(hinstKernel, "CALLPROC32W");
    FreeLibrary32W = (BOOL (FAR PASCAL *)(DWORD)) GetProcAddress(hinstKernel, "FREELIBRARY32W");
    LoadLibraryEx32W =  (DWORD (FAR PASCAL *)(LPSTR, DWORD, DWORD))GetProcAddress(hinstKernel, "LOADLIBRARYEX32W");
    GetProcAddress32W = (LPVOID (FAR PASCAL *)(DWORD, LPCSTR))  GetProcAddress(hinstKernel, "GETPROCADDRESS32W");
    FreeLibrary(hinstKernel);

    if (LoadLibraryEx32W && GetProcAddress32W && FreeLibrary32W) {
      hinstKernel32 = LoadLibraryEx32W("kernel32", 0, 0);
      lpvGetLastError = GetProcAddress32W(hinstKernel32, "GetLastError");
      FreeLibrary32W(hinstKernel);
    }
    
    if (!CallProc32W || !FreeLibrary32W || !LoadLibraryEx32W || 
        !GetProcAddress32W || !lpvGetLastError) {
      VBSetErrorMessage(ERR_NOTNT, szNotNT);
      VBRuntimeError(ERR_NOTNT);
    }
    
    fGotProcs = TRUE;
  }  

  // If needed, allocate a PROC32ENTRY structure
  if (cRegistered == cAlloc) {
    if (rgProc32Entry) 
      rgProc32Entry = (PROC32ENTRY FAR *) GlobalReAllocPtr(rgProc32Entry, 
                                          (cAlloc + CALLOCGROW) * sizeof(PROC32ENTRY), GMEM_MOVEABLE);
    else                                          
      rgProc32Entry = (PROC32ENTRY FAR *) GlobalAllocPtr(GMEM_MOVEABLE, CALLOCGROW * sizeof(PROC32ENTRY));
    if (!rgProc32Entry) {
      VBRuntimeError(ERR_OOM);
    }
    cAlloc += CALLOCGROW;
  }  
  
  // Process the arg list descriptor string to 
  // get the hwnd and addr translation masks, and the
  // number of args.
  dwAddrXlat = dwHwndXlat = 0;
  if ((nParams = lstrlen(lpstrArg)) > 32) {
    VBSetErrorMessage(ERR_INVALIDPARMSTRING, szInvalidParmString);
    VBRuntimeError(ERR_INVALIDPARMSTRING);
  }
  while (*lpstrArg) {
    dwAddrXlat <<= 1;
    dwHwndXlat <<= 1;
    switch (*lpstrArg) {
    case 'p':
      dwAddrXlat |= 1;
      break;
    case 'i':
      break;
    case 'w':
      dwHwndXlat |= 1;
      break;
    default:
      VBSetErrorMessage(ERR_INVALIDPARMSTRING, szInvalidParmString);
      VBRuntimeError(ERR_INVALIDPARMSTRING);
    }
    ++lpstrArg;
  }
  
  // Load the 32-bit library.  
  hinst = LoadLibraryEx32W(lpstrLib, NULL, 0);
  if (!hinst) {
    DWORD errCode = 0;

    // Get NT error code (szBuffer is convenient scratch buffer)    
    errCode = ((DWORD (FAR PASCAL *)(LPVOID, DWORD, DWORD)) CallProc32W) (lpvGetLastError, (DWORD)0, (DWORD)0);
    wsprintf(szBuffer, szCantLoadLibrary, lpstrLib, errCode);
    VBSetErrorMessage(ERR_CANTLOADLIBRARY, szBuffer);
    VBRuntimeError(ERR_CANTLOADLIBRARY);
  }
  
  // Get the 32-bit function address.  Try the following three
  // variations of the name (example: NAME):
  //    NAME
  //    _NAME@nn     (stdcall naming convention: nn is bytes of args)
  //    NAMEA        (Win32 ANSI function naming convention)
  lpfunc = GetProcAddress32W(hinst, lpstrName); 
  if (!lpfunc && lstrlen(lpstrName) < 122) {
    // Change to stdcall naming convention.
    wsprintf(szBuffer, "_%s@%d", lpstrName, nParams * 4);
    lpfunc = GetProcAddress32W(hinst, szBuffer);
  }  
  if (!lpfunc && lstrlen(lpstrName) < 126) {
    // Add suffix "A" for ansi
    lstrcpy(szBuffer, lpstrName);
    lstrcat(szBuffer, "A");
    lpfunc = GetProcAddress32W(hinst, szBuffer);
  }  
  if (!lpfunc) {
    FreeLibrary32W(hinst);
    VBSetErrorMessage(ERR_CANTFINDFUNCTION, szCantFindFunction);
    VBRuntimeError(ERR_CANTFINDFUNCTION);
  }
  
  // Fill in PROC32ENTRY struct and return index.     
  rgProc32Entry[cRegistered].hinst = hinst;
  rgProc32Entry[cRegistered].lpfunc = lpfunc;
  rgProc32Entry[cRegistered].dwAddrXlat = dwAddrXlat;
  rgProc32Entry[cRegistered].dwHwndXlat = dwHwndXlat;
  rgProc32Entry[cRegistered].nParams = nParams;
  return cRegistered++;
}

//-----------------------------------------------------
// WEP
//   Called when DLL is unloaded.  We free all the
//   32-bit DLLs we were using and clear the
//   PROC32ENTRY list.
//-----------------------------------------------------
int FAR PASCAL _export WEP(int nExitType)
{
  while (--cRegistered >= 0) 
    FreeLibrary32W(rgProc32Entry[cRegistered].hinst);
  if (rgProc32Entry)  
    GlobalFreePtr(rgProc32Entry);
  rgProc32Entry = NULL;
  cRegistered = cAlloc = 0;
  return 1;
}



