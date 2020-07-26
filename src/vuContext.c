
#include "egl.h"
#include "vuContext.h"
#include <string.h>
#include <stdio.h>

EGLAPI EGLBoolean EGLAPIENTRY eglInitialize (EGLDisplay dpy, EGLint *major, EGLint *minor)
{
	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglMakeCurrent (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglQueryContext (EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
{
	return EGL_TRUE;
}
EGLAPI const char *EGLAPIENTRY eglQueryString (EGLDisplay dpy, EGLint name)
{
	return NULL;
}

EGLAPI EGLBoolean EGLAPIENTRY eglQuerySurface (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglSwapBuffers (EGLDisplay dpy, EGLSurface surface)
{
	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglTerminate (EGLDisplay dpy)
{
	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglWaitGL (void)
{
	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglWaitNative (EGLint engine)
{
	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglChooseConfig (EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglCopyBuffers (EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target)
{
	return EGL_TRUE;
}

EGLAPI EGLContext EGLAPIENTRY eglCreateContext (EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list)
{
	return EGL_NO_CONTEXT;
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePbufferSurface (EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
	return EGL_NO_SURFACE;
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePixmapSurface (EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list)
{
	return EGL_NO_SURFACE;
}

EGLAPI EGLSurface EGLAPIENTRY eglCreateWindowSurface (EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list)
{
	return EGL_NO_SURFACE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroyContext (EGLDisplay dpy, EGLContext ctx)
{
	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroySurface (EGLDisplay dpy, EGLSurface surface)
{
	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigAttrib (EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigs (EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
	return EGL_TRUE;
}

EGLAPI EGLDisplay EGLAPIENTRY eglGetCurrentDisplay (void)
{
	return EGL_NO_DISPLAY;
}

EGLAPI EGLSurface EGLAPIENTRY eglGetCurrentSurface (EGLint readdraw)
{
	return EGL_NO_SURFACE;
}

EGLAPI EGLDisplay EGLAPIENTRY eglGetDisplay (EGLNativeDisplayType display_id)
{
	return EGL_NO_DISPLAY;
}

EGLAPI EGLint EGLAPIENTRY eglGetError (void)
{
	return EGL_NONE;
}

