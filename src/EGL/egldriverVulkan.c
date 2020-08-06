/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Kristian Høgsberg <krh@bitplanet.net>
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <c11/threads.h>
#include <time.h>
#include <vulkan/vulkan.h>
#include "egldefines.h"
#include "egldriver.h"
#include "egldisplay.h"
#include "egldevice.h"


static VkInstance
_getInstance(_EGLDisplay *disp)
{
   if(disp->Platform == _EGL_PLATFORM_VULKAN_SURFACELESS) {

      // Vulkan instance was already set by the client
      return (VkInstance)disp->PlatformDisplay;

   } else if(disp->Platform == _EGL_PLATFORM_VULKAN) {

      // Vulkan instance needs to be created by this driver
      VkApplicationInfo appInfo;
      VkInstanceCreateInfo createInfo;
      VkInstance vkInstance;
      VkResult result;
      
      /* App info */
      appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      appInfo.pApplicationName = "VulkanVG";
      appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
      appInfo.pEngineName = "No Engine";
      appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
      appInfo.apiVersion = VK_API_VERSION_1_2;
      
      /* Instance create info */
      createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      createInfo.pNext = NULL;
      createInfo.flags = 0;
      createInfo.pApplicationInfo = &appInfo;
      createInfo.enabledLayerCount = 0;
      createInfo.ppEnabledLayerNames = NULL;
      createInfo.enabledExtensionCount = 0;
      createInfo.ppEnabledExtensionNames = NULL;
      
      /* Instance */
      result = vkCreateInstance(&createInfo, NULL, &vkInstance);
      if(result != VK_SUCCESS)
         return NULL;
      
      disp->PlatformDisplay = (void*)vkInstance;
      return vkInstance;

   } else {
      // Not support
      return NULL;
   }
}

static void
_enumeratePhysicalDevices(VkInstance instance, VkPhysicalDevice* pPhyDevs, uint32_t* pCount)
{
   // Performance_query to get the number of the GPUs.
   // If it does not work on your GPUs, 
   // execute below command: (for intel GPUs)
   // $ sudo sysctl -w dev.i915.perf_stream_paranoid=0
   vkEnumeratePhysicalDevices(instance, pCount, NULL);
   vkEnumeratePhysicalDevices(instance, pCount, pPhyDevs);
}

static void
_createLogicalDevices(VkInstance instance, VkPhysicalDevice* phyDevList, uint32_t count, _EGLDisplay *disp)
{
   _EGLDevice *deviceList = NULL;
   VkLogicalDevice vk = {0};

   for(uint32_t i = 0; i < count; i++){
      
      // TODO: Create VkDevice from phyDevList.
      vk.device = NULL;

      // _eglAddDevice will create and add _EGLDevice to the list in _eglGlobal.
      deviceList = _eglAddDevice((void*)&vk, false); // Always returns a top of the list of _EGLDevice.
      if (!deviceList)
         break;
   }

   disp->Device = deviceList;
}
   
#define VULKAN_DEVICE_MAX   8
static EGLBoolean
_Initialize(_EGLDriver *drv, _EGLDisplay *disp)
{
   VkInstance vkInstance;
   VkPhysicalDevice phyDevList[VULKAN_DEVICE_MAX] = {0};
   uint32_t countDev;
  
   /* Get instance */
   vkInstance = _getInstance(disp);
   if(vkInstance == NULL)
      return EGL_FALSE;

   /* Get physical devices */
   _enumeratePhysicalDevices(vkInstance, phyDevList, &countDev);
   if(countDev > VULKAN_DEVICE_MAX)
      return EGL_FALSE;

   /* Create logical devices */
   _createLogicalDevices(vkInstance, phyDevList, countDev, disp);

   return EGL_TRUE;
}

_EGLDriver _eglDriver = {
   .Initialize						= _Initialize,
   .Terminate						= NULL,
   .CreateContext					= NULL,
   .DestroyContext					= NULL,
   .MakeCurrent						= NULL,
   .CreateWindowSurface				= NULL,
   .CreatePixmapSurface				= NULL,
   .CreatePbufferSurface			= NULL,
   .DestroySurface					= NULL,
   .GetProcAddress					= NULL,
   .WaitClient						= NULL,
   .WaitNative						= NULL,
   .BindTexImage					= NULL,
   .ReleaseTexImage					= NULL,
   .SwapInterval					= NULL,
   .SwapBuffers						= NULL,
   .SwapBuffersWithDamageEXT		= NULL,
   .SwapBuffersRegionNOK			= NULL,
   .SetDamageRegion					= NULL,
   .PostSubBufferNV					= NULL,
   .CopyBuffers						= NULL,
   .QueryBufferAge					= NULL,
   .CreateImageKHR					= NULL,
   .DestroyImageKHR					= NULL,
   .CreateWaylandBufferFromImageWL	= NULL,
   .QuerySurface					= NULL,
   .QueryDriverName					= NULL,
   .QueryDriverConfig				= NULL,
   .GetSyncValuesCHROMIUM			= NULL,
   .CreateSyncKHR					= NULL,
   .ClientWaitSyncKHR				= NULL,
   .SignalSyncKHR					= NULL,
   .WaitSyncKHR						= NULL,
   .DestroySyncKHR					= NULL,
   .GLInteropQueryDeviceInfo		= NULL,
   .GLInteropExportObject			= NULL,
   .DupNativeFenceFDANDROID			= NULL,
   .SetBlobCacheFuncsANDROID		= NULL
};

