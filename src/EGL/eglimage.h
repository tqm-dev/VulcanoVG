
#ifndef EGLIMAGE_INCLUDED
#define EGLIMAGE_INCLUDED

#include "c99_compat.h"

#include "egltypedefs.h"
#include "egldisplay.h"


#ifdef __cplusplus
extern "C" {
#endif

struct _egl_image_attrib_int
{
   EGLint Value;
   EGLBoolean IsPresent;
};

#define DMA_BUF_MAX_PLANES 4

struct _egl_image_attribs
{
   /* EGL_KHR_image_base */
   EGLBoolean ImagePreserved;

   /* EGL_KHR_gl_image */
   EGLint GLTextureLevel;
   EGLint GLTextureZOffset;

   /* EGL_MESA_drm_image */
   EGLint Width;
   EGLint Height;
   EGLint DRMBufferFormatMESA;
   EGLint DRMBufferUseMESA;
   EGLint DRMBufferStrideMESA;

   /* EGL_WL_bind_wayland_display */
   EGLint PlaneWL;

   /* EGL_EXT_image_dma_buf_import and
    * EGL_EXT_image_dma_buf_import_modifiers */
   struct _egl_image_attrib_int DMABufFourCC;
   struct _egl_image_attrib_int DMABufPlaneFds[DMA_BUF_MAX_PLANES];
   struct _egl_image_attrib_int DMABufPlaneOffsets[DMA_BUF_MAX_PLANES];
   struct _egl_image_attrib_int DMABufPlanePitches[DMA_BUF_MAX_PLANES];
   struct _egl_image_attrib_int DMABufPlaneModifiersLo[DMA_BUF_MAX_PLANES];
   struct _egl_image_attrib_int DMABufPlaneModifiersHi[DMA_BUF_MAX_PLANES];
   struct _egl_image_attrib_int DMABufYuvColorSpaceHint;
   struct _egl_image_attrib_int DMABufSampleRangeHint;
   struct _egl_image_attrib_int DMABufChromaHorizontalSiting;
   struct _egl_image_attrib_int DMABufChromaVerticalSiting;
};

/**
 * "Base" class for device driver images.
 */
struct _egl_image
{
   /* An image is a display resource */
   _EGLResource Resource;
};


EGLBoolean
_eglParseImageAttribList(_EGLImageAttribs *attrs, _EGLDisplay *disp,
                         const EGLint *attrib_list);


static inline void
_eglInitImage(_EGLImage *img, _EGLDisplay *disp)
{
   _eglInitResource(&img->Resource, sizeof(*img), disp);
}


/**
 * Increment reference count for the image.
 */
static inline _EGLImage *
_eglGetImage(_EGLImage *img)
{
   if (img)
      _eglGetResource(&img->Resource);
   return img;
}


/**
 * Decrement reference count for the image.
 */
static inline EGLBoolean
_eglPutImage(_EGLImage *img)
{
   return (img) ? _eglPutResource(&img->Resource) : EGL_FALSE;
}


/**
 * Link an image to its display and return the handle of the link.
 * The handle can be passed to client directly.
 */
static inline EGLImage
_eglLinkImage(_EGLImage *img)
{
   _eglLinkResource(&img->Resource, _EGL_RESOURCE_IMAGE);
   return (EGLImage) img;
}


/**
 * Unlink a linked image from its display.
 * Accessing an unlinked image should generate EGL_BAD_PARAMETER error.
 */
static inline void
_eglUnlinkImage(_EGLImage *img)
{
   _eglUnlinkResource(&img->Resource, _EGL_RESOURCE_IMAGE);
}


/**
 * Lookup a handle to find the linked image.
 * Return NULL if the handle has no corresponding linked image.
 */
static inline _EGLImage *
_eglLookupImage(EGLImage image, _EGLDisplay *disp)
{
   _EGLImage *img = (_EGLImage *) image;
   if (!disp || !_eglCheckResource((void *) img, _EGL_RESOURCE_IMAGE, disp))
      img = NULL;
   return img;
}


/**
 * Return the handle of a linked image, or EGL_NO_IMAGE_KHR.
 */
static inline EGLImage
_eglGetImageHandle(_EGLImage *img)
{
   _EGLResource *res = (_EGLResource *) img;
   return (res && _eglIsResourceLinked(res)) ?
      (EGLImage) img : EGL_NO_IMAGE_KHR;
}


#ifdef __cplusplus
}
#endif

#endif /* EGLIMAGE_INCLUDED */
