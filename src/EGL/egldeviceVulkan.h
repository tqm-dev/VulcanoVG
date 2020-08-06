/**************************************************************************
 *
 * Copyright 2015, 2018 Collabora
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#ifndef EGLDEVICEVULKAN_INCLUDED
#define EGLDEVICEVULKAN_INCLUDED


#include <stdbool.h>
#include <stddef.h>

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_QUEUE_FAMILY 10
typedef struct 
{
   VkPhysicalDevice                 physical_device;
   VkPhysicalDeviceProperties       properties;
   VkPhysicalDeviceFeatures         features;
   VkPhysicalDeviceMemoryProperties memories;
   
   VkQueueFamilyProperties          queue_families[MAX_QUEUE_FAMILY];
   uint32_t                         queue_family_count;
   bool                             queue_families_incomplete;
} PhysicalDevice;

typedef struct
{
   VkCommandPool    pool;

   VkQueueFlags     qflags;
   VkQueue*         queues;
   uint32_t         queue_count;
   VkCommandBuffer* buffers;
   uint32_t         buffer_count;
} CommandPool;

typedef struct {

   VkDevice         device;
   CommandPool*     command_pools;
   uint32_t         command_pool_count;

} LogicalDevice;

#ifdef __cplusplus
}
#endif

#endif /* EGLDEVICEVULKAN_INCLUDED */
