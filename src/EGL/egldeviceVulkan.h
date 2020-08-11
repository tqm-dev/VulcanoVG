
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
} QueueFamily;

typedef struct {
   VkDevice         device;
   QueueFamily*     queue_families;
   uint32_t         command_pool_count;
} LogicalDevice;

#ifdef __cplusplus
}
#endif

#endif /* EGLDEVICEVULKAN_INCLUDED */
