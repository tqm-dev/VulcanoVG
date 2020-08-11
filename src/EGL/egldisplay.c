
/**
 * Functions related to EGLDisplay.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "c11/threads.h"
#include "util/macros.h"

#include "eglcontext.h"
#include "eglcurrent.h"
#include "eglsurface.h"
#include "egldevice.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "egllog.h"
#include "eglimage.h"
#include "eglsync.h"


/**
 * Map build-system platform names to platform types.
 */
static const struct {
   _EGLPlatformType platform;
   const char *name;
} egl_platforms[] = {
   { _EGL_PLATFORM_VULKAN, "vulkan" },
   { _EGL_PLATFORM_VULKAN_SURFACELESS, "vulkan surfaceless" },
};


/**
 * Return the native platform by parsing EGL_PLATFORM.
 */
static _EGLPlatformType
_eglGetNativePlatformFromEnv(void)
{
   _EGLPlatformType plat = _EGL_INVALID_PLATFORM;
   const char *plat_name;
   EGLint i;

   STATIC_ASSERT(ARRAY_SIZE(egl_platforms) == _EGL_NUM_PLATFORMS);

   plat_name = getenv("EGL_PLATFORM");
   /* try deprecated env variable */
   if (!plat_name || !plat_name[0])
      plat_name = getenv("EGL_DISPLAY");
   if (!plat_name || !plat_name[0])
      return _EGL_INVALID_PLATFORM;

   for (i = 0; i < ARRAY_SIZE(egl_platforms); i++) {
      if (strcmp(egl_platforms[i].name, plat_name) == 0) {
         plat = egl_platforms[i].platform;
         break;
      }
   }

   if (plat == _EGL_INVALID_PLATFORM)
      _eglLog(_EGL_WARNING, "invalid EGL_PLATFORM given");

   return plat;
}


/**
 * Try detecting native platform with the help of native display characteristcs.
 */
static _EGLPlatformType
_eglNativePlatformDetectNativeDisplay(void *nativeDisplay)
{
   if (nativeDisplay == EGL_DEFAULT_DISPLAY)
      // Support for rendering to window system via Vulkan WSI
      return _EGL_PLATFORM_VULKAN;

   // Only support for off-screen rendering to VkImage
   return _EGL_PLATFORM_VULKAN_SURFACELESS;
}


/**
 * Return the native platform.  It is the platform of the EGL native types.
 */
_EGLPlatformType
_eglGetNativePlatform(void *nativeDisplay)
{
   _EGLPlatformType detected_platform = _eglGetNativePlatformFromEnv();
   const char *detection_method = "environment";

   if (detected_platform == _EGL_INVALID_PLATFORM) {
      detected_platform = _eglNativePlatformDetectNativeDisplay(nativeDisplay);
      detection_method = "autodetected";
   }

   if (detected_platform == _EGL_INVALID_PLATFORM) {
      return detected_platform;
   }

   _eglLog(_EGL_DEBUG, "Native platform type: %s (%s)",
           egl_platforms[detected_platform].name, detection_method);

   return detected_platform;
}


/**
 * Finish display management.
 */
void
_eglFiniDisplay(void)
{
   _EGLDisplay *dispList, *disp;

   /* atexit function is called with global mutex locked */
   dispList = _eglGlobal.DisplayList;
   while (dispList) {
      EGLint i;

      /* pop list head */
      disp = dispList;
      dispList = dispList->Next;

      for (i = 0; i < _EGL_NUM_RESOURCES; i++) {
         if (disp->ResourceLists[i]) {
            _eglLog(_EGL_DEBUG, "Display %p is destroyed with resources", disp);
            break;
         }
      }


      /* The fcntl() code in _eglGetDeviceDisplay() ensures that valid fd >= 3,
       * and invalid one is 0.
       */
      if (disp->Options.fd)
         close(disp->Options.fd);

      free(disp->Options.Attribs);
      free(disp);
   }
   _eglGlobal.DisplayList = NULL;
}

static EGLBoolean
_eglSameAttribs(const EGLAttrib *a, const EGLAttrib *b)
{
   size_t na = _eglNumAttribs(a);
   size_t nb = _eglNumAttribs(b);

   /* different numbers of attributes must be different */
   if (na != nb)
      return EGL_FALSE;

   /* both lists NULL are the same */
   if (!a && !b)
      return EGL_TRUE;

   /* otherwise, compare the lists */
   return memcmp(a, b, na * sizeof(a[0])) == 0 ? EGL_TRUE : EGL_FALSE;
}

