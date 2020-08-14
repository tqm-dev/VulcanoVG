
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
      appInfo.pApplicationName = "VulcanoVG";
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
   VCPhysical* devs, // out
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
      devs[i].device = phy_devs[i];
      
      vkGetPhysicalDeviceProperties(devs[i].device, &devs[i].properties);
      vkGetPhysicalDeviceFeatures(devs[i].device, &devs[i].features);
      vkGetPhysicalDeviceMemoryProperties(devs[i].device, &devs[i].memories);
      
      devs[i].queue_family_count = MAX_QUEUE_FAMILY;
      vkGetPhysicalDeviceQueueFamilyProperties(devs[i].device, &qfc, NULL);
      vkGetPhysicalDeviceQueueFamilyProperties(devs[i].device, &devs[i].queue_family_count, devs[i].queue_families);
      
      devs[i].queue_families_incomplete = devs[i].queue_family_count < qfc;
   }
}

static int _createLogicalDevice(
   VCPhysical*         phy_dev,
   VCLogical*          dev,
   VkQueueFlags            qflags,
   VkDeviceQueueCreateInfo queue_info[],
   uint32_t*               queue_family_count, // in/out  This will be edited by checking qflags
   const char *            ext_names[],
   uint32_t                ext_count
){
   int retval = 0;
   VkResult res;
   
   /* Here is to hoping we don't have to redo this function again ;) */
   *dev = (VCLogical){0};
   
   /* We have already seen how to create a logical device and request queues in Tutorial 2 and again in 5 */
   uint32_t max_queue_count = *queue_family_count;
   *queue_family_count = 0;
   
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
      queue_info[(*queue_family_count)++] = info;
   }
   
   /* If there are no compatible queues, there is little one can do here */
   if (*queue_family_count == 0)
   {
      retval = -1;
      goto exit_failed;
   }
   
   VkDeviceCreateInfo dev_info = {0};
   dev_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   dev_info.queueCreateInfoCount = *queue_family_count;
   dev_info.pQueueCreateInfos = queue_info;
   dev_info.enabledExtensionCount = ext_count;
   dev_info.ppEnabledExtensionNames = ext_names;
   dev_info.pEnabledFeatures = &phy_dev->features;
   
   vkCreateDevice(phy_dev->device, &dev_info, NULL, &dev->device);
   
exit_failed:
   return retval;

}

