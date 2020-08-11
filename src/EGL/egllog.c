
/**
 * Logging facility for debug/info messages.
 * _EGL_FATAL messages are printed to stderr
 * The EGL_LOG_LEVEL var controls the output of other warning/info/debug msgs.
 */


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "c11/threads.h"
#include "util/macros.h"

#include "egllog.h"

#define MAXSTRING 1000
#define FALLBACK_LOG_LEVEL _EGL_WARNING


static struct {
   mtx_t mutex;

   EGLBoolean initialized;
   EGLint level;
} logging = {
   .mutex = _MTX_INITIALIZER_NP,
   .initialized = EGL_FALSE,
   .level = FALLBACK_LOG_LEVEL,
};

static const char *level_strings[] = {
   [_EGL_FATAL] = "fatal",
   [_EGL_WARNING]  = "warning",
   [_EGL_INFO] = "info",
   [_EGL_DEBUG] = "debug",
};


/**
 * The default logger.  It prints the message to stderr.
 */
static void
_eglDefaultLogger(EGLint level, const char *msg)
{
   fprintf(stderr, "libEGL %s: %s\n", level_strings[level], msg);
}


/**
 * Initialize the logging facility.
 */
static void
_eglInitLogger(void)
{
   const char *log_env;
   EGLint i, level = -1;

   if (logging.initialized)
      return;

   log_env = getenv("EGL_LOG_LEVEL");
   if (log_env) {
      for (i = 0; i < ARRAY_SIZE(level_strings); i++) {
         if (strcasecmp(log_env, level_strings[i]) == 0) {
            level = i;
            break;
         }
      }
   }

   logging.level = (level >= 0) ? level : FALLBACK_LOG_LEVEL;
   logging.initialized = EGL_TRUE;

   /* it is fine to call _eglLog now */
   if (log_env && level < 0) {
      _eglLog(_EGL_WARNING,
              "Unrecognized EGL_LOG_LEVEL environment variable value. "
              "Expected one of \"fatal\", \"warning\", \"info\", \"debug\". "
              "Got \"%s\". Falling back to \"%s\".",
              log_env, level_strings[FALLBACK_LOG_LEVEL]);
   }
}


/**
 * Log a message with message logger.
 * \param level one of _EGL_FATAL, _EGL_WARNING, _EGL_INFO, _EGL_DEBUG.
 */
void
_eglLog(EGLint level, const char *fmtStr, ...)
{
   va_list args;
   char msg[MAXSTRING];
   int ret;

   /* one-time initialization; a little race here is fine */
   if (!logging.initialized)
      _eglInitLogger();
   if (level > logging.level || level < 0)
      return;

   mtx_lock(&logging.mutex);

   va_start(args, fmtStr);
   ret = vsnprintf(msg, MAXSTRING, fmtStr, args);
   if (ret < 0 || ret >= MAXSTRING)
      strcpy(msg, "<message truncated>");
   va_end(args);

   _eglDefaultLogger(level, msg);

   mtx_unlock(&logging.mutex);

   if (level == _EGL_FATAL)
      exit(1); /* or abort()? */
}
