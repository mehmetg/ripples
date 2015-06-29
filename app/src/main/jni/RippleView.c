
#include "RippleView.h"
#include "RippleModel.h"

#if __ANDROID__
    #include <jni.h>
#endif

#include <stdlib.h>
#include <sys/time.h>


#if __ANDROID__
JNIEXPORT void JNICALL Java_com_kamcord_ripples_RippleLib_InitRipple(
    JNIEnv * env, jobject obj, jint width, jint height,
    jint textureWidth, jint textureHeight, jintArray pixels, jint versionEnum);
JNIEXPORT void JNICALL Java_com_kamcord_ripples_RippleLib_DestroyRipple(
    JNIEnv * env, jobject obj);
JNIEXPORT void JNICALL Java_com_kamcord_ripples_RippleLib_DrawRipple(
    JNIEnv * env, jobject obj, jint versionEnum);
JNIEXPORT void JNICALL Java_com_kamcord_ripples_RippleLib_TouchRipple(
    JNIEnv * env, jobject obj, jfloat x, jfloat y);
#endif

static GLint uniforms[NUM_UNIFORMS];

typedef struct RippleView {
    GLuint _program;

    GLuint _positionVBO;
    GLuint _texcoordVBO;
    GLuint _indexVBO;

    GLfloat* _texCoordsES11;
    size_t _texCoordsES11Size;

    float _screenWidth;
    float _screenHeight;
    size_t _textureWidth;
    size_t _textureHeight;
    float _meshFactor;

    bool _firstDraw;

    RippleModel *_ripple;

    // knobs to add redudancy to simulation to
    // slow it down to 30fps on target device.
    int _layers;
    int _timesteps;

    GLuint _rgbTexture;

    bool _recording;

} RippleView;

typedef void(*UpdateTexCoordsFunc)(RippleView*);
typedef void(*DrawFunc)(RippleView*);

static void setUpBuffers(RippleView* view);
static void setUpBuffersES11(RippleView* view);

static void setUpGL(RippleView* view);
static void setUpGLES11(RippleView* view);

static void tearDownGL(RippleView* view);
static void tearDownGLES11(RippleView* view);

static bool loadShaders(RippleView* view);
static bool compileShader(GLuint *shader, GLenum type, const char* code);
static bool linkProgram(GLuint prog);

static void (*setUpBuffersFunc)(RippleView*);
static void (*setUpGLFunc)(RippleView*);
static void (*tearDownGLFunc)(RippleView*);


