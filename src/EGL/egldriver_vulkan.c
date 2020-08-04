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


static VkInstance _createVkInstanceInternal(void)
{
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

   return vkInstance;
}

static VkPhysicalDevice* _enumeratePhysicalDevices(VkInstance instance, uint32_t* pCount)
{
   VkPhysicalDevice* pPhyDevs;

   // Performance_query to get the number of the GPUs.
   // If it does not work on your GPUs, 
   // execute below command: (for intel GPUs)
   // $ sudo sysctl -w dev.i915.perf_stream_paranoid=0
   vkEnumeratePhysicalDevices(instance, pCount, NULL);

   pPhyDevs = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * (*pCount));
   vkEnumeratePhysicalDevices(instance, pCount, pPhyDevs);

   return pPhyDevs;
}

static EGLBoolean
_Initialize(_EGLDriver *drv, _EGLDisplay *disp)
{
   EGLBoolean ret = EGL_FALSE;
   VkInstance vkInstance;
   VkPhysicalDevice* phyDevList;
   uint32_t countPhyDev;
   
   switch (disp->Platform) {
      case _EGL_PLATFORM_VULKAN_SURFACELESS:
      // Vulkan instance was already set by the client
      vkInstance = (VkInstance)disp->PlatformDisplay;
      break;
   case _EGL_PLATFORM_VULKAN:
      // Vulkan instance needs to be created by this driver
      vkInstance = _createVkInstanceInternal();
      disp->PlatformDisplay = (void*)vkInstance;
      break;
   default:
      return EGL_FALSE;
   }
   
   if(!vkInstance)
      return EGL_FALSE;
   
   phyDevList = _enumeratePhysicalDevices(vkInstance, &countPhyDev);
   
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

