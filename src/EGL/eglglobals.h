
#ifndef EGLGLOBALS_INCLUDED
#define EGLGLOBALS_INCLUDED

#include <stdbool.h>
#include "c11/threads.h"

#include "egltypedefs.h"

enum
{
    _EGL_DEBUG_BIT_CRITICAL = 0x1,
    _EGL_DEBUG_BIT_ERROR = 0x2,
    _EGL_DEBUG_BIT_WARN = 0x4,
    _EGL_DEBUG_BIT_INFO = 0x8,
};

/**
 * Global library data
 */
struct _egl_global
{
   mtx_t *Mutex;

   /* the list of all displays */
   _EGLDisplay *DisplayList;

   _EGLDevice *DeviceList;

   EGLint NumAtExitCalls;
   void (*AtExitCalls[10])(void);

   /*
    * Under libglvnd, the client extension string has to be split into two
    * strings, one for platform extensions, and one for everything else.
    * For a non-glvnd build create a concatenated one.
    */
#if USE_LIBGLVND
   const char *ClientOnlyExtensionString;
   const char *PlatformExtensionString;
#else
   const char *ClientExtensionString;
#endif

   EGLDEBUGPROCKHR debugCallback;
   unsigned int debugTypesEnabled;
};


extern struct _egl_global _eglGlobal;


extern void
_eglAddAtExitCall(void (*func)(void));

static inline unsigned int DebugBitFromType(EGLenum type)
{
   assert(type >= EGL_DEBUG_MSG_CRITICAL_KHR && type <= EGL_DEBUG_MSG_INFO_KHR);
   return (1 << (type - EGL_DEBUG_MSG_CRITICAL_KHR));
}

/**
 * Perform validity checks on a generic pointer.
 */
extern EGLBoolean
_eglPointerIsDereferencable(void *p);

#endif /* EGLGLOBALS_INCLUDED */