void initRGBTexture(RippleView* view, int width, int height, int* pixelData)
{
    glActiveTexture(GL_TEXTURE2);

    glGenTextures(1, &view->_rgbTexture);

    glBindTexture(GL_TEXTURE_2D, view->_rgbTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    for( int i = 0; i < width * height; i++ )
    {
        int p = pixelData[i];
        int q = 0;

        // Byte order that java's bitmap class defaults to is different from GL_RGBA.
        // Need to swizzle.  Other 32-bit formats not allowed in OpenGL ES 2.

        q |= (p >> 24) & 0xff;
        q = q << 8;
        q |= (p >> 0) & 0xff;
        q = q << 8;
        q |= (p >> 8) & 0xff;
        q = q << 8;
        q |= (p >> 16) & 0xff;

        pixelData[i] = q;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
}

static void updateTexCoordsPointer(RippleView* view)
{
    // The texture coordinates in the ripple view are not in the same coordinates
    // as the one OpenGL expects, convert them here
    const unsigned int numBytes = getVertexSize(view->_ripple);
    const size_t numFloats = numBytes / sizeof(GLfloat);
    GLfloat* texCoords = getTexCoords(view->_ripple);

    if ( numFloats != view->_texCoordsES11Size )
    {
        // Have to reallocate (or allocate if first one)
        GLfloat* temp = realloc(view->_texCoordsES11, numBytes);
        view->_texCoordsES11 = temp;
        view->_texCoordsES11Size = numFloats;
    }

    // Now, loop over all floats, calculating the new texture coordinates
    for ( size_t i = 0; i < numFloats; ++i )
    {
        view->_texCoordsES11[i] = 1.0f - texCoords[i];
    }
    
    glTexCoordPointer(2, GL_FLOAT, 2 * sizeof(GLfloat), view->_texCoordsES11);
}


void setUpView(RippleView* view, int textureHeight, int textureWidth, int* pixelData, GLVersion version)
{
    view->_textureWidth = textureWidth;
    view->_textureHeight = textureHeight;

    if( view->_ripple )
        free(view->_ripple);

    view->_ripple = (RippleModel*)calloc(1, sizeof(RippleModel));

    InitRippleModel(
        view->_ripple,
        view->_screenWidth,
        view->_screenHeight,
        view->_meshFactor,
        10,
        view->_textureWidth,
        view->_textureHeight);

    if ( VERSION_OPENGL_ES_1_1 == version )
    {
        setUpGLFunc = &setUpGLES11;
        setUpBuffersFunc = &setUpBuffersES11;
        tearDownGLFunc = &tearDownGLES11;
    }
    else
    {
        setUpGLFunc = setUpGL;
        setUpBuffersFunc = setUpBuffers;
        tearDownGLFunc = tearDownGL;
    }
    
    setUpGLFunc(view);
    setUpBuffersFunc(view);
    initRGBTexture(view, textureWidth, textureHeight, pixelData);
}

void setUpBuffers(RippleView* view)
{
    glGenBuffers(1, &view->_indexVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, view->_indexVBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, getIndexSize(view->_ripple), getIndices(view->_ripple), GL_STATIC_DRAW);

    glGenBuffers(1, &view->_positionVBO);
    glBindBuffer(GL_ARRAY_BUFFER, view->_positionVBO);
    glBufferData(GL_ARRAY_BUFFER, getVertexSize(view->_ripple), getVertices(view->_ripple), GL_STATIC_DRAW);

    glEnableVertexAttribArray(ATTRIB_VERTEX);
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), 0);

    glGenBuffers(1, &view->_texcoordVBO);
    glBindBuffer(GL_ARRAY_BUFFER, view->_texcoordVBO);
    glBufferData(GL_ARRAY_BUFFER, getVertexSize(view->_ripple), getTexCoords(view->_ripple), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(ATTRIB_TEXCOORD);
    glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), 0);
}

void setUpBuffersES11(RippleView* view)
{
    glVertexPointer(2, GL_FLOAT, 2 * sizeof(GLfloat), getVertices(view->_ripple));
    updateTexCoordsPointer(view);
}

void setUpGL(RippleView* view)
{
    loadShaders(view);

    glUseProgram(view->_program);

    glDisable(GL_DEPTH_TEST);

    glUniform1i(uniforms[UNIFORM_RGB], 2);

    glViewport(0, 0, view->_screenWidth, view->_screenHeight);
}

void setUpGLES11(RippleView* view)
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_FOG);
    glDisable(GL_LIGHTING);

    glEnable(GL_TEXTURE_2D);

    glClientActiveTexture(GL_TEXTURE2);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glViewport(0, 0, view->_screenWidth, view->_screenHeight);
}

void tearDownGL(RippleView* view)
{
    if (view->_positionVBO)
    {
        glDeleteBuffers(1, &view->_positionVBO);
        view->_positionVBO = 0;
    }

    if (view->_texcoordVBO)
    {
        glDeleteBuffers(1, &view->_texcoordVBO);
        view->_texcoordVBO = 0;
    }

    if (view->_indexVBO)
    {
        glDeleteBuffers(1, &view->_indexVBO);
        view->_indexVBO = 0;
    }

    if (view->_program)
    {
        glDeleteProgram(view->_program);
        view->_program = 0;
    }

    if (view->_rgbTexture)
    {
        glDeleteTextures(1, &view->_rgbTexture);
        view->_rgbTexture = 0;
    }
}

