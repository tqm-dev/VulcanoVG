#include "test.h"
#include <assert.h>

#include <vulkan/vulkan.h>
#include <VG/openvg.h>
#include <VG/vulcanvg.h>
#include <EGL/vulcanegl.h>

static EGLDisplay createVkInstanceAsEGLDisplay(void);

#define SURF_WIDTH		100
#define SURF_HEIGHT		100
int main(int argc, char **argv)
{

	EGLDisplay	egldisplay;
	EGLConfig	eglconfig;
	EGLSurface	eglsurface;
	EGLContext	eglcontext;
	static const EGLint s_configAttribs[] =
	{
		EGL_RED_SIZE,		8,
		EGL_GREEN_SIZE, 	8,
		EGL_BLUE_SIZE,		8,
		EGL_ALPHA_SIZE, 	8,
		EGL_LUMINANCE_SIZE, EGL_DONT_CARE,			//EGL_DONT_CARE
		EGL_SURFACE_TYPE,	EGL_WINDOW_BIT,
		EGL_SAMPLES,		1,
		EGL_NONE
	};
	EGLint numconfigs;

	egldisplay = createVkInstanceAsEGLDisplay();
	assert(egldisplay != EGL_NO_DISPLAY);
	eglInitialize(egldisplay, NULL, NULL);
	assert(eglGetError() == EGL_SUCCESS);
	eglBindAPI(EGL_OPENVG_API);

	eglChooseConfig(egldisplay, s_configAttribs, &eglconfig, 1, &numconfigs);
	assert(eglGetError() == EGL_SUCCESS);
	assert(numconfigs == 1);

	eglsurface = eglCreateWindowSurface(egldisplay, eglconfig, 0, NULL);
	assert(eglGetError() == EGL_SUCCESS);
	eglcontext = eglCreateContext(egldisplay, eglconfig, NULL, NULL);
	assert(eglGetError() == EGL_SUCCESS);
	eglMakeCurrent(egldisplay, eglsurface, eglsurface, eglcontext);
	assert(eglGetError() == EGL_SUCCESS);

	vgClear(0, 0, SURF_WIDTH, SURF_HEIGHT);
	eglSwapBuffers(egldisplay, eglsurface);

	eglDestroySurface(egldisplay, eglsurface);
	eglDestroyContext(egldisplay, eglcontext);
	eglTerminate(egldisplay);

	return EXIT_SUCCESS;
}

static EGLDisplay createVkInstanceAsEGLDisplay(void){

	VkApplicationInfo appInfo;
	VkInstanceCreateInfo createInfo;
	VkInstance vkInstance;

	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "OpenVG Tiger sample";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = NULL;
	createInfo.enabledExtensionCount = 0;
	createInfo.ppEnabledExtensionNames = NULL;

	if(vkCreateInstance(&createInfo, NULL, &vkInstance) != VK_SUCCESS)
		return EGL_NO_DISPLAY;

	return (EGLDisplay)vkInstance;
}

