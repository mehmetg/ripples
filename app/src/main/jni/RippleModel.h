
#ifndef _RIPPLE_MODEL_
#define _RIPPLE_MODEL_

#if __ANDROID__
#include <GLES2/gl2.h>
#include <stdbool.h>
#endif

#ifdef __APPLE_CC__
#include <glut/glut.h>
#endif



typedef struct RippleModel {
    unsigned int screenWidth;
    unsigned int screenHeight;
    unsigned int poolWidth;
    unsigned int poolHeight;
    unsigned int resolutionScaleFactor;
    unsigned int touchRadius;
    
    // ripple coefficients
    float *rippleCoeff;
    
    // ripple simulation buffers
    float *rippleSource;
    float *rippleDest;
    
    // data passed to GL
    GLfloat *rippleVertices;
    GLfloat *rippleTexCoords;
    GLushort *rippleIndices;
} RippleModel;

void InitRippleModel(RippleModel* model,
	unsigned int width,
    unsigned int height,
	float meshFactor,
	unsigned int touchRadius,
	unsigned int texWidth,
	unsigned int texHeight);

void DestroyRippleModel(RippleModel* model);

void runSimulation(RippleModel* model, float deltat);

void initiateRippleAtLocation(RippleModel* model, float x, float y);

GLfloat* getVertices(RippleModel* model);
GLfloat* getTexCoords(RippleModel* model);
GLushort* getIndices(RippleModel* model);

unsigned int getVertexSize(RippleModel* model);
unsigned int getIndexSize(RippleModel* model);
unsigned int getIndexCount(RippleModel* model);

#endif

