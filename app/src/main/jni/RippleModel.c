
#include "RippleModel.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

void initRippleMap(RippleModel* model)
{
    // +2 for padding the border
    memset(model->rippleSource, 0, (model->poolWidth+2)*(model->poolHeight+2)*sizeof(float));
    memset(model->rippleDest, 0, (model->poolWidth+2)*(model->poolHeight+2)*sizeof(float));
}

void initRippleCoeff(RippleModel* model)
{
    for (int y=0; y<=2*model->touchRadius; y++)
    {
        for (int x=0; x<=2*model->touchRadius; x++)
        {
            float distance = sqrt((x-model->touchRadius)*(x-model->touchRadius)+
            	(y-model->touchRadius)*(y-model->touchRadius));

            if (distance <= model->touchRadius)
            {
                float factor = (distance/model->touchRadius);

                // goes from -512 -> 0
                model->rippleCoeff[y*(model->touchRadius*2+1)+x] = -(cos(factor*M_PI)+1.f) * 256.f;
            }
            else
            {
                model->rippleCoeff[y*(model->touchRadius*2+1)+x] = 0.f;
            }
        }
    }
}

void initMesh(RippleModel* model)
{
    for (int j=0; j<model->poolHeight; j++)
	for (int i=0; i<model->poolWidth; i++)
	{
		int index = (j*model->poolWidth+i)*2;
		model->rippleVertices[index+0] = -1.f + i*(2.f/(model->poolWidth-1));
		model->rippleVertices[index+1] = -1.f + j*(2.f/(model->poolHeight-1));

		model->rippleTexCoords[index+0] = (float)i/(model->poolWidth-1);
		model->rippleTexCoords[index+1] = (float)j/(model->poolHeight-1);
	}

    unsigned int index = 0;

    for (int j=0; j<model->poolHeight-1; j++)
	for (int i=0; i<model->poolWidth-1; i++)
	{
		model->rippleIndices[index] = i + j*model->poolWidth;
		index++;

		model->rippleIndices[index] = (i+1) + j*model->poolWidth;
		index++;

		model->rippleIndices[index] = (i+1) + (j+1)*model->poolWidth;
		index++;


		model->rippleIndices[index] = i + j*model->poolWidth;
		index++;

		model->rippleIndices[index] = (i+1) + (j+1)*model->poolWidth;
		index++;

		model->rippleIndices[index] = i + (j+1)*model->poolWidth;
		index++;
    }

}

GLfloat* getVertices(RippleModel* model)
{
    return model->rippleVertices;
}

GLfloat* getTexCoords(RippleModel* model)
{
    return model->rippleTexCoords;
}

GLushort* getIndices(RippleModel* model)
{
    return model->rippleIndices;
}

unsigned int getVertexSize(RippleModel* model)
{
    return model->poolWidth*model->poolHeight*2*sizeof(GLfloat);
}

unsigned int getIndexSize(RippleModel* model)
{
    return getIndexCount(model) * sizeof(GLushort);
}

unsigned int getIndexCount(RippleModel* model)
{
    return 6 * (model->poolHeight-1) * (model->poolWidth-1);
}

void freeBuffers(RippleModel* model)
{
	if( model->rippleCoeff )
	{
    	free(model->rippleCoeff);
    	model->rippleCoeff = NULL;
    }

    if( model->rippleSource )
	{
    	free(model->rippleSource);
    	model->rippleSource = NULL;
    }

	if( model->rippleDest )
	{
    	free(model->rippleDest);
    	model->rippleDest = NULL;
    }

	if( model->rippleVertices )
	{
    	free(model->rippleVertices);
    	model->rippleVertices = NULL;
    }

	if( model->rippleTexCoords )
	{
    	free(model->rippleTexCoords);
    	model->rippleTexCoords = NULL;
    }

	if( model->rippleIndices )
	{
    	free(model->rippleIndices);
    	model->rippleIndices = NULL;
    }
}