/**
 * Find the display corresponding to the specified native display, or create a
 * new one. EGL 1.5 says:
 *
 *     Multiple calls made to eglGetPlatformDisplay with the same parameters
 *     will return the same EGLDisplay handle.
 *
 * We read this extremely strictly, and treat a call with NULL attribs as
 * different from a call with attribs only equal to { EGL_NONE }. Similarly
 * we do not sort the attribute list, so even if all attribute _values_ are
 * identical, different attribute orders will be considered different
 * parameters.
 */
_EGLDisplay *
_eglFindDisplay(_EGLPlatformType plat, void *plat_dpy,
                const EGLAttrib *attrib_list)
{
   _EGLDisplay *disp;
   size_t num_attribs;

   if (plat == _EGL_INVALID_PLATFORM)
      return NULL;

   mtx_lock(_eglGlobal.Mutex);

   /* search the display list first */
   for (disp = _eglGlobal.DisplayList; disp; disp = disp->Next) {
      if (disp->Platform == plat && disp->PlatformDisplay == plat_dpy &&
          _eglSameAttribs(disp->Options.Attribs, attrib_list))
         break;
   }

   /* create a new display */
   if (!disp) {
      disp = calloc(1, sizeof(_EGLDisplay));
      if (disp) {
         mtx_init(&disp->Mutex, mtx_plain);
         disp->Platform = plat;
         disp->PlatformDisplay = plat_dpy;
         num_attribs = _eglNumAttribs(attrib_list);
         if (num_attribs) {
            disp->Options.Attribs = calloc(num_attribs, sizeof(EGLAttrib));
            if (!disp->Options.Attribs) {
               free(disp);
               disp = NULL;
               goto out;
            }
            memcpy(disp->Options.Attribs, attrib_list,
                   num_attribs * sizeof(EGLAttrib));
         }
         /* add to the display list */
         disp->Next = _eglGlobal.DisplayList;
         _eglGlobal.DisplayList = disp;
      }
   }

out:
   mtx_unlock(_eglGlobal.Mutex);

   return disp;
}


/**
 * Destroy the contexts and surfaces that are linked to the display.
 */
void
_eglReleaseDisplayResources(_EGLDriver *drv, _EGLDisplay *display)
{
   _EGLResource *list;

   list = display->ResourceLists[_EGL_RESOURCE_CONTEXT];
   while (list) {
      _EGLContext *ctx = (_EGLContext *) list;
      list = list->Next;

      _eglUnlinkContext(ctx);
      drv->DestroyContext(drv, display, ctx);
   }
   assert(!display->ResourceLists[_EGL_RESOURCE_CONTEXT]);

   list = display->ResourceLists[_EGL_RESOURCE_SURFACE];
   while (list) {
      _EGLSurface *surf = (_EGLSurface *) list;
      list = list->Next;

      _eglUnlinkSurface(surf);
      drv->DestroySurface(drv, display, surf);
   }
   assert(!display->ResourceLists[_EGL_RESOURCE_SURFACE]);

   list = display->ResourceLists[_EGL_RESOURCE_IMAGE];
   while (list) {
      _EGLImage *image = (_EGLImage *) list;
      list = list->Next;

      _eglUnlinkImage(image);
      drv->DestroyImageKHR(drv, display, image);
   }
   assert(!display->ResourceLists[_EGL_RESOURCE_IMAGE]);

   list = display->ResourceLists[_EGL_RESOURCE_SYNC];
   while (list) {
      _EGLSync *sync = (_EGLSync *) list;
      list = list->Next;

      _eglUnlinkSync(sync);
      drv->DestroySyncKHR(drv, display, sync);
   }
   assert(!display->ResourceLists[_EGL_RESOURCE_SYNC]);
}


/**
 * Free all the data hanging of an _EGLDisplay object, but not
 * the object itself.
 */
void
_eglCleanupDisplay(_EGLDisplay *disp)
{
   if (disp->Configs) {
      _eglDestroyArray(disp->Configs, free);
      disp->Configs = NULL;
   }

   /* XXX incomplete */
}


