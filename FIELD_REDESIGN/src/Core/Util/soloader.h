// soloader.h written by Chris Moulding 11/98
// 
// these functions are used to abstract the interface for 
// accessing shared libraries (.so for unix and .dll for windows)
//

#include <SCICore/share/share.h>

#ifdef _WIN32
#include <afxwin.h> // for LoadLibrary(), GetProcAddress() and HINSTANCE
typedef HINSTANCE LIBRARY_HANDLE;
#else
#include <dlfcn.h>   // for dlopen() and dlsym()
typedef void* LIBRARY_HANDLE;
#endif

/////////////////////////////////////////////////////////////////
//
// GetLibrarySymbolAddress()
//
// returns a pointer to the data or function called "symbolname"
// from within the shared library called "libname"
//

SCICORESHARE void* GetLibrarySymbolAddress(const char* libname, const char* symbolname);


/////////////////////////////////////////////////////////////////
//
// GetLibraryHandle()
//
// opens, and returns the handle to, the library module
// called "libname"
//

SCICORESHARE LIBRARY_HANDLE GetLibraryHandle(const char* libname);


/////////////////////////////////////////////////////////////////
//
// GetHandleSymbolAddress()
//
// returns a pointer to the data or function called "symbolname"
// from within the shared library with handle "handle"
//

SCICORESHARE void* GetHandleSymbolAddress(LIBRARY_HANDLE handle, const char* symbolname);


/////////////////////////////////////////////////////////////////
//
// CloseLibraries()
//
// disassociates all libraries opened, by the calling process, using
// the GetLibrarySymbolAddress() or GetLibraryHandle()
// functions.  Call this function to clean up stuff when shutting the
// application down.
//

SCICORESHARE void CloseLibraries();
