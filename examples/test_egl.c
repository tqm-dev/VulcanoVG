#include "test.h"
#include <assert.h>
#include <vulkan/vulkan.h>

#define SURFACELESS_PLATFORM 1

#if SURFACELESS_PLATFORM == 1
static VkInstance _createVkInstance(void);
#endif

int main(int argc, char **argv)
{
	EGLDisplay dpy;
	EGLConfig config;
	EGLContext ctx;
	EGLBoolean ret;
	EGLint major, minor;

    testInit(argc, argv, 600,600, "VulkanVG: Dummy");

#if SURFACELESS_PLATFORM
	VkInstance vkInstance = _createVkInstance();
	printf("_createVkInstance(): [%p] \r\n",  vkInstance);
	dpy = eglGetDisplay((EGLNativeDisplayType)vkInstance);
#else
	dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
#endif
	printf("eglGetDisplay(): [%p] \r\n", dpy);

	ret = eglInitialize(dpy, &major, &minor);
	printf("eglInitialize(): [%d] \r\n", ret);

	{
		EGLint num_config;
		EGLint attrib_list[3] = {
#if SURFACELESS_PLATFORM
			EGL_CONFIG_ID, EGL_CONFIG_ID_VULKAN_VG_SURFACELESS,
#else
			EGL_CONFIG_ID, EGL_CONFIG_ID_VULKAN_VG,
#endif
			EGL_NONE
		};
		ret = eglChooseConfig(dpy, attrib_list, &config, 1, &num_config);
		printf("eglChooseConfig(): [%d] \r\n", ret);
		printf("config: [%lu] \r\n", (uintptr_t)config);
		assert(num_config == 1);
	}

	ret = eglBindAPI(EGL_OPENVG_API);
	printf("eglBindAPI(): [%d] \r\n", ret);

	ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, NULL);
	printf("eglCreateContext(): [%p] \r\n", ctx);

	ret = eglDestroyContext(dpy, ctx);
	printf("eglDestroyContext(): [%d] \r\n", ret);

	ret = eglTerminate(dpy);
	printf("eglTerminate(): [%d] \r\n", ret);

	return EXIT_SUCCESS;
}

static VkInstance _createVkInstance(void)
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

