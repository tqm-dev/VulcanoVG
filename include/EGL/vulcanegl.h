
#ifndef _VULCANEGL_H
#define _VULCANEGL_H

#include <EGL/egl.h>
#include <EGL/eglplatform.h>

#ifdef __cplusplus 
extern "C" { 
#endif

/* window */
EGLNativeWindowType vcNativeCreateWindow(uint32_t, uint32_t);
EGLBoolean vcNativeDestroyWindow(EGLNativeWindowType);

#ifdef __cplusplus 
} /* extern "C" */
#endif

#endif /* _VULCANEGL_H */