void tearDownGLES11(RippleView* view)
{
    if (view->_rgbTexture)
    {
        glDeleteTextures(1, &view->_rgbTexture);
        view->_rgbTexture = 0;
    }
}

static void updateTexCoordsVBO(RippleView* view)
{
    glBufferData(GL_ARRAY_BUFFER,
		 getVertexSize(view->_ripple), getTexCoords(view->_ripple), GL_DYNAMIC_DRAW);
}

static void update(RippleView* view, UpdateTexCoordsFunc updateTexCoordsFunc)
{
    if (view->_ripple)
    {
        for( int i = 0; i < view->_timesteps; i++)
        {
            runSimulation(view->_ripple, 1.0f/view->_timesteps);
        }

	updateTexCoordsFunc(view);
    }
}

bool loadShaders(RippleView* view)
{
    GLuint vertShader = 0, fragShader = 0;

    // Create shader program.
    view->_program = glCreateProgram();

    const char* vertexShaderCode =
        "\n"
        "\n"
        "\n"
        "attribute vec4 position;\n"
        "attribute vec2 texCoord;\n"
        "\n"
        "varying vec2 texCoordVarying;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = position;\n"
        "    texCoordVarying = texCoord;\n"
        "}\n"
        "\n"
        "\n";

    const char* fragmentShaderCode =
        "\n"
        "uniform sampler2D SamplerRGB;\n"
        "\n"
        "varying highp vec2 texCoordVarying;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    lowp vec3 rgb;\n"
        "    lowp vec2 t = texCoordVarying.xy;\n"
        "\n"
        "    rgb = texture2D(SamplerRGB, vec2(1.0, 1.0) - t).rgb;\n"
        "    gl_FragColor = vec4(rgb, 1.0);\n"
        "}\n"
        "\n"
        "\n"
        "\n";

    if (!compileShader(&vertShader, GL_VERTEX_SHADER, vertexShaderCode)) {
        return false;
    }

    if (!compileShader(&fragShader, GL_FRAGMENT_SHADER, fragmentShaderCode)) {
        return false;
    }

    // Attach vertex shader to program.
    glAttachShader(view->_program, vertShader);

    // Attach fragment shader to program.
    glAttachShader(view->_program, fragShader);

    // Bind attribute locations.
    // This needs to be done prior to linking.
    glBindAttribLocation(view->_program, ATTRIB_VERTEX, "position");
    glBindAttribLocation(view->_program, ATTRIB_TEXCOORD, "texCoord");

    // Link program.
    if (!linkProgram(view->_program)) {

        if (vertShader) {
            glDeleteShader(vertShader);
            vertShader = 0;
        }
        if (fragShader) {
            glDeleteShader(fragShader);
            fragShader = 0;
        }
        if (view->_program) {
            glDeleteProgram(view->_program);
            view->_program = 0;
        }

        return false;
    }

    // Get uniform locations.
    uniforms[UNIFORM_RGB] = glGetUniformLocation(view->_program, "SamplerRGB");

    // Release vertex and fragment shaders.
    if (vertShader) {
        glDetachShader(view->_program, vertShader);
        glDeleteShader(vertShader);
    }
    if (fragShader) {
        glDetachShader(view->_program, fragShader);
        glDeleteShader(fragShader);
    }

    return true;
}

bool compileShader(GLuint *shader, GLenum type, const char* code)
{
    GLint status;
    const GLchar *source = code;

    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);

    GLint logLength;
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength+1);
        glGetShaderInfoLog(*shader, logLength, &logLength, log);
        free(log);
    }

    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        glDeleteShader(*shader);
        return false;
    }

    return true;
}

bool linkProgram(GLuint prog)
{
    GLint status;
    glLinkProgram(prog);

    GLint logLength;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if ( logLength > 0 )
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        
        free(log);
    }

    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if ( status == 0 )
    {
        return false;
    }

    return true;
}


