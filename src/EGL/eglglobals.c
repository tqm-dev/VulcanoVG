
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "c11/threads.h"

#include "eglglobals.h"
#include "egldevice.h"
#include "egldisplay.h"
#include "egldriver.h"

#include "util/macros.h"

static mtx_t _eglGlobalMutex = _MTX_INITIALIZER_NP;

struct _egl_global _eglGlobal =
{
   .Mutex = &_eglGlobalMutex,
   .DisplayList = NULL,
   .DeviceList = &_eglSoftwareDevice,
   .NumAtExitCalls = 2,
   .AtExitCalls = {
      /* default AtExitCalls, called in reverse order */
      _eglFiniDevice, /* always called last */
      _eglFiniDisplay,
   },

#if USE_LIBGLVND
   .ClientOnlyExtensionString =
#else
   .ClientExtensionString =
#endif
   "EGL_EXT_client_extensions"
   " EGL_EXT_device_base"
   " EGL_EXT_device_enumeration"
   " EGL_EXT_device_query"
   " EGL_EXT_platform_base"
   " EGL_KHR_client_get_all_proc_addresses"
   " EGL_KHR_debug"

#if USE_LIBGLVND
   ,
   .PlatformExtensionString =
#else
   " "
#endif

   "EGL_EXT_platform_device"
   " EGL_MESA_platform_surfaceless"
   "",

   .debugCallback = NULL,
   .debugTypesEnabled = _EGL_DEBUG_BIT_CRITICAL | _EGL_DEBUG_BIT_ERROR,
};


static void
_eglAtExit(void)
{
   EGLint i;
   for (i = _eglGlobal.NumAtExitCalls - 1; i >= 0; i--)
      _eglGlobal.AtExitCalls[i]();
}


void
_eglAddAtExitCall(void (*func)(void))
{
   if (func) {
      static EGLBoolean registered = EGL_FALSE;

      mtx_lock(_eglGlobal.Mutex);

      if (!registered) {
         atexit(_eglAtExit);
         registered = EGL_TRUE;
      }

      assert(_eglGlobal.NumAtExitCalls < ARRAY_SIZE(_eglGlobal.AtExitCalls));
      _eglGlobal.AtExitCalls[_eglGlobal.NumAtExitCalls++] = func;

      mtx_unlock(_eglGlobal.Mutex);
   }
}

EGLBoolean
_eglPointerIsDereferencable(void *p)
{
   uintptr_t addr = (uintptr_t) p;
   const long page_size = getpagesize();
   return addr >= page_size;
}