void InitRippleModel(RippleModel* model,
	unsigned int width,
    unsigned int height,
	float meshFactor,
	unsigned int radius,
	unsigned int texWidth,
	unsigned int texHeight)
{
	model->screenWidth = width;
	model->screenHeight = height;
	model->poolWidth = width/meshFactor;
	model->poolHeight = height/meshFactor;
	model->touchRadius = radius;

	freeBuffers(model);

	model->rippleCoeff = (float *)malloc((model->touchRadius*2+1)*(model->touchRadius*2+1)*sizeof(float));

	// +2 for padding the border
	model->rippleSource = (float *)malloc((model->poolWidth+2)*(model->poolHeight+2)*sizeof(float));
	model->rippleDest = (float *)malloc((model->poolWidth+2)*(model->poolHeight+2)*sizeof(float));

	model->rippleVertices = (GLfloat *)malloc(getVertexSize(model));
	model->rippleTexCoords = (GLfloat *)malloc(getVertexSize(model));
	model->rippleIndices = (GLushort *)malloc(getIndexSize(model));

	if (!model->rippleCoeff || !model->rippleSource || !model->rippleDest ||
		!model->rippleVertices || !model->rippleTexCoords || !model->rippleIndices)
	{
		freeBuffers(model);
		exit(0);
	}

	initRippleMap(model);
	initRippleCoeff(model);
	initMesh(model);
}

void runSimulation(RippleModel* model, float deltat)
{
    float k = pow(0.9, deltat);

    for (int j=0; j<model->poolHeight; j++)
	for (int i=0; i<model->poolWidth; i++)
    {
        // * - denotes current pixel
        //
        //       a
        //     c * d
        //       b

        // +1 to both x/y values because the border is padded
        float a = model->rippleSource[(j)*(model->poolWidth+2) + i+1];
        float b = model->rippleSource[(j+2)*(model->poolWidth+2) + i+1];
        float c = model->rippleSource[(j+1)*(model->poolWidth+2) + i];
        float d = model->rippleSource[(j+1)*(model->poolWidth+2) + i+2];

        float v = model->rippleSource[(j+1)*(model->poolWidth+2) + i+1];
        float deltat = 0.5;
        int index = (j+1)*(model->poolWidth+2);
        float result = 2.0*deltat*((a + b + c + d)/4.f - v) + 2*v - model->rippleDest[index + i + 1];
        result*=k;

        model->rippleDest[index + i+1] = result;
    }

    double scalek = 1.0 * model->screenWidth / model->screenHeight;

    for (int j=0; j<model->poolHeight; j++)
	for (int i=0; i<model->poolWidth; i++)
    {
        // * - denotes current pixel
        //
        //       a
        //     c * d
        //       b

        // +1 to both x/y values because the border is padded
        float a = model->rippleDest[j*(model->poolWidth+2) + i+1];
        float b = model->rippleDest[(j+2)*(model->poolWidth+2) + i+1];
        float c = model->rippleDest[(j+1)*(model->poolWidth+2) + i];
        float d = model->rippleDest[(j+1)*(model->poolWidth+2) + i+2];

        float s_offset = ((c - d) / 3000.f);
        float t_offset = ((b - a) / 3000.f);

        // clamp
        s_offset = (s_offset < -0.5f) ? -0.5f : s_offset;
        t_offset = (t_offset < -0.5f) ? -0.5f : t_offset;
        s_offset = (s_offset > 0.5f) ? 0.5f : s_offset;
        t_offset = (t_offset > 0.5f) ? 0.5f : t_offset;

        float s_tc = (float)i/(model->poolWidth-1);
        float t_tc = (float)j/(model->poolHeight-1);

        int index = (j*model->poolWidth+i)*2;
        model->rippleTexCoords[index+0] = scalek * ((s_tc + s_offset)-0.5) + 0.5;
        model->rippleTexCoords[index+1] = t_tc + t_offset;
    }

    float *pTmp = model->rippleDest;
    model->rippleDest = model->rippleSource;
    model->rippleSource = pTmp;
}

void initiateRippleAtLocation(RippleModel* model, float x, float y)
{
    unsigned int xIndex = (unsigned int)((x / model->screenWidth) * model->poolWidth);
    unsigned int yIndex = (unsigned int)((1.0 - (y / model->screenHeight)) * model->poolHeight);

    for (int y=(int)yIndex-(int)model->touchRadius; y<=(int)yIndex+(int)model->touchRadius; y++)
    {
        for (int x=(int)xIndex-(int)model->touchRadius; x<=(int)xIndex+(int)model->touchRadius; x++)
        {
            if (x>=0 && x<model->poolWidth &&
                y>=0 && y<model->poolHeight)
            {
                // +1 to both x/y values because the border is padded
                model->rippleSource[(model->poolWidth+2)*(y+1)+x+1] +=
                    model->rippleCoeff
                        [
                        ( y - (yIndex-model->touchRadius))*(model->touchRadius*2+1) + x - (xIndex-model->touchRadius)
                        ];
            }
        }
    }

    // model->rippleSource[(model->poolWidth+2) * (yIndex+1) + xIndex + 1] = 1000.0f;
}

void DestroyRippleModel(RippleModel* model)
{
    freeBuffers(model);
}

