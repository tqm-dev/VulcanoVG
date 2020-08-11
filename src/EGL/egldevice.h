
#ifndef EGLDEVICE_INCLUDED
#define EGLDEVICE_INCLUDED


#include <stdbool.h>
#include <stddef.h>
#include "egltypedefs.h"
#include "egldeviceVulkan.h"

#ifdef __cplusplus
extern "C" {
#endif

extern _EGLDevice _eglSoftwareDevice;

void
_eglFiniDevice(void);

EGLBoolean
_eglCheckDeviceHandle(EGLDeviceEXT device);

static inline _EGLDevice *
_eglLookupDevice(EGLDeviceEXT device)
{
   _EGLDevice *dev = (_EGLDevice *) device;
   if (!_eglCheckDeviceHandle(device))
      dev = NULL;
   return dev;
}

_EGLDevice *
_eglAddDevice(void* device, bool software);

enum _egl_device_extension {
   _EGL_DEVICE_SOFTWARE,
   _EGL_DEVICE_DRM,
   _EGL_DEVICE_VULKAN_LOGICAL,
};

struct _egl_device {
   _EGLDevice *Next;

   const char *extensions;

   EGLBoolean MESA_device_software;
   EGLBoolean EXT_device_drm;
   EGLBoolean EXT_device_vulkan_logical;

   LogicalDevice vk;
};

typedef enum _egl_device_extension _EGLDeviceExtension;

EGLBoolean
_eglDeviceSupports(_EGLDevice *dev, _EGLDeviceExtension ext);

const char *
_eglGetDRMDeviceRenderNode(_EGLDevice *dev);

EGLBoolean
_eglQueryDeviceAttribEXT(_EGLDevice *dev, EGLint attribute,
                         EGLAttrib *value);

const char *
_eglQueryDeviceStringEXT(_EGLDevice *dev, EGLint name);

EGLBoolean
_eglQueryDevicesEXT(EGLint max_devices, _EGLDevice **devices,
                    EGLint *num_devices);

#ifdef __cplusplus
}
#endif

#endif /* EGLDEVICE_INCLUDED */
