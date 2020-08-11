
#ifndef EGLSYNC_INCLUDED
#define EGLSYNC_INCLUDED


#include "c99_compat.h"

#include "egltypedefs.h"
#include "egldisplay.h"


/**
 * "Base" class for device driver syncs.
 */
struct _egl_sync
{
   /* A sync is a display resource */
   _EGLResource Resource;

   EGLenum Type;
   EGLenum SyncStatus;
   EGLenum SyncCondition;
   EGLAttrib CLEvent;
   EGLint SyncFd;
};


extern EGLBoolean
_eglInitSync(_EGLSync *sync, _EGLDisplay *disp, EGLenum type,
             const EGLAttrib *attrib_list);


extern EGLBoolean
_eglGetSyncAttrib(_EGLDriver *drv, _EGLDisplay *disp, _EGLSync *sync,
                  EGLint attribute, EGLAttrib *value);


/**
 * Increment reference count for the sync.
 */
static inline _EGLSync *
_eglGetSync(_EGLSync *sync)
{
   if (sync)
      _eglGetResource(&sync->Resource);
   return sync;
}


/**
 * Decrement reference count for the sync.
 */
static inline EGLBoolean
_eglPutSync(_EGLSync *sync)
{
   return (sync) ? _eglPutResource(&sync->Resource) : EGL_FALSE;
}


/**
 * Link a sync to its display and return the handle of the link.
 * The handle can be passed to client directly.
 */
static inline EGLSync
_eglLinkSync(_EGLSync *sync)
{
   _eglLinkResource(&sync->Resource, _EGL_RESOURCE_SYNC);
   return (EGLSync) sync;
}


/**
 * Unlink a linked sync from its display.
 */
static inline void
_eglUnlinkSync(_EGLSync *sync)
{
   _eglUnlinkResource(&sync->Resource, _EGL_RESOURCE_SYNC);
}


/**
 * Lookup a handle to find the linked sync.
 * Return NULL if the handle has no corresponding linked sync.
 */
static inline _EGLSync *
_eglLookupSync(EGLSync handle, _EGLDisplay *disp)
{
   _EGLSync *sync = (_EGLSync *) handle;
   if (!disp || !_eglCheckResource((void *) sync, _EGL_RESOURCE_SYNC, disp))
      sync = NULL;
   return sync;
}


/**
 * Return the handle of a linked sync, or EGL_NO_SYNC_KHR.
 */
static inline EGLSync
_eglGetSyncHandle(_EGLSync *sync)
{
   _EGLResource *res = (_EGLResource *) sync;
   return (res && _eglIsResourceLinked(res)) ?
      (EGLSync) sync : EGL_NO_SYNC_KHR;
}


#endif /* EGLSYNC_INCLUDED */
