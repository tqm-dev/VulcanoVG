
/**
 * Internal EGL defines
 */


#ifndef EGLDEFINES_INCLUDED
#define EGLDEFINES_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define _EGL_MAX_EXTENSIONS_LEN 1000

/* Hardcoded, conservative default for EGL_LARGEST_PBUFFER,
 * this is used to implement EGL_LARGEST_PBUFFER.
 */
#define _EGL_MAX_PBUFFER_WIDTH 4096
#define _EGL_MAX_PBUFFER_HEIGHT 4096

#define _EGL_VENDOR_STRING "Mesa Project"

#ifdef __cplusplus
}
#endif

#endif /* EGLDEFINES_INCLUDED */
