
#include "egl.h"
#include "vuContext.h"
#include <string.h>
#include <stdio.h>

#define VU_EGL_DEFAULT_DISPLAY		EGL_CAST(EGLDisplay,1)
#define VU_EGL_DEFAULT_CONFIG_ID	0xff
static VGContext* _p_egl_context_for_vg = NULL;
static VkInstance _vkInstance = VK_NULL_HANDLE;

EGLAPI EGLBoolean EGLAPIENTRY eglInitialize (EGLDisplay dpy, EGLint *major, EGLint *minor)
{
	VkApplicationInfo appInfo;
	VkInstanceCreateInfo createInfo;
	VkInstance vkInstance;
	VkResult result;

	if(dpy != VU_EGL_DEFAULT_DISPLAY)
		return EGL_FALSE;

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
		return EGL_FALSE;

	_vkInstance = vkInstance;

	/* Update the EGL version numbers */
	if(major) *major = 1;
	if(minor) *minor = 2;
	
	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglMakeCurrent (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
	if(dpy != VU_EGL_DEFAULT_DISPLAY)
		return EGL_FALSE;

	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglQueryContext (EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
{
	if(dpy != VU_EGL_DEFAULT_DISPLAY)
		return EGL_FALSE;

	return EGL_TRUE;
}
EGLAPI const char *EGLAPIENTRY eglQueryString (EGLDisplay dpy, EGLint name)
{
	if(dpy != VU_EGL_DEFAULT_DISPLAY)
		return NULL;

	return NULL;
}

EGLAPI EGLBoolean EGLAPIENTRY eglQuerySurface (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
	if(dpy != VU_EGL_DEFAULT_DISPLAY)
		return EGL_FALSE;

	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglSwapBuffers (EGLDisplay dpy, EGLSurface surface)
{
	if(dpy != VU_EGL_DEFAULT_DISPLAY)
		return EGL_FALSE;

	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglTerminate (EGLDisplay dpy)
{
	if(dpy != VU_EGL_DEFAULT_DISPLAY)
		return EGL_FALSE;

	if(_vkInstance == VK_NULL_HANDLE)
		return EGL_FALSE;

	vkDestroyInstance(_vkInstance, NULL);
	_vkInstance = VK_NULL_HANDLE;

	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglWaitGL (void)
{
	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglWaitNative (EGLint engine)
{
	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglChooseConfig (EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
	if(dpy != VU_EGL_DEFAULT_DISPLAY)
		return EGL_FALSE;

	if(attrib_list == NULL)
		return EGL_FALSE;

	/* EGL_CONFIG_ID must be set to VU_EGL_DEFAULT_CONFIG_ID */
	{
		EGLint config_id = 0;
		uint8_t i;

		for(i = 0; attrib_list[i] != EGL_NONE; i += 2)
			if(attrib_list[i] == EGL_CONFIG_ID)
				config_id = attrib_list[i+1];

		if(config_id == VU_EGL_DEFAULT_CONFIG_ID){
			if(configs)    configs[0] = EGL_CAST(EGLConfig,VU_EGL_DEFAULT_CONFIG_ID);
			if(num_config) *num_config = 1;
		} else {
			if(num_config) *num_config = 0;
		}
	}

	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglCopyBuffers (EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target)
{
	if(dpy != VU_EGL_DEFAULT_DISPLAY)
		return EGL_FALSE;

	return EGL_TRUE;
}

EGLAPI EGLContext EGLAPIENTRY eglCreateContext (EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list)
{
	if(dpy != VU_EGL_DEFAULT_DISPLAY)
		return EGL_NO_CONTEXT;

	if(config != EGL_CAST(EGLConfig,VU_EGL_DEFAULT_CONFIG_ID))
		return EGL_NO_CONTEXT;

	/* Share context is not supported yet */
	if(share_context != EGL_NO_CONTEXT)
		return EGL_NO_CONTEXT;

	/* Attrib is not supported yet */
	if(attrib_list != NULL)
		return EGL_NO_CONTEXT;

	/* return if already created */
	if(_p_egl_context_for_vg)
		return EGL_NO_CONTEXT;
  
	/* create new context */
	SH_NEWOBJ(VGContext, _p_egl_context_for_vg);
	if (_p_egl_context_for_vg == NULL)
		return EGL_NO_CONTEXT;

	return EGL_CAST(EGLContext,_p_egl_context_for_vg);
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePbufferSurface (EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
	if(dpy != VU_EGL_DEFAULT_DISPLAY)
		return EGL_NO_SURFACE;

	return EGL_NO_SURFACE;
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePixmapSurface (EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list)
{
	if(dpy != VU_EGL_DEFAULT_DISPLAY)
		return EGL_NO_SURFACE;

	return EGL_NO_SURFACE;
}

EGLAPI EGLSurface EGLAPIENTRY eglCreateWindowSurface (EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list)
{
	if(dpy != VU_EGL_DEFAULT_DISPLAY)
		return EGL_NO_SURFACE;

	return EGL_NO_SURFACE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroyContext (EGLDisplay dpy, EGLContext ctx)
{
	if(dpy != VU_EGL_DEFAULT_DISPLAY)
		return EGL_FALSE;

	if(ctx == EGL_NO_CONTEXT)
		return EGL_FALSE;

	SH_DELETEOBJ(VGContext, ctx);

	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroySurface (EGLDisplay dpy, EGLSurface surface)
{
	if(dpy != VU_EGL_DEFAULT_DISPLAY)
		return EGL_FALSE;

	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigAttrib (EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
	if(dpy != VU_EGL_DEFAULT_DISPLAY)
		return EGL_FALSE;

	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigs (EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
	if(dpy != VU_EGL_DEFAULT_DISPLAY)
		return EGL_FALSE;

	return EGL_TRUE;
}

EGLAPI EGLDisplay EGLAPIENTRY eglGetCurrentDisplay (void)
{
	return VU_EGL_DEFAULT_DISPLAY;
}

EGLAPI EGLSurface EGLAPIENTRY eglGetCurrentSurface (EGLint readdraw)
{
	return EGL_NO_SURFACE;
}

EGLAPI EGLDisplay EGLAPIENTRY eglGetDisplay (EGLNativeDisplayType display_id)
{
	if(display_id != EGL_DEFAULT_DISPLAY)
		return EGL_NO_DISPLAY;

	return VU_EGL_DEFAULT_DISPLAY;
}

EGLAPI EGLint EGLAPIENTRY eglGetError (void)
{
	return EGL_NONE;
}