int 
_createCommandBuffers(
   VCPhysical *        phy_dev,
   VCLogical *         dev,
   VkDeviceQueueCreateInfo queue_info[],
   uint32_t                queue_family_count,
   uint8_t                 buffer_num_per_queue
){
   int retval = 0;
   VkResult res;
   
   dev->queue_families = malloc(queue_family_count * sizeof *dev->queue_families);
   if (dev->queue_families == NULL)
   {
      retval = -1;
      goto exit_failed;
   }
   
   for (uint32_t i = 0; i < queue_family_count; ++i)
   {
      QueueFamily *cmd = &dev->queue_families[i];
      *cmd = (QueueFamily){0};
      
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
      buffer_info.commandBufferCount = queue_info[i].queueCount * buffer_num_per_queue;
      
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
_setupLogicalDevices(
   VCPhysical* phyDevList, // in
   VCLogical*  logDevList, // out
   uint32_t        deviceCount // in
){
   const char *extension_names[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
   int res;

   for (uint32_t i = 0; i < deviceCount; ++i) {

      VCPhysical *phy_dev = &phyDevList[i];
      VCLogical  *dev = &logDevList[i];
      VkDeviceQueueCreateInfo queue_info[phy_dev->queue_family_count];
      uint32_t queue_family_count = phy_dev->queue_family_count;

      // Create VkDevice for graphics processing
      res = _createLogicalDevice(phy_dev, dev, VK_QUEUE_GRAPHICS_BIT, queue_info,
              &queue_family_count, // Containing all of the queue families
              extension_names, sizeof extension_names / sizeof *extension_names);

      // Setup command buffers
      if(res == 0)
         _createCommandBuffers(phy_dev, dev, queue_info,
            queue_family_count, // It should equal to 1 (a graphics queue) because VK_QUEUE_GRAPHICS_BIT enabled.
            2                   // Command buffer num per command queue.
         );
   }
}
   
static void
_addLogicalDevices(
   VCPhysical*    phyDevList, // in
   VCLogical*     logDevList, // in
   uint32_t       deviceCount,// in
   _EGLDisplay*   disp        // out
){
   _EGLDevice *top = NULL;

   // Add _EGLDevice containing the VCLogical to the list in _eglGlobal.
   for(uint32_t i = 0; i < deviceCount; i++){
      top = _eglAddVulkanDevice((void*)&logDevList[i], (void*)&phyDevList[i]); // Always returns a top of the list of _EGLDevice.
      if (!top)
         break;
   }

   // Make _EGLDisplay can access to the list of _EGLDevice
   disp->Device = top;
}
 
static void
_setupDefaultConfigs(_EGLDisplay* disp
){
   EGLint configId;
   EGLint surfType;

   _EGLConfig* defaultConfig = malloc(sizeof(_EGLConfig));

   switch(disp->Platform) {
      case _EGL_PLATFORM_VULKAN:
         configId = EGL_CONFIG_ID_VULKAN_VG;
         surfType = EGL_PBUFFER_BIT | EGL_WINDOW_BIT;
         break;
      case _EGL_PLATFORM_VULKAN_SURFACELESS:
         configId = EGL_CONFIG_ID_VULKAN_VG_SURFACELESS;
         surfType = EGL_PBUFFER_BIT;
         break;
      default:
         goto cleanup;
   }

   // Init with config id
   _eglInitConfig(defaultConfig, disp, configId);

   // Modify config for VulcanoVG
   _eglSetConfigKey(defaultConfig, EGL_RENDERABLE_TYPE, EGL_OPENVG_BIT);
   _eglSetConfigKey(defaultConfig, EGL_SURFACE_TYPE, surfType);

   // Link config to egl display
   if(!_eglLinkConfig(defaultConfig))
      goto cleanup;

   return;

cleanup:
   assert(0);
   free(defaultConfig);
   return;
}

#define VULKAN_DEVICE_MAX   8
static EGLBoolean
_Initialize(
   _EGLDriver *drv,
   _EGLDisplay *disp
){
   VkInstance instance;
   VCPhysical phyDevList[VULKAN_DEVICE_MAX] = {0};
   VCLogical  logDevList[VULKAN_DEVICE_MAX] = {0};
   uint32_t countDev;
  
   /* Get instance */
   if((instance = _getInstance(disp)) == NULL)
      return EGL_FALSE;

   /* Get physical devices */
   _enumeratePhysicalDevices(instance, phyDevList, &countDev);
   if(countDev > VULKAN_DEVICE_MAX)
      return EGL_FALSE;

   /* Setup logical devices and command buffers*/
   _setupLogicalDevices(phyDevList, logDevList, countDev);

   /* Add devices to _EGLDevice*/
  _addLogicalDevices(phyDevList, logDevList, countDev, disp);

   /* Create default configs */
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
  strncpy(c->renderer, "VulcanoVG 0.1.0", sizeof(c->renderer));
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

VkRenderPass _createRenderPass(
   VkDevice      device
){

   VkRenderPass renderPass;
   if(!device)
      return NULL;

   // Create render pass
   {
      // Subpass descriptions
      VkAttachmentReference colorAttachmentRef = {
         .attachment = 0, /* corresponds to the index in pAttachments of VkRenderPassCreateInfo */
         .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      };
      VkSubpassDescription graphicsSubpassDesc = {
         .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
         .colorAttachmentCount = 1,
         .pColorAttachments = &colorAttachmentRef,
         .pDepthStencilAttachment = NULL,
      };
      VkSubpassDescription subpassDescs[1] = {
         graphicsSubpassDesc
      };

      // Attachment descriptions
      VkAttachmentDescription colorAttachmentDesc = {
         .format = VK_FORMAT_R8G8B8A8_SRGB,
         .samples = VK_SAMPLE_COUNT_1_BIT,
         .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
         .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
         .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
         .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      };
      VkAttachmentDescription attachmentDesc[1] = {
         colorAttachmentDesc
      };
      VkRenderPassCreateInfo rpInfo = {
         .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
         .attachmentCount = 1,
         .pAttachments = attachmentDesc,
         .subpassCount = 1,
         .pSubpasses = subpassDescs,
      };

      if(vkCreateRenderPass(device, &rpInfo, NULL, &renderPass) != VK_SUCCESS)
         return NULL;
   }

   return renderPass;
}

VkFramebuffer _createFrameBuffer(
   VkDevice      device, 
   VkRenderPass  renderPass,
   VkImageView   vkImageView,
   uint32_t      width,
   uint32_t      height
){

   VkFramebuffer frameBuffer;

   if(!device || !renderPass || !vkImageView)
      return NULL;

   // Create frame buffer
   {
      VkImageView fbAttachments[1];
      fbAttachments[0] = vkImageView;

      VkFramebufferCreateInfo fbInfo = {
         .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
         .pNext = NULL,
         .flags = 0,
         .renderPass = renderPass,
         .attachmentCount = 1,
         .pAttachments = fbAttachments,
         .width  = width,
         .height = height,
         .layers = 1,
      };
      if(vkCreateFramebuffer(device, &fbInfo, NULL, &frameBuffer) != VK_SUCCESS)
         return NULL;
   }

   return frameBuffer;
}


typedef struct {
   _EGLSurface   base;
   VkFramebuffer frameBuffer;
   VkRenderPass  renderPass;
} VuSurface;

static EGLBoolean _vuInitSurface(
   VuSurface* vuSurf,
   VkDevice   device,
   VkImage    vkImageView,
   uint32_t   width,
   uint32_t   height
){
   if(!vuSurf || !device || !vkImageView)
      return EGL_FALSE;

   // Render pass
   {
      // Subpass descriptions
      VkAttachmentReference colorAttachmentRef = {
         .attachment = 0, /* corresponds to the index in pAttachments of VkRenderPassCreateInfo */
         .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      };
      VkSubpassDescription graphicsSubpassDesc = {
         .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
         .colorAttachmentCount = 1,
         .pColorAttachments = &colorAttachmentRef,
         .pDepthStencilAttachment = NULL,
      };
      VkSubpassDescription subpassDescs[1] = {
         graphicsSubpassDesc
      };
   
      // Attachment descriptions
      VkAttachmentDescription colorAttachmentDesc = {
         .format = VK_FORMAT_R8G8B8A8_SRGB,
         .samples = VK_SAMPLE_COUNT_1_BIT,
         .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
         .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
         .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
         .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      };
      VkAttachmentDescription attachmentDesc[1] = {
         colorAttachmentDesc
      };
      VkRenderPassCreateInfo rpInfo = {
         .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
         .attachmentCount = 1,
         .pAttachments = attachmentDesc,
         .subpassCount = 1,
         .pSubpasses = subpassDescs,
      };
   
      if(vkCreateRenderPass(device, &rpInfo, NULL, &vuSurf->renderPass) != VK_SUCCESS)
         return EGL_FALSE;
   }

   // Frame buffer
   {
      VkImageView fbAttachments[1];
      fbAttachments[0] = vkImageView;

      VkFramebufferCreateInfo fbInfo = {
         .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
         .pNext = NULL,
         .flags = 0,
         .renderPass = vuSurf->renderPass,
         .attachmentCount = 1,
         .pAttachments = fbAttachments,
         .width  = width,
         .height = height,
         .layers = 1,
      };
      if(vkCreateFramebuffer(device, &fbInfo, NULL, &vuSurf->frameBuffer) != VK_SUCCESS)
         return EGL_FALSE;
   }

   return EGL_TRUE;
}

static _EGLSurface*
createWindowSurface(
   _EGLDriver   *drv,
   _EGLDisplay  *disp,
   _EGLConfig   *conf,
   void         *native_window,
   const EGLint *attrib_list
){
   _EGLDevice  *dev    = _getPrimaryDevice(disp);
   VuSurface   *vuSurf = malloc(sizeof(VuSurface));
   _EGLSurface *surf = &vuSurf->base;

   if(!_eglInitSurface(surf, disp, EGL_WINDOW_BIT, conf, attrib_list, native_window))
      goto cleanup;

   // TODO: Get vkImageView,width,height from native_window
   SHImage *image  = (SHImage*)native_window;
   surf->Width  = image->width;
   surf->Height = image->height; 

   if(!_vuInitSurface(vuSurf, dev->logical.device, image->vkImageView, image->width, image->height))
      goto cleanup;

   return surf;

cleanup:
   free(vuSurf);
   return NULL;
}

_EGLSurface* _createPbufferFromClientBuffer(
   _EGLDriver *drv,
   _EGLDisplay *disp,
   EGLenum buftype,
   EGLClientBuffer buffer,
   _EGLConfig *conf,
   const EGLint *attrib_list
){
   _EGLDevice  *dev    = _getPrimaryDevice(disp);
   VuSurface   *vuSurf = malloc(sizeof(VuSurface));
   _EGLSurface *surf = &vuSurf->base;

   if(buftype != EGL_OPENVG_IMAGE);
      goto cleanup;

   if(!_eglInitSurface(surf, disp, EGL_PBUFFER_BIT, conf, attrib_list, NULL))
      goto cleanup;

   SHImage *image  = (SHImage*)buffer;
   surf->Width  = image->width;
   surf->Height = image->height; 

   if(!_vuInitSurface(vuSurf, dev->logical.device, image->vkImageView, image->width, image->height))
      goto cleanup;

   return surf;

cleanup:
   free(vuSurf);
   return NULL;
}

EGLBoolean _prepareRendering(
   VkCommandBuffer cmdbuf,
   VuSurface      *vuSurf
){
   _EGLSurface* surf = &vuSurf->base;

   // Start recording commands
   VkCommandBufferBeginInfo begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
   };
   if(vkBeginCommandBuffer(cmdbuf, &begin_info) != VK_SUCCESS)
      return EGL_FALSE;

   // Begin render pass to make frame buffer as arendering target
   VkClearValue clear_values[2] = {
      { .color = { .float32 = {0.1, 0.1, 0.1, 255}, }, },
      { .depthStencil = { .depth = -1000, }, },
   };
   VkRenderPassBeginInfo pass_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass  = vuSurf->renderPass,
      .framebuffer = vuSurf->frameBuffer,
      .renderArea  = {
             .offset = { .x = 0,
                         .y = 0, },
             .extent = { .width  = surf->Width,
                         .height = surf->Height },
      },
      .clearValueCount = 2,
      .pClearValues = clear_values,
   };

   // TODO: Memory barrier
   // TODO: Pipline barrier

   // Start recording commands
   vkCmdBeginRenderPass(cmdbuf, &pass_info, VK_SUBPASS_CONTENTS_INLINE);

   // Viewport
   VkViewport viewport = {
      .x = 0,
      .y = 0,
      .width  = surf->Width,
      .height = surf->Height,
      .minDepth = 0,
      .maxDepth = 1,
   };
   vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
   
   // Scissor
   VkRect2D scissor = {
      .offset = { .x = 0, .y = 0, },
      .extent = {.width  = surf->Width,
                 .height = surf->Height },
   };
   vkCmdSetScissor(cmdbuf, 0, 1, &scissor);

   // Stop recording commands
   if(vkEndCommandBuffer(cmdbuf) != VK_SUCCESS)
      return EGL_FALSE;

   return EGL_TRUE;
}

#define COM_INDEX_PREPARE_RENDERING  0
EGLBoolean
_makeCurrent(
   _EGLDriver  *drv,
   _EGLDisplay *disp, 
   _EGLSurface *dsurf,
   _EGLSurface *rsurf, 
   _EGLContext *ctx
){
   _EGLContext *old_ctx;
   _EGLSurface *old_dsurf, *old_rsurf;
   VuSurface   *vuSurf = (VuSurface*)dsurf;
   
   if (!_eglBindContext(ctx, dsurf, rsurf, &old_ctx, &old_dsurf, &old_rsurf))
      return EGL_FALSE;
   
   _EGLDevice* dev = _getPrimaryDevice(disp);

   // Queue family must be a graphics queue family
   QueueFamily queueFamily = dev->logical.queue_families[0];
   assert(queueFamily.qflags & VK_QUEUE_GRAPHICS_BIT);

   // Select command buffer to prepare rendering
   VkCommandBuffer cmdbuf = queueFamily.buffers[COM_INDEX_PREPARE_RENDERING];
   vkResetCommandBuffer(cmdbuf, 0);

   // Record commands to prepare rendering
   if(!_prepareRendering(cmdbuf, vuSurf))
      return EGL_FALSE;

   return EGL_TRUE;
}


#define MAX_PRESENT_MODES 4
typedef struct
{
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	VkSurfaceFormatKHR surface_format;
	VkSurfaceCapabilitiesKHR surface_caps;
	VkPresentModeKHR present_modes[MAX_PRESENT_MODES];
	uint32_t present_modes_count;
} VCSwapchain;

typedef struct{
   VkImage     *images;
   uint32_t    image_count;
   uint32_t    width;
   uint32_t    height;
   VCSwapchain swapchain;
} VCNativeWindow;

EGLNativeWindowType
vcNativeCreateWindowFromSDL(EGLDisplay dpy, SDL_Window window)
{
   _EGLDisplay *disp = _eglLockDisplay(dpy);
   //TODO: Thread safe

   VCNativeWindow *nativeWin = (VCNativeWindow*)malloc(sizeof(VCNativeWindow));
   VCSwapchain    *swapchain = &nativeWin->swapchain;
   _EGLDevice     *dev       = _getPrimaryDevice(disp);

   // SDL info
   SDL_SysWMinfo wm;
   SDL_VERSION(&wm.version);
   SDL_GetWindowWMInfo(window, &wm);

   // Create surface
   VkSurfaceKHR surface;
   switch(wm.subsystem){
      case SDL_SYSWM_X11:
         VkXcbSurfaceCreateInfoKHR surface_info = {
            .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
            .connection = XGetXCBConnection(wm.info.x11.display),
            .window = wm.info.x11.window,
         };
         vkCreateXcbSurfaceKHR((VkInstance)disp->PlatformDisplay, &surface_info, NULL, &surface);
         break;
      default
         goto cleanup;
   }
   swapchain->surface = surface;

   // Surface format
   uint32_t surface_format_count = 1;
   vkGetPhysicalDeviceSurfaceFormatsKHR(dev->physical.device, swapchain->surface, &surface_format_count, &swapchain->surface_format);
   if (surface_format_count == 1 && swapchain->surface_format.format == VK_FORMAT_UNDEFINED)
      swapchain->surface_format.format = VK_FORMAT_R8G8B8_UNORM;

   // Present mode
   VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
   swapchain->present_modes_count = MAX_PRESENT_MODES;
   VkResult res;
   bool allow_no_vsync = false;
   res = vkGetPhysicalDeviceSurfacePresentModesKHR(dev->physical.device, swapchain->surface,
          &swapchain->present_modes_count, swapchain->present_modes);
   if (res >= 0) {
      for (uint32_t i = 0; i < swapchain->present_modes_count; ++i){
         if (allow_no_vsync && swapchain->present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR){
            present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            break;
         }
         if (!allow_no_vsync && swapchain->present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR){
            present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
         }
      }
   }

   // Swapchain image count
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev->physical.device, swapchain->surface, &swapchain->surface_caps);
   uint8_t  thread_count = 1;
   uint32_t min_image_count = swapchain->surface_caps.minImageCount + thread_count - 1;
   if (swapchain->surface_caps.maxImageCount < min_image_count && swapchain->surface_caps.maxImageCount != 0)
      min_image_count = swapchain->surface_caps.maxImageCount;

   // Create swapchain
   VkSwapchainCreateInfoKHR swapchain_info = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = swapchain->surface,
      .minImageCount = min_image_count,
      .imageFormat = swapchain->surface_format.format,
      .imageColorSpace = swapchain->surface_format.colorSpace,
      .imageExtent = swapchain->surface_caps.currentExtent.width == 0xFFFFFFFF ?
                        swapchain->surface_caps.minImageExtent :
                        swapchain->surface_caps.currentExtent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .preTransform = swapchain->surface_caps.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = present_mode,
      .clipped = true,
   };
   if(vkCreateSwapchainKHR(dev->logical.device, &swapchain_info, NULL, &swapchain->swapchain) != VK_SUCCESS)
      goto cleanup;

   // Query image num in the swapchain
   vkGetSwapchainImagesKHR(dev->logical.device, swapchain->swapchain, &nativeWin->image_count, NULL);

   // Get images in the swapcain
   nativeWin->images = malloc(nativeWin->image_count * sizeof(VkImage));
   if(vkGetSwapchainImagesKHR(dev->logical.device, swapchain->swapchain, &nativeWin->image_count, nativeWin->images) != VK_SUCCESS){
      free(nativeWin->images);
      goto cleanup;
   }

   nativeWin->width  = width;
   nativeWin->height = height;

   assert(sizeof(EGLNativeWindowType) == sizeof(void*));
   return (EGLNativeWindowType)nativeWin;

cleanup:
   free(nativeWin);
   return (EGLNativeWindowType)NULL;
}

EGLBoolean
vcNativeDestroyWindow(EGLNativeWindowType window)
{
   return EGL_FALSE;
}

_EGLDriver _eglDriver = {
   .Initialize                    = _Initialize,
   .Terminate                     = NULL,
   .CreateContext                 = _CreateContext,
   .DestroyContext                = NULL,
   .MakeCurrent                   = _makeCurrent,
   .CreateWindowSurface           = createWindowSurface,
   .CreatePixmapSurface           = NULL,
   .CreatePbufferSurface          = NULL,
   .CreatePbufferFromClientBuffer = _createPbufferFromClientBuffer,
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

