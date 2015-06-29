
#ifndef _RIPPLE_DEMO_
#define _RIPPLE_DEMO_

#if __ANDROID__
#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include <stdbool.h>
#endif

#ifdef __APPLE_CC__
#include <glut/glut.h>
#endif

// Uniform index.
enum
{
    UNIFORM_RGB,
    NUM_UNIFORMS
};

// Attribute index.
enum
{
    ATTRIB_VERTEX,
    ATTRIB_TEXCOORD,
    NUM_ATTRIBUTES
};

typedef enum GLVersion
{
    VERSION_OPENGL_ES_1_1,
    VERSION_OPENGL_ES_2_0
} GLVersion;

void KamcordLib_InitRipple(
	int width, int height,
	int textureWidth, int textureHeight,
	int *pixelData,
	GLVersion version);

void KamcordLib_DestroyRipple();

void KamcordLib_DrawRipple(GLVersion version);
void KamcordLib_TouchRipple(float x, float y);

#endif

