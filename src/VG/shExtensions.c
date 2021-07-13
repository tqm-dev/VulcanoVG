
#define VG_API_EXPORT
#include <VG/openvg.h>
#include "shDefs.h"
#include "shExtensions.h"
#include "shContext.h"
#include <stdio.h>
#include <string.h>

/*-----------------------------------------------------
 * Extensions check
 *-----------------------------------------------------*/

void fallbackActiveTexture(GLenum texture) {
}

void fallbackMultiTexCoord1f(GLenum target, GLfloat x) {
#if RENDERING_ENGINE == OPENGL_1
  glTexCoord1f(x);
#endif
}

void fallbackMultiTexCoord2f(GLenum target, GLfloat x, GLfloat y) {
#if RENDERING_ENGINE == OPENGL_1
  glTexCoord2f(x, y);
#endif
}

static int checkExtension(const char *extensions, const char *name)
{
	int nlen = (int)strlen(name);
	int elen = (int)strlen(extensions);
	const char *e = extensions;
	SH_ASSERT(nlen > 0);

	while (1) {

		/* Try to find sub-string */
		e = strstr(e, name);
    if (e == NULL) return 0;
		/* Check if last */
		if (e == extensions + elen - nlen)
			return 1;
		/* Check if space follows (avoid same names with a suffix) */
    if (*(e + nlen) == ' ')
      return 1;
    
    e += nlen;
	}

  return 0;
}

typedef void (*PFVOID)();

PFVOID shGetProcAddress(const char *name)
{
  #if defined(_WIN32)
  return (PFVOID)wglGetProcAddress(name);
  #elif defined(__APPLE__)
  /* TODO: Mac OS glGetProcAddress implementation */
  return (PFVOID)NULL;
  #elif RENDERING_ENGINE == OPENGL_ES2
  return (PFVOID)NULL;
  #else
  return (PFVOID)glXGetProcAddress((const unsigned char *)name);
  #endif
}

void shLoadExtensions(VGContext *c)
{
#if RENDERING_ENGINE != OPENGL_1
    c->isGLAvailable_ClampToEdge = 0;
    c->isGLAvailable_MirroredRepeat = 0;
    c->isGLAvailable_Multitexture = 0;
    c->pglActiveTexture = (SH_PGLACTIVETEXTURE)fallbackActiveTexture;
    c->pglMultiTexCoord1f = (SH_PGLMULTITEXCOORD1F)fallbackMultiTexCoord1f;
    c->pglMultiTexCoord2f = (SH_PGLMULTITEXCOORD2F)fallbackMultiTexCoord2f;
    c->isGLAvailable_TextureNonPowerOfTwo = 0;
#else
  const char *ext = (const char*)glGetString(GL_EXTENSIONS);
  
  /* GL_TEXTURE_CLAMP_TO_EDGE */
  if (checkExtension(ext, "GL_EXT_texture_edge_clamp"))
    c->isGLAvailable_ClampToEdge = 1;
  else if (checkExtension(ext, "GL_SGIS_texture_edge_clamp"))
    c->isGLAvailable_ClampToEdge = 1;
  else /* Unavailable */
    c->isGLAvailable_ClampToEdge = 0;
  
  
  /* GL_TEXTURE_MIRRORED_REPEAT */
  if (checkExtension(ext, "GL_ARB_texture_mirrored_repeat"))
    c->isGLAvailable_MirroredRepeat = 1;
  else if(checkExtension(ext, "GL_IBM_texture_mirrored_repeat"))
    c->isGLAvailable_MirroredRepeat = 1;
  else /* Unavailable */
    c->isGLAvailable_MirroredRepeat = 0;
  
  
  /* glActiveTexture, glMultiTexCoord1f */  
  if (checkExtension(ext, "GL_ARB_multitexture")) {    
    c->isGLAvailable_Multitexture = 1;
    
    c->pglActiveTexture = (SH_PGLACTIVETEXTURE)
      shGetProcAddress("glActiveTextureARB");
    c->pglMultiTexCoord1f = (SH_PGLMULTITEXCOORD1F)
      shGetProcAddress("glMultiTexCoord1fARB");
    c->pglMultiTexCoord2f = (SH_PGLMULTITEXCOORD2F)
      shGetProcAddress("glMultiTexCoord2fARB");
    
    if (c->pglActiveTexture == NULL || c->pglMultiTexCoord1f == NULL ||
        c->pglMultiTexCoord2f == NULL)
      c->isGLAvailable_Multitexture = 0;
    
  }else{ /* Unavailable */
    c->isGLAvailable_Multitexture = 0;
    c->pglActiveTexture = (SH_PGLACTIVETEXTURE)fallbackActiveTexture;
    c->pglMultiTexCoord1f = (SH_PGLMULTITEXCOORD1F)fallbackMultiTexCoord1f;
    c->pglMultiTexCoord2f = (SH_PGLMULTITEXCOORD2F)fallbackMultiTexCoord2f;
  }
  
  /* Non-power-of-two textures */
  if (checkExtension(ext, "GL_ARB_texture_non_power_of_two"))
    c->isGLAvailable_TextureNonPowerOfTwo = 1;
  else /* Unavailable */
    c->isGLAvailable_TextureNonPowerOfTwo = 0;
#endif
}