static double now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return 0.000001 * tv.tv_usec + 1.0 * tv.tv_sec;
}

static void drawElements(RippleView* view)
{
    glDrawElements(GL_TRIANGLES, getIndexCount(view->_ripple), GL_UNSIGNED_SHORT, 0);
}

static void drawElementsES11(RippleView* view)
{
    glDrawElements(GL_TRIANGLES, getIndexCount(view->_ripple), GL_UNSIGNED_SHORT, getIndices(view->_ripple));
}

static void drawView(RippleView* view, UpdateTexCoordsFunc texCoordUpdateFunc, DrawFunc drawFunc)
{
    static double then = 0;
    double t = now();
    then = t;

    update(view, texCoordUpdateFunc);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for( int i = 0; i < view->_layers; i++ )
    {
        drawFunc(view);
    }
}

RippleView* __rippleView = NULL;

void RippleLib_InitRipple(
    int width, int height,
    int textureWidth, int textureHeight,
    int *pixelData,
    GLVersion version)
{
    if( __rippleView )
        free( __rippleView );

    __rippleView = (RippleView*)calloc(1, sizeof(RippleView));

    __rippleView->_screenWidth = width;
    __rippleView->_screenHeight = height;
    __rippleView->_meshFactor = 8;

    __rippleView->_layers = 1;
    __rippleView->_timesteps = 1;

    setUpView(__rippleView, textureWidth, textureHeight, pixelData, version);
}

void RippleLib_DestroyRipple()
{
    if( __rippleView )
    {
        if( __rippleView->_ripple )
            DestroyRippleModel( __rippleView->_ripple );

        if( __rippleView->_ripple )
        {
            free(__rippleView->_ripple);
            __rippleView->_ripple = NULL;
        }

	if ( __rippleView->_texCoordsES11 )
	{
	    free(__rippleView->_texCoordsES11);
	    __rippleView->_texCoordsES11 = NULL;
	}


        tearDownGLFunc( __rippleView );
        free( __rippleView );
    }

    __rippleView = NULL;
}

void RippleLib_DrawRipple(GLVersion version)
{
    if ( VERSION_OPENGL_ES_1_1 == version )
    {
        drawView(__rippleView, &updateTexCoordsPointer, &drawElementsES11);
    }
    else
    {
        drawView(__rippleView, &updateTexCoordsVBO, &drawElements);
    }
}

void RippleLib_TouchRipple(float x, float y)
{
    initiateRippleAtLocation(__rippleView->_ripple, x, y);
}


#if __ANDROID__

JNIEXPORT void JNICALL Java_com_kamcord_ripples_RippleLib_InitRipple(
    JNIEnv * env, jobject obj, jint width, jint height,
    jint textureWidth, jint textureHeight, jintArray pixels,
    jint versionEnum)
{
    int *pixelData = (*env)->GetIntArrayElements(env, pixels, 0);
    RippleLib_InitRipple(width, height, textureWidth, textureHeight, pixelData, (GLVersion)versionEnum);
    (*env)->ReleaseIntArrayElements(env, pixels, pixelData, 0);
}

JNIEXPORT void JNICALL Java_com_kamcord_ripples_RippleLib_DestroyRipple(
    JNIEnv * env, jobject obj)
{
    RippleLib_DestroyRipple();
}

JNIEXPORT void JNICALL Java_com_kamcord_ripples_RippleLib_DrawRipple(
    JNIEnv * env, jobject obj, jint versionEnum)
{
    RippleLib_DrawRipple((GLVersion)versionEnum);
}

JNIEXPORT void JNICALL Java_com_kamcord_ripples_RippleLib_TouchRipple(
    JNIEnv * env, jobject obj, jfloat x, jfloat y)
{
    initiateRippleAtLocation(__rippleView->_ripple, x, y);
}

#endif
