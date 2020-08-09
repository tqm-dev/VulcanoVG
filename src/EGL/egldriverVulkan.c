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
#include "eglconfig.h"
#include "eglcontext.h"
#include "eglsurface.h"

#include <VG/openvg.h>
#include "shContext.h"


static VkInstance
_getInstance(
   _EGLDisplay *disp // in/out
){
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

#define MAX_QUEUE_FAMILY 10
static void
_enumeratePhysicalDevices(
   VkInstance      vk,   // in
   PhysicalDevice* devs, // out
   uint32_t*       count // out
){
   // If Performance_query does not work on your GPUs, 
   // execute below command (for intel GPUs):
   // $ sudo sysctl -w dev.i915.perf_stream_paranoid=0
   vkEnumeratePhysicalDevices(vk, count, NULL);

   VkPhysicalDevice phy_devs[*count];
   vkEnumeratePhysicalDevices(vk, count, phy_devs);

   for (uint32_t i = 0; i < *count; ++i)
   {
      uint32_t qfc = 0;
      devs[i].physical_device = phy_devs[i];
      
      vkGetPhysicalDeviceProperties(devs[i].physical_device, &devs[i].properties);
      vkGetPhysicalDeviceFeatures(devs[i].physical_device, &devs[i].features);
      vkGetPhysicalDeviceMemoryProperties(devs[i].physical_device, &devs[i].memories);
      
      devs[i].queue_family_count = MAX_QUEUE_FAMILY;
      vkGetPhysicalDeviceQueueFamilyProperties(devs[i].physical_device, &qfc, NULL);
      vkGetPhysicalDeviceQueueFamilyProperties(devs[i].physical_device, &devs[i].queue_family_count, devs[i].queue_families);
      
      devs[i].queue_families_incomplete = devs[i].queue_family_count < qfc;
   }
}

static int _createLogicalDevice(
   PhysicalDevice*         phy_dev,
   LogicalDevice*          dev,
   VkQueueFlags            qflags,
   VkDeviceQueueCreateInfo queue_info[],
   uint32_t*               queue_info_count, // in/out  This will be edited by checking qflags
   const char *            ext_names[],
   uint32_t                ext_count
){
   int retval = 0;
   VkResult res;
   
   /* Here is to hoping we don't have to redo this function again ;) */
   *dev = (LogicalDevice){0};
   
   /* We have already seen how to create a logical device and request queues in Tutorial 2 and again in 5 */
   uint32_t max_queue_count = *queue_info_count;
   *queue_info_count = 0;
   
   uint32_t max_family_queues = 0;
   for (uint32_t i = 0; i < phy_dev->queue_family_count; ++i)
      if (max_family_queues < phy_dev->queue_families[i].queueCount)
         max_family_queues = phy_dev->queue_families[i].queueCount;

   float queue_priorities[max_family_queues];
   memset(queue_priorities, 0, sizeof queue_priorities);
   
   for (uint32_t i = 0; i < phy_dev->queue_family_count && i < max_queue_count; ++i)
   {
      VkDeviceQueueCreateInfo info = {0};

      /* Check if the queue has the desired capabilities.  If so, add it to the list of desired queues */
      if ((phy_dev->queue_families[i].queueFlags & qflags) == 0)
         continue;
      
      info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      info.queueFamilyIndex = i;
      info.queueCount = phy_dev->queue_families[i].queueCount;
      info.pQueuePriorities = queue_priorities;
      queue_info[(*queue_info_count)++] = info;
   }
   
   /* If there are no compatible queues, there is little one can do here */
   if (*queue_info_count == 0)
   {
      retval = -1;
      goto exit_failed;
   }
   
   VkDeviceCreateInfo dev_info = {0};
   dev_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   dev_info.queueCreateInfoCount = *queue_info_count;
   dev_info.pQueueCreateInfos = queue_info;
   dev_info.enabledExtensionCount = ext_count;
   dev_info.ppEnabledExtensionNames = ext_names;
   dev_info.pEnabledFeatures = &phy_dev->features;
   
   vkCreateDevice(phy_dev->physical_device, &dev_info, NULL, &dev->device);
   
exit_failed:
   return retval;

}

int 
_createCommandBuffers(
   PhysicalDevice *        phy_dev,
   LogicalDevice *         dev,
   VkDeviceQueueCreateInfo queue_info[],
   uint32_t                queue_info_count
){
   int retval = 0;
   VkResult res;
   
   dev->command_pools = malloc(queue_info_count * sizeof *dev->command_pools);
   if (dev->command_pools == NULL)
   {
      retval = -1;
      goto exit_failed;
   }
   
   for (uint32_t i = 0; i < queue_info_count; ++i)
   {
      CommandPool *cmd = &dev->command_pools[i];
      *cmd = (CommandPool){0};
      
      cmd->qflags = phy_dev->queue_families[queue_info[i].queueFamilyIndex].queueFlags;
      
      VkCommandPoolCreateInfo pool_info = {0};
      pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      pool_info.queueFamilyIndex = queue_info[i].queueFamilyIndex;
      
      res = vkCreateCommandPool(dev->device, &pool_info, NULL, &cmd->pool);
      if (res < 0){
          retval = -1;
         goto exit_failed;
      }
      ++dev->command_pool_count;
      
      cmd->queues = malloc(queue_info[i].queueCount * sizeof *cmd->queues);
      cmd->buffers = malloc(queue_info[i].queueCount * sizeof *cmd->buffers);
      if (cmd->queues == NULL || cmd->buffers == NULL)
      {
          retval = -1;
         goto exit_failed;
      }
      
      for (uint32_t j = 0; j < queue_info[i].queueCount; ++j)
         vkGetDeviceQueue(dev->device, queue_info[i].queueFamilyIndex, j, &cmd->queues[j]);

      cmd->queue_count = queue_info[i].queueCount;
      
      VkCommandBufferAllocateInfo buffer_info = {0};
      buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      buffer_info.commandPool = cmd->pool;
      buffer_info.commandBufferCount = queue_info[i].queueCount;
      
      res = vkAllocateCommandBuffers(dev->device, &buffer_info, cmd->buffers);
      if (res){
          retval = -1;
         goto exit_failed;
      }
      
      cmd->buffer_count = queue_info[i].queueCount;
   }

exit_failed:
   return retval;
}

static void
_createLogicalDevices(
   PhysicalDevice* phyDevList, // in
   LogicalDevice*  logDevList, // out
   uint32_t        count       // in
){
   const char *extension_names[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
   int res;

   for (uint32_t i = 0; i < count; ++i) {

      PhysicalDevice *phy_dev = &phyDevList[i];
      LogicalDevice  *dev = &logDevList[i];
      VkDeviceQueueCreateInfo queue_info[phy_dev->queue_family_count];
      uint32_t queue_info_count = phy_dev->queue_family_count;

      // Create VkDevice for graphics processing
      res = _createLogicalDevice(phy_dev, dev, VK_QUEUE_GRAPHICS_BIT, queue_info,
              &queue_info_count, // Containing all of queue family
              extension_names, sizeof extension_names / sizeof *extension_names);

      // Setup command buffers
      if(res == 0)
         _createCommandBuffers(phy_dev, dev, queue_info,
            queue_info_count // Containing only a graphics queue family because of only VK_QUEUE_GRAPHICS_BIT enabled.
         );
   }
}
   
static void
_addLogicalDevices(
   LogicalDevice* logDevList, // in
   uint32_t       count,      // in
   _EGLDisplay*   disp        // out
){
   _EGLDevice *top = NULL;

   // Add _EGLDevice containing the LogicalDevice to the list in _eglGlobal.
   for(uint32_t i = 0; i < count; i++){
      top = _eglAddDevice((void*)&logDevList[i], false); // Always returns a top of the list of _EGLDevice.
      if (!top)
         break;
   }

   // Make _EGLDisplay can access to the list of _EGLDevice
   disp->Device = top;
}
 
static _EGLConfig _baseConfig = {0};
static void
_setupDefaultConfigs(_EGLDisplay* disp
){
   EGLint configId;

   switch(disp->Platform) {
      case _EGL_PLATFORM_VULKAN:
         configId = EGL_CONFIG_ID_VULKAN_VG;
         break;
      case _EGL_PLATFORM_VULKAN_SURFACELESS:
         configId = EGL_CONFIG_ID_VULKAN_VG_SURFACELESS;
         break;
      default:
         assert(0);
         return;
   }

   _eglInitConfig(&_baseConfig, disp, configId);
   _eglLinkConfig(&_baseConfig);
}

#define VULKAN_DEVICE_MAX   8
static EGLBoolean
_Initialize(
   _EGLDriver *drv,
   _EGLDisplay *disp
){
   VkInstance vkInstance;
   PhysicalDevice phyDevList[VULKAN_DEVICE_MAX] = {0};
   LogicalDevice  logDevList[VULKAN_DEVICE_MAX] = {0};
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
   _createLogicalDevices(phyDevList, logDevList, countDev);

   /* Add logical devices to _EGLDevice*/
  _addLogicalDevices(logDevList, countDev, disp);

   /* Setup egl configs */
  _setupDefaultConfigs(disp);

   return EGL_TRUE;
}

extern void shLoadExtensions(VGContext *c);
static bool _initVGContext(VGContext *c)
{
  /* Surface info */
  c->surfaceWidth = 0;
  c->surfaceHeight = 0;
  
  /* GetString info */
  strncpy(c->vendor, "John doe", sizeof(c->vendor));
  strncpy(c->renderer, "VulkanVG 0.1.0", sizeof(c->renderer));
  strncpy(c->version, "1.0", sizeof(c->version));
  strncpy(c->extensions, "", sizeof(c->extensions));
  
  /* Mode settings */
  c->matrixMode = VG_MATRIX_PATH_USER_TO_SURFACE;
  c->fillRule = VG_EVEN_ODD;
  c->imageQuality = VG_IMAGE_QUALITY_FASTER;
  c->renderingQuality = VG_RENDERING_QUALITY_BETTER;
  c->blendMode = VG_BLEND_SRC_OVER;
  c->imageMode = VG_DRAW_IMAGE_NORMAL;
  
  /* Scissor rectangles */
  SH_INITOBJ(SHRectArray, c->scissor);
  c->scissoring = VG_FALSE;
  c->masking = VG_FALSE;
  
  /* Stroke parameters */
  c->strokeLineWidth = 1.0f;
  c->strokeCapStyle = VG_CAP_BUTT;
  c->strokeJoinStyle = VG_JOIN_MITER;
  c->strokeMiterLimit = 4.0f;
  c->strokeDashPhase = 0.0f;
  c->strokeDashPhaseReset = VG_FALSE;
  SH_INITOBJ(SHFloatArray, c->strokeDashPattern);
  
  /* Edge fill color for vgConvolve and pattern paint */
  CSET(c->tileFillColor, 0,0,0,0);
  
  /* Color for vgClear */
  CSET(c->clearColor, 0,0,0,0);
  
  /* Color components layout inside pixel */
  c->pixelLayout = VG_PIXEL_LAYOUT_UNKNOWN;
  
  /* Source format for image filters */
  c->filterFormatLinear = VG_FALSE;
  c->filterFormatPremultiplied = VG_FALSE;
  c->filterChannelMask = VG_RED|VG_GREEN|VG_BLUE|VG_ALPHA;
  
  /* Matrices */
  SH_INITOBJ(SHMatrix3x3, c->pathTransform);
  SH_INITOBJ(SHMatrix3x3, c->imageTransform);
  SH_INITOBJ(SHMatrix3x3, c->fillTransform);
  SH_INITOBJ(SHMatrix3x3, c->strokeTransform);
  
  /* Paints */
  c->fillPaint = NULL;
  c->strokePaint = NULL;
  SH_INITOBJ(SHPaint, c->defaultPaint);
  
  /* Error */
  c->error = VG_NO_ERROR;
  
  /* Resources */
  SH_INITOBJ(SHPathArray, c->paths);
  SH_INITOBJ(SHPaintArray, c->paints);
  SH_INITOBJ(SHImageArray, c->images);

  shLoadExtensions(c);

  return true;
}

typedef struct {
   _EGLContext base;
   VGContext   vg;
} VuContext;

static _EGLContext *
_CreateContext(_EGLDriver *drv, _EGLDisplay *disp, _EGLConfig *conf,
                    _EGLContext *share_list, const EGLint *attrib_list)
{

   VuContext* vuCtx = malloc(sizeof(VuContext));
   if (!vuCtx) {
      return NULL;
   }

   if (!_eglInitContext(&vuCtx->base, disp, conf, attrib_list))
      goto cleanup;

   if(!_initVGContext(&vuCtx->vg))
      goto cleanup;

   return &vuCtx->base;

cleanup:
   free(vuCtx);
   return NULL;
}


static _EGLDevice* _getPrimaryDevice(_EGLDisplay* disp)
{
   _EGLDevice* eglDev = disp->Device;
   while(eglDev){
      if(_eglDeviceSupports(eglDev,_EGL_DEVICE_VULKAN_LOGICAL))
         return eglDev;
      eglDev = eglDev->Next;
   }

   return NULL;
}


typedef struct {
   _EGLSurface   base;
   VkFramebuffer fb;
} VuSurface;

_EGLSurface* createPbufferFromClientBuffer(
   _EGLDriver *drv,
   _EGLDisplay *disp,
   EGLenum buftype,
   EGLClientBuffer buffer,
   _EGLConfig *config,
   const EGLint *attrib_list
){
   VuSurface* vuSurf = malloc(sizeof(VuSurface));

   if(buftype != EGL_OPENVG_IMAGE);
      goto cleanup;

   if(!buffer || !((SHImage*)buffer)->vkImageView)
      goto cleanup;

   if(!_eglInitSurface(&vuSurf->base, disp, EGL_PBUFFER_BIT, config, attrib_list, NULL))
      goto cleanup;

   _EGLDevice* dev = _getPrimaryDevice(disp);

   if(dev == NULL || dev->vk.device == NULL)
      goto cleanup;

   // Create render pass
   VkRenderPass renderPass;
   {
      VkAttachmentDescription attachmentDesc[1];
      attachmentDesc[0].format = VK_FORMAT_R8G8B8A8_SRGB;
      attachmentDesc[0].samples = VK_SAMPLE_COUNT_1_BIT;
      attachmentDesc[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachmentDesc[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachmentDesc[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      attachmentDesc[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      VkAttachmentReference attachmentRefs[1];
      attachmentRefs[0].attachment = 0; /* corresponds to the index in pAttachments of VkRenderPassCreateInfo */
      attachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      VkSubpassDescription subpassDescs[1];
      subpassDescs[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpassDescs[0].colorAttachmentCount = 1;
      subpassDescs[0].pColorAttachments = &attachmentRefs[0];
      subpassDescs[0].pDepthStencilAttachment = NULL;

      VkRenderPassCreateInfo rpInfo;
      rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      rpInfo.attachmentCount = 1;
      rpInfo.pAttachments = attachmentDesc;
      rpInfo.subpassCount = 1;
      rpInfo.pSubpasses = subpassDescs;

      vkCreateRenderPass(dev->vk.device, &rpInfo, NULL, &renderPass);
   }

   // Create frame buffer
   {
      VkImageView fbAttachments[1];
      SHImage* image = (SHImage*)buffer;
      fbAttachments[0] = image->vkImageView;

      VkFramebufferCreateInfo fbInfo = {0};
      fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      fbInfo.pNext = NULL;
      fbInfo.flags = 0;
      fbInfo.renderPass = renderPass;
      fbInfo.attachmentCount = 1;
      fbInfo.pAttachments = fbAttachments;
      fbInfo.width = image->width;
      fbInfo.height = image->height;
      fbInfo.layers = 1;
      if(vkCreateFramebuffer(dev->vk.device, &fbInfo, NULL, &vuSurf->fb) != VK_SUCCESS)
         goto cleanup;
   }

   return &vuSurf->base;

cleanup:
   free(vuSurf);
   return EGL_NO_SURFACE;
}

_EGLDriver _eglDriver = {
   .Initialize                    = _Initialize,
   .Terminate                     = NULL,
   .CreateContext                 = _CreateContext,
   .DestroyContext                = NULL,
   .MakeCurrent                   = NULL,
   .CreateWindowSurface           = NULL,
   .CreatePixmapSurface           = NULL,
   .CreatePbufferSurface          = NULL,
   .CreatePbufferFromClientBuffer = createPbufferFromClientBuffer,
   .DestroySurface                = NULL,
   .GetProcAddress                = NULL,
   .WaitClient                    = NULL,
   .WaitNative                    = NULL,
   .BindTexImage                  = NULL,
   .ReleaseTexImage               = NULL,
   .SwapInterval                  = NULL,
   .SwapBuffers                   = NULL,
   .CopyBuffers                   = NULL,
   .QueryBufferAge                = NULL,
   .CreateImageKHR                = NULL,
   .DestroyImageKHR               = NULL,
   .QuerySurface                  = NULL,
   .QueryDriverName               = NULL,
   .QueryDriverConfig             = NULL,
   .CreateSyncKHR                 = NULL,
   .ClientWaitSyncKHR             = NULL,
   .SignalSyncKHR                 = NULL,
   .WaitSyncKHR                   = NULL,
   .DestroySyncKHR                = NULL,
};

