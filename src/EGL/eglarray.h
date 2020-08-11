
#ifndef EGLARRAY_INCLUDED
#define EGLARRAY_INCLUDED

#include "c99_compat.h"

#include "egltypedefs.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef EGLBoolean (*_EGLArrayForEach)(void *elem, void *foreach_data);


struct _egl_array {
   const char *Name;
   EGLint MaxSize;

   void **Elements;
   EGLint Size;
};


extern _EGLArray *
_eglCreateArray(const char *name, EGLint init_size);


extern void
_eglDestroyArray(_EGLArray *array, void (*free_cb)(void *));


extern void
_eglAppendArray(_EGLArray *array, void *elem);


extern void
_eglEraseArray(_EGLArray *array, EGLint i, void (*free_cb)(void *));


void *
_eglFindArray(_EGLArray *array, void *elem);


extern EGLint
_eglFilterArray(_EGLArray *array, void **data, EGLint size,
                _EGLArrayForEach filter, void *filter_data);


EGLint
_eglFlattenArray(_EGLArray *array, void *buffer, EGLint elem_size, EGLint size,
                 _EGLArrayForEach flatten);


static inline EGLint
_eglGetArraySize(_EGLArray *array)
{
   return (array) ? array->Size : 0;
}


#ifdef __cplusplus
}
#endif

#endif /* EGLARRAY_INCLUDED */
