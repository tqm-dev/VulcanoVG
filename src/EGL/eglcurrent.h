
#ifndef EGLCURRENT_INCLUDED
#define EGLCURRENT_INCLUDED

#include "c99_compat.h"

#include "egltypedefs.h"


#ifdef __cplusplus
extern "C" {
#endif

#define _EGL_API_ALL_BITS \
   (EGL_OPENGL_ES_BIT   | \
    EGL_OPENVG_BIT      | \
    EGL_OPENGL_ES2_BIT  | \
    EGL_OPENGL_ES3_BIT_KHR | \
    EGL_OPENGL_BIT)


/**
 * Per-thread info
 */
struct _egl_thread_info
{
   EGLint LastError;
   _EGLContext *CurrentContext;
   EGLenum CurrentAPI;
   EGLLabelKHR Label;

   /**
    * The name of the EGL function that's being called at the moment. This is
    * used to report the function name to the EGL_KHR_debug callback.
    */
   const char *CurrentFuncName;
   EGLLabelKHR CurrentObjectLabel;
};


/**
 * Return true if a client API enum is recognized.
 */
static inline EGLBoolean
_eglIsApiValid(EGLenum api)
{
   return api == EGL_OPENVG_API;
}


extern _EGLThreadInfo *
_eglGetCurrentThread(void);


extern void
_eglDestroyCurrentThread(void);


extern EGLBoolean
_eglIsCurrentThreadDummy(void);


extern _EGLContext *
_eglGetCurrentContext(void);


extern EGLBoolean
_eglError(EGLint errCode, const char *msg);

extern void
_eglDebugReport(EGLenum error, const char *funcName,
      EGLint type, const char *message, ...);

#define _eglReportCritical(error, funcName, ...) \
    _eglDebugReport(error, funcName, EGL_DEBUG_MSG_CRITICAL_KHR, __VA_ARGS__)

#define _eglReportError(error, funcName, ...) \
    _eglDebugReport(error, funcName, EGL_DEBUG_MSG_ERROR_KHR, __VA_ARGS__)

#define _eglReportWarn(funcName, ...) \
    _eglDebugReport(EGL_SUCCESS, funcName, EGL_DEBUG_MSG_WARN_KHR, __VA_ARGS__)

#define _eglReportInfo(funcName, ...) \
    _eglDebugReport(EGL_SUCCESS, funcName, EGL_DEBUG_MSG_INFO_KHR, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* EGLCURRENT_INCLUDED */
