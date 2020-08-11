
#ifndef __SHEXTENSIONS_H
#define __SHEXTENSIONS_H

/* Define missing constants and route missing
   functions to extension pointers */

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

#ifndef GL_VERSION_1_2
#  define GL_BGRA                          0x80E1
#  define GL_UNSIGNED_SHORT_5_6_5          0x8363
#  define GL_UNSIGNED_SHORT_4_4_4_4        0x8033
#  define GL_UNSIGNED_SHORT_4_4_4_4_REV    0x8365
#  define GL_UNSIGNED_SHORT_5_5_5_1        0x8034
#  define GL_UNSIGNED_SHORT_1_5_5_5_REV    0x8366
#  define GL_UNSIGNED_INT_8_8_8_8          0x8035
#  define GL_UNSIGNED_INT_8_8_8_8_REV      0x8367
#  define GL_CLAMP_TO_EDGE                 0x812F
#endif

#ifndef GL_VERSION_1_3
#  define GL_MULTISAMPLE                   0x809D
#  define GL_TEXTURE0                      0x84C0
#  define GL_TEXTURE1                      0x84C1
#  define GL_CLAMP_TO_BORDER               0x812D
#  define glActiveTexture                  context->pglActiveTexture
#  define glMultiTexCoord1f                context->pglMultiTexCoord1f
#  define glMultiTexCoord2f                context->pglMultiTexCoord2f
#endif

#ifndef GL_VERSION_1_4
#  define GL_MIRRORED_REPEAT               0x8370
#endif

typedef void (APIENTRYP SH_PGLACTIVETEXTURE) (GLenum);
typedef void (APIENTRYP SH_PGLMULTITEXCOORD1F) (GLenum, GLfloat);
typedef void (APIENTRYP SH_PGLMULTITEXCOORD2F) (GLenum, GLfloat, GLfloat);

#endif
