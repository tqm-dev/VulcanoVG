
#include "util/macros.h"

#include "eglcurrent.h"
#include "egldevice.h"
#include "egllog.h"
#include "eglglobals.h"
#include "egltypedefs.h"

void
_eglFiniDevice(void)
{
   _EGLDevice *dev_list, *dev;

   /* atexit function is called with global mutex locked */

   dev_list = _eglGlobal.DeviceList;

   /* The first device is static allocated SW device */
   assert(dev_list);
   assert(_eglDeviceSupports(dev_list, _EGL_DEVICE_SOFTWARE));
   dev_list = dev_list->Next;

   while (dev_list) {
      /* pop list head */
      dev = dev_list;
      dev_list = dev_list->Next;
//TODO: vulkan
//    drmFreeDevice(&dev->vk);
      free(dev);
   }

   _eglGlobal.DeviceList = NULL;
}

EGLBoolean
_eglCheckDeviceHandle(EGLDeviceEXT device)
{
   _EGLDevice *cur;

   mtx_lock(_eglGlobal.Mutex);
   cur = _eglGlobal.DeviceList;
   while (cur) {
      if (cur == (_EGLDevice *) device)
         break;
      cur = cur->Next;
   }
   mtx_unlock(_eglGlobal.Mutex);
   return (cur != NULL);
}

// This is a dummy device that is set by default
_EGLDevice _eglSoftwareDevice = {
   .Next = NULL,
   .extensions = "EGL_MESA_device_software",
   .MESA_device_software = EGL_TRUE,
   .EXT_device_drm = EGL_FALSE,
   .EXT_device_vulkan_logical = EGL_FALSE,
};

/* Adds a device in DeviceList, if needed for the given fd.
 *
 * If a software device, the fd is ignored.
 */
_EGLDevice *
_eglAddVulkanDevice(VCLogical* logical, VCPhysical* physical)
{
   mtx_lock(_eglGlobal.Mutex);

   /* The first device is always software */
   _EGLDevice *dev = _eglGlobal.DeviceList;
   assert(_eglDeviceSupports(dev, _EGL_DEVICE_SOFTWARE));
   
   while (dev->Next) {
      dev = dev->Next;
      assert(_eglDeviceSupports(dev, _EGL_DEVICE_VULKAN_LOGICAL));
   }
   
   dev->Next = calloc(1, sizeof(_EGLDevice));
   dev = dev->Next;
   dev->Next = NULL;
   dev->extensions = "EXT_device_vulkan_logical";
   dev->EXT_device_vulkan_logical = EGL_TRUE;
   dev->EXT_device_drm = EGL_FALSE;
   dev->MESA_device_software = EGL_FALSE;
   dev->logical = *logical;
   dev->physical = *physical;

   // return to top
   dev = _eglGlobal.DeviceList;

   mtx_unlock(_eglGlobal.Mutex);

   return dev;
}

EGLBoolean
_eglDeviceSupports(_EGLDevice *dev, _EGLDeviceExtension ext)
{
   switch (ext) {
   case _EGL_DEVICE_SOFTWARE:
      return dev->MESA_device_software;
   case _EGL_DEVICE_DRM:
      return dev->EXT_device_drm;
   case _EGL_DEVICE_VULKAN_LOGICAL:
      return dev->EXT_device_vulkan_logical;
   default:
      assert(0);
      return EGL_FALSE;
   };
}

/* Ideally we'll have an extension which passes the render node,
 * instead of the card one + magic.
 *
 * Then we can move this in _eglQueryDeviceStringEXT below. Until then
 * keep it separate.
 */
const char *
_eglGetDRMDeviceRenderNode(_EGLDevice *dev)
{
   return NULL;
}

EGLBoolean
_eglQueryDeviceAttribEXT(_EGLDevice *dev, EGLint attribute,
                         EGLAttrib *value)
{
   switch (attribute) {
   default:
      _eglError(EGL_BAD_ATTRIBUTE, "eglQueryDeviceStringEXT");
      return EGL_FALSE;
   }
}

const char *
_eglQueryDeviceStringEXT(_EGLDevice *dev, EGLint name)
{
   switch (name) {
   case EGL_EXTENSIONS:
      return dev->extensions;
      /* fall through */
   default:
      _eglError(EGL_BAD_PARAMETER, "eglQueryDeviceStringEXT");
      return NULL;
   };
}

EGLBoolean
_eglQueryDevicesEXT(EGLint max_devices,
                    _EGLDevice **devices,
                    EGLint *num_devices)
{
   _EGLDevice *dev, *devs;
   int i = 0, num_devs;

   if ((devices && max_devices <= 0) || !num_devices)
      return _eglError(EGL_BAD_PARAMETER, "eglQueryDevicesEXT");

   mtx_lock(_eglGlobal.Mutex);

//TODO   num_devs = _eglRefreshDeviceList();
   devs = _eglGlobal.DeviceList;

   /* bail early if we only care about the count */
   if (!devices) {
      *num_devices = num_devs;
      goto out;
   }

   /* Push the first device (the software one) to the end of the list.
    * Sending it to the user only if they've requested the full list.
    *
    * By default, the user is likely to pick the first device so having the
    * software (aka least performant) one is not a good idea.
    */
   *num_devices = MIN2(num_devs, max_devices);

   for (i = 0, dev = devs->Next; dev && i < max_devices; i++) {
      devices[i] = dev;
      dev = dev->Next;
   }

   /* User requested the full device list, add the sofware device. */
   if (max_devices >= num_devs) {
      assert(_eglDeviceSupports(devs, _EGL_DEVICE_SOFTWARE));
      devices[num_devs - 1] = devs;
   }

out:
   mtx_unlock(_eglGlobal.Mutex);

   return EGL_TRUE;
}
