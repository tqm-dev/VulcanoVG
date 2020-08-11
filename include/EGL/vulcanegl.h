
#ifndef _VULCANEGL_H
#define _VULCANEGL_H

#include <EGL/egl.h>
#include <EGL/eglplatform.h>

#ifdef __cplusplus 
extern "C" { 
#endif

#define VC_FORMAT_R8G8B8A8_SRGB		0x00000001

/* window */
EGLNativeWindowType vcNativeCreateWindow(uint32_t, uint32_t, uint8_t);
EGLBoolean vcNativeDestroyWindow(EGLNativeWindowType);

#ifdef __cplusplus 
} /* extern "C" */
#endif

#endif /* _VULCANEGL_H */
