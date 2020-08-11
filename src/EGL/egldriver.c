
/**
 * Functions for choosing and opening/loading device drivers.
 */


#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "c11/threads.h"

#include "egldefines.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "egllog.h"

extern _EGLDriver _eglDriver;

/**
 * Initialize the display using the driver's function.
 * If the initialisation fails, try again using only software rendering.
 */
bool
_eglInitializeDisplay(_EGLDisplay *disp)
{
   assert(!disp->Initialized);

   /* set options */
   disp->Options.ForceSoftware = false;

   if (_eglDriver.Initialize(&_eglDriver, disp)) {
      disp->Driver = &_eglDriver;
      disp->Initialized = EGL_TRUE;
      return true;
   }

   if (disp->Options.ForceSoftware)
      return false;

   disp->Options.ForceSoftware = EGL_TRUE;
   if (!_eglDriver.Initialize(&_eglDriver, disp))
      return false;

   disp->Driver = &_eglDriver;
   disp->Initialized = EGL_TRUE;
   return true;
}

__eglMustCastToProperFunctionPointerType
_eglGetDriverProc(const char *procname)
{
   if (_eglDriver.GetProcAddress)
      return _eglDriver.GetProcAddress(&_eglDriver, procname);

   return NULL;
}
