
#ifndef _VULCANVG_H
#define _VULCANVG_H

#include "openvg.h"
#include <vulkan/vulkan.h>

#ifdef __cplusplus 
extern "C" { 
#endif

/* Images */
VG_API_CALL VGImage vgCreateImageFromVkImageEXT(
   VGImageFormat format,
   VGint width,
   VGint height,
   VGbitfield allowedQuality,
   VkImage image);

#ifdef __cplusplus 
} /* extern "C" */
#endif

#endif /* _VULCANVG_H */