/**
 * Return EGL_TRUE if the given handle is a valid handle to a display.
 */
EGLBoolean
_eglCheckDisplayHandle(EGLDisplay dpy)
{
   _EGLDisplay *cur;

   mtx_lock(_eglGlobal.Mutex);
   cur = _eglGlobal.DisplayList;
   while (cur) {
      if (cur == (_EGLDisplay *) dpy)
         break;
      cur = cur->Next;
   }
   mtx_unlock(_eglGlobal.Mutex);
   return (cur != NULL);
}


/**
 * Return EGL_TRUE if the given resource is valid.  That is, the display does
 * own the resource.
 */
EGLBoolean
_eglCheckResource(void *res, _EGLResourceType type, _EGLDisplay *disp)
{
   _EGLResource *list = disp->ResourceLists[type];
   
   if (!res)
      return EGL_FALSE;

   while (list) {
      if (res == (void *) list) {
         assert(list->Display == disp);
         break;
      }
      list = list->Next;
   }

   return (list != NULL);
}


/**
 * Initialize a display resource.  The size of the subclass object is
 * specified.
 *
 * This is supposed to be called from the initializers of subclasses, such as
 * _eglInitContext or _eglInitSurface.
 */
void
_eglInitResource(_EGLResource *res, EGLint size, _EGLDisplay *disp)
{
   memset(res, 0, size);
   res->Display = disp;
   res->RefCount = 1;
}


/**
 * Increment reference count for the resource.
 */
void
_eglGetResource(_EGLResource *res)
{
   assert(res && res->RefCount > 0);
   /* hopefully a resource is always manipulated with its display locked */
   res->RefCount++;
}


/**
 * Decrement reference count for the resource.
 */
EGLBoolean
_eglPutResource(_EGLResource *res)
{
   assert(res && res->RefCount > 0);
   res->RefCount--;
   return (!res->RefCount);
}


/**
 * Link a resource to its display.
 */
void
_eglLinkResource(_EGLResource *res, _EGLResourceType type)
{
   assert(res->Display);

   res->IsLinked = EGL_TRUE;
   res->Next = res->Display->ResourceLists[type];
   res->Display->ResourceLists[type] = res;
   _eglGetResource(res);
}


/**
 * Unlink a linked resource from its display.
 */
void
_eglUnlinkResource(_EGLResource *res, _EGLResourceType type)
{
   _EGLResource *prev;

   prev = res->Display->ResourceLists[type];
   if (prev != res) {
      while (prev) {
         if (prev->Next == res)
            break;
         prev = prev->Next;
      }
      assert(prev);
      prev->Next = res->Next;
   }
   else {
      res->Display->ResourceLists[type] = res->Next;
   }

   res->Next = NULL;
   res->IsLinked = EGL_FALSE;
   _eglPutResource(res);

   /* We always unlink before destroy.  The driver still owns a reference */
   assert(res->RefCount);
}

_EGLDisplay*
_eglGetVulkanDisplay(void *native_display,
                          const EGLAttrib *attrib_list)
{
   /* This platform has no native display. */
   if (native_display != NULL) {
      _eglError(EGL_BAD_PARAMETER, "eglGetPlatformDisplay");
      return NULL;
   }

   /* This platform recognizes no display attributes. */
   if (attrib_list != NULL && attrib_list[0] != EGL_NONE) {
      _eglError(EGL_BAD_ATTRIBUTE, "eglGetPlatformDisplay");
      return NULL;
   }

   return _eglFindDisplay(_EGL_PLATFORM_VULKAN, native_display,
                          attrib_list);
}

_EGLDisplay*
_eglGetVulkanSurfacelessDisplay(void *native_display,
                          const EGLAttrib *attrib_list)
{
   /* This platform has no native display. */
   if (native_display != NULL) {
      _eglError(EGL_BAD_PARAMETER, "eglGetPlatformDisplay");
      return NULL;
   }

   /* This platform recognizes no display attributes. */
   if (attrib_list != NULL && attrib_list[0] != EGL_NONE) {
      _eglError(EGL_BAD_ATTRIBUTE, "eglGetPlatformDisplay");
      return NULL;
   }

   return _eglFindDisplay(_EGL_PLATFORM_VULKAN_SURFACELESS, native_display,
                          attrib_list);
}

