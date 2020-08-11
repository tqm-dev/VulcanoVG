
#ifndef EGLCONTEXT_INCLUDED
#define EGLCONTEXT_INCLUDED

#include "c99_compat.h"

#include "egltypedefs.h"
#include "egldisplay.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * "Base" class for device driver contexts.
 */
struct _egl_context
{
   /* A context is a display resource */
   _EGLResource Resource;

   /* The bound status of the context */
   _EGLThreadInfo *Binding;
   _EGLSurface *DrawSurface;
   _EGLSurface *ReadSurface;

   _EGLConfig *Config;

   EGLint ClientAPI; /**< EGL_OPENGL_ES_API, EGL_OPENGL_API, EGL_OPENVG_API */
   EGLint ClientMajorVersion;
   EGLint ClientMinorVersion;
   EGLint Flags;
   EGLint Profile;
   EGLint ResetNotificationStrategy;
   EGLint ContextPriority;
   EGLBoolean NoError;
   EGLint ReleaseBehavior;
};


extern EGLBoolean
_eglInitContext(_EGLContext *ctx, _EGLDisplay *disp,
                _EGLConfig *config, const EGLint *attrib_list);


extern EGLBoolean
_eglQueryContext(_EGLDriver *drv, _EGLDisplay *disp, _EGLContext *ctx, EGLint attribute, EGLint *value);


extern EGLBoolean
_eglBindContext(_EGLContext *ctx, _EGLSurface *draw, _EGLSurface *read,
                _EGLContext **old_ctx,
                _EGLSurface **old_draw, _EGLSurface **old_read);

extern _EGLContext *
_eglBindContextToThread(_EGLContext *ctx, _EGLThreadInfo *t);


/**
 * Increment reference count for the context.
 */
static inline _EGLContext *
_eglGetContext(_EGLContext *ctx)
{
   if (ctx)
      _eglGetResource(&ctx->Resource);
   return ctx;
}


/**
 * Decrement reference count for the context.
 */
static inline EGLBoolean
_eglPutContext(_EGLContext *ctx)
{
   return (ctx) ? _eglPutResource(&ctx->Resource) : EGL_FALSE;
}


/**
 * Link a context to its display and return the handle of the link.
 * The handle can be passed to client directly.
 */
static inline EGLContext
_eglLinkContext(_EGLContext *ctx)
{
   _eglLinkResource(&ctx->Resource, _EGL_RESOURCE_CONTEXT);
   return (EGLContext) ctx;
}


/**
 * Unlink a linked context from its display.
 * Accessing an unlinked context should generate EGL_BAD_CONTEXT error.
 */
static inline void
_eglUnlinkContext(_EGLContext *ctx)
{
   _eglUnlinkResource(&ctx->Resource, _EGL_RESOURCE_CONTEXT);
}


/**
 * Lookup a handle to find the linked context.
 * Return NULL if the handle has no corresponding linked context.
 */
static inline _EGLContext *
_eglLookupContext(EGLContext context, _EGLDisplay *disp)
{
   _EGLContext *ctx = (_EGLContext *) context;
   if (!disp || !_eglCheckResource((void *) ctx, _EGL_RESOURCE_CONTEXT, disp))
      ctx = NULL;
   return ctx;
}


/**
 * Return the handle of a linked context, or EGL_NO_CONTEXT.
 */
static inline EGLContext
_eglGetContextHandle(_EGLContext *ctx)
{
   _EGLResource *res = (_EGLResource *) ctx;
   return (res && _eglIsResourceLinked(res)) ?
      (EGLContext) ctx : EGL_NO_CONTEXT;
}


#ifdef __cplusplus
}
#endif

#endif /* EGLCONTEXT_INCLUDED */
