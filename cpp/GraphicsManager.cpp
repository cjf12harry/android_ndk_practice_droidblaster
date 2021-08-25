//
// Created by cjf12 on 2019-10-10.
//

#include "include/GraphicsManager.h"
#include "include/Log.h"
#include "Libraries/libpng/png.h"
#include <stdio.h>
#include <string.h>

GraphicsManager::GraphicsManager(android_app *pApplication) :
        mApplication(pApplication),
        mRenderWidth(0), mRenderHeight(0),
        mDisplay(EGL_NO_DISPLAY), mSurface(EGL_NO_CONTEXT),
        mContext(EGL_NO_SURFACE),
        mProjectionMatrix(),
        mTextures(),
        mShaders(),
        mVertexBuffers(),
        mComponents(),
        mScreenFrameBuffer(0),
        mRenderFrameBuffer(0), mRenderVertexBuffer(0),
        mRenderTexture(0), mRenderShaderProgram(0),
        aPosition(0), aTexture(0),
        uProjection(0), uTexture(0) {
    Log::info("Creating GraphicsManager.");
}

GraphicsManager::~GraphicsManager() {
    Log::info("Destroying GraphicsManager.");
}

void GraphicsManager::registerComponent(GraphicsComponent *pComponent) {
    mComponents.push_back(pComponent);
}

status GraphicsManager::start() {
    Log::info("Starting GraphicsManager.");
    EGLint format, numCOnfigs, errorResult;
    GLenum status;
    EGLConfig config;

    //Defines display requirements. 16bits mode here.
    const EGLint DISPLAY_ATTRIBS[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_BLUE_SIZE, 5, EGL_GREEN_SIZE, 6, EGL_RED_SIZE, 5,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_NONE
    };

    //Request an OpenGL ES 2 context.
    const EGLint CONTEXT_ATTRIBS[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
    };

    Log::info("Connecting to display.");
    mDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mDisplay == EGL_NO_DISPLAY) goto ERROR;
    if (eglInitialize(mDisplay, NULL, NULL) != EGL_TRUE) goto ERROR;

    if (eglChooseConfig(mDisplay, DISPLAY_ATTRIBS, &config, 1, &numCOnfigs) != EGL_TRUE ||
        (numCOnfigs <= 0))
        goto ERROR;
    if (!eglGetConfigAttrib(mDisplay, config, EGL_NATIVE_VISUAL_ID, &format)) goto ERROR;

    ANativeWindow_Buffer windowBuffer;
    if (ANativeWindow_setBuffersGeometry(mApplication->window, 0, 0,
                                         format) < 0) {
        Log::error("Error while setting buffer geometry.");
        return STATUS_KO;
    }

    mSurface = eglCreateWindowSurface(mDisplay, config, mApplication->window, NULL);
    if (mSurface == EGL_NO_SURFACE) goto ERROR;
    mContext = eglCreateContext(mDisplay, config, NULL, CONTEXT_ATTRIBS);
    if (mContext == EGL_NO_CONTEXT) goto ERROR;

    if (eglMakeCurrent(mDisplay, mSurface, mSurface, mContext) != EGL_TRUE ||
        eglQuerySurface(mDisplay, mSurface, EGL_WIDTH, &mScreenWidth) != EGL_TRUE ||
        eglQuerySurface(mDisplay, mSurface, EGL_HEIGHT, &mScreenHeight) != EGL_TRUE ||
        (mScreenWidth <= 0) || (mScreenHeight <= 0))
        goto ERROR;

    //Defines and initializes offscreen surface.
    if (initializeRenderBuffer() != STATUS_OK) goto ERROR;

    glViewport(0, 0, mRenderWidth, mRenderHeight);
    glDisable(GL_DEPTH_TEST);

    //Prepares the projection matrix with viewport dimensions.
    memset(mProjectionMatrix[0], 0, sizeof(mProjectionMatrix));
    mProjectionMatrix[0][0] = 2.0f / GLfloat(mRenderWidth);
    mProjectionMatrix[1][1] = 2.0f / GLfloat(mRenderHeight);
    mProjectionMatrix[2][2] = -1.0f;
    mProjectionMatrix[3][0] = -1.0f;
    mProjectionMatrix[3][1] = -1.0f;
    mProjectionMatrix[3][2] = 0.0f;
    mProjectionMatrix[3][3] = 1.0f;

    Log::info("Starting GraphicsManager");
    Log::info("Version   : %s", glGetString(GL_VERSION));
    Log::info("Vendor    : %s", glGetString(GL_VENDOR));
    Log::info("Renderer  : %s", glGetString(GL_RENDERER));
    Log::info("Offscreen : %d x %d", mRenderWidth, mRenderHeight);

    //Loads graphics components.
    for (std::vector<GraphicsComponent *>::iterator componentIt = mComponents.begin();
         componentIt < mComponents.end();
         ++componentIt) {
        if ((*componentIt)->load() != STATUS_OK) return STATUS_KO;
    }
    return STATUS_OK;

    ERROR:
    Log::error("Error while starting GraphicsManager");
    stop();
    return STATUS_KO;

}

void GraphicsManager::stop() {
    Log::info("Stopping GraphicsManager");
    // Releases textures.
    std::map<Resource *, TextureProperties>::iterator textureIt;
    for (textureIt = mTextures.begin(); textureIt != mTextures.end(); ++textureIt) {
        glDeleteTextures(1, &textureIt->second.texture);
    }

    // Release shaders.
    std::vector<GLuint>::iterator shaderIt;
    for (shaderIt = mShaders.begin();
         shaderIt < mShaders.end();
         ++shaderIt) {
        glDeleteProgram(*shaderIt);
    }
    mShaders.clear();

    // Release vertex buffers.
    std::vector<GLuint>::iterator vertexBufferIt;
    for (vertexBufferIt = mVertexBuffers.begin();
         vertexBufferIt < mVertexBuffers.end();
         ++vertexBufferIt) {
        glDeleteBuffers(1, &(*vertexBufferIt));
    }
    mVertexBuffers.clear();

   if (mRenderFrameBuffer != 0) {
        glDeleteFramebuffers(1, &mRenderFrameBuffer);
        mRenderFrameBuffer = 0;
    }

    if (mRenderTexture != 0) {
        glDeleteTextures(1, &mRenderTexture);
        mRenderTexture = 0;
    }

    //Destroys OpenGL context.
    if (mDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (EGL_NO_CONTEXT != mContext) {
            eglDestroyContext(mDisplay, mContext);
            mContext = EGL_NO_CONTEXT;
        }

        if (EGL_NO_SURFACE != mSurface) {
            eglDestroySurface(mDisplay, mSurface);
            mSurface = EGL_NO_SURFACE;
        }

        eglTerminate(mDisplay);
        mDisplay = EGL_NO_DISPLAY;
    }


}

status GraphicsManager::update() {
    glBindFramebuffer(GL_FRAMEBUFFER,
                      mRenderFrameBuffer); // glBindFramebuffer binds the framebuffer object with name framebuffer to the framebuffer target specified by target.
    glViewport(0, 0, mRenderWidth,
               mRenderHeight); // glViewport specifies the affine transformation of x and y from normalized device coordinates to window coordinates.
    glClear(GL_COLOR_BUFFER_BIT); // clear the bit that indicates the buffers currently enabled for color writing.

    // Render graphic components.
    std::vector<GraphicsComponent*>::iterator componentIt;
    for (componentIt = mComponents.begin();
    componentIt < mComponents.end();
    ++componentIt){
        (*componentIt)->draw();
    }

    // The FBO is rendered and scaled into the screen.
    glBindFramebuffer(GL_FRAMEBUFFER, mScreenFrameBuffer);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, mScreenWidth, mScreenHeight);

    glActiveTexture(
            GL_TEXTURE0); // glActiveTexture selects which texture unit subsequent texture state calls will affect. The number of texture units an implementation supports is implementation dependent, but must be at least 80.
    glBindTexture(GL_TEXTURE_2D,
                  mRenderTexture); // glBindTexture lets you create or use a named texture.
    glUseProgram(mRenderShaderProgram);
    glUniform1i(uTexture, 0);

    //Indicates to OpenGL how position and uv coordinates are stored
    glBindBuffer(GL_ARRAY_BUFFER, mRenderVertexBuffer);
    glEnableVertexAttribArray(aPosition);
    glVertexAttribPointer(aPosition, //Attribute Index
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(RenderVertex),
                          (GLvoid *) 0);
    glEnableVertexAttribArray(aTexture);
    glVertexAttribPointer(aTexture,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(RenderVertex),
                          (GLvoid *) (sizeof(GLfloat) * 2));

    glDrawArrays(GL_TRIANGLE_STRIP, 0,
                 4); //glDrawArrays specifies multiple geometric primitives with very few subroutine calls.
    //When glDrawArrays is called, it uses count sequential elements from each enabled array to construct a sequence of geometric primitives, beginning with element first. mode specifies what kind of primitives are constructed and how the array elements construct those primitives.
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (eglSwapBuffers(mDisplay, mSurface) !=
        EGL_TRUE) { //post EGL surface color buffer to a native window
        Log::error("Error %d swapping buffers.", eglGetError());
        return STATUS_KO;
    } else {
        return STATUS_OK;
    }
}

void callback_readPng(png_structp pStruct, png_bytep pData, png_size_t pSize) {
    Resource *resource = ((Resource *) png_get_io_ptr(pStruct));
    if (resource->read(pData, pSize) != STATUS_OK) {
        resource->close();
    }
}

TextureProperties *GraphicsManager::loadTexture(Resource &pResource) {
    //Looks for the texture in cache first.
    std::map<Resource*, TextureProperties>::iterator textureIt = mTextures.find(&pResource);

    if (textureIt != mTextures.end()) {
        return &textureIt->second;
    }

    Log::info("Loading texture %s", pResource.getPath());

    TextureProperties *textureProperties;
    GLuint texture;
    GLint format;
    png_byte header[8];
    png_structp pngPtr = NULL;
    png_infop infoPtr = NULL;
    png_byte *image = NULL;
    png_bytep *rowPtrs = NULL;
    png_int_32 rowSize;
    bool transparency;

    if (pResource.open() != STATUS_OK) goto ERROR;
    Log::info("Checking signature.");
    if (pResource.read(header, sizeof(header)) != STATUS_OK) goto ERROR;
    if (png_sig_cmp(header, 0, 8) != 0)
        goto ERROR;  //png_sig_cmp() checks whether the given number of bytes match the PNG signature starting from the start position. The function shall return non-zero if num_to_check == 0 or start > 7.

    Log::info("Creating required structures.");
    pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL,
                                    NULL); // allocate and initialize a png_struct structure for reading PNG file
    if (!pngPtr) goto ERROR;
    infoPtr = png_create_info_struct(pngPtr); //allocate and initialize a png_info structure
    if (!infoPtr) goto ERROR;

    //prepare reading operation by setting up a read callback.
    png_set_read_fn(pngPtr, &pResource,
                    callback_readPng);  //set user-defined function for reading a PNG stream. png_set_read_fn() sets the read_data_fn as the input function for reading PNG files instead of using standard C I/O stream functions. png_ptr - pointer to input data structure png_struct io_ptr - pointer to user-defined structure containing information about the input functions. This value may be NULL. read_data_fn - pointer to new input function that shall take the following arguments: - a pointer to a png_struct - a pointer to a structure where input data can be stored - 32-bit unsigned int to indicate number of bytes to read The input function should invoke png_error() to handle any fatal errors and png_warning() to handle non-fatal errors.
    //Set up error management. If an error occurs while reading.
    // code will come back here and jump
    if (setjmp(png_jmpbuf(pngPtr))) goto ERROR;

    //Ignores first 8 bytes already read.
    png_set_sig_bytes(pngPtr,
                      8); //png_set_sig_bytes() shall store the number of bytes of the PNG file signature that have been read from the PNG stream.

    // Retrieved PNG info and updates PNG struct accordingly.
    png_int_32 depth, colorType;
    png_uint_32 width, height;
    png_read_info(pngPtr,
                  infoPtr); //Reads the information before the actual image data from the PNG file. The function allows reading a file that already has the PNG signature bytes read from the stream.
    png_get_IHDR(pngPtr, infoPtr, &width, &height, &depth, &colorType, NULL, NULL,
                 NULL); //png_get_IHDR() gets PNG_IHDR chunk type information from png_info structure.

    //Creates a full alpha channel if transparency is encoded as
    //an array of palette entries or a single transparent color.
    transparency = false;
    if (png_get_valid(pngPtr, infoPtr,
                      PNG_INFO_tRNS)) { //determine if given chunk data is valid (PNG_INFO_tRNS means transparency data for images)
        png_set_tRNS_to_alpha(
                pngPtr); // png_set_tRNS_to_alpha() shall set transformation in png_ptr such that tRNS chunks are expanded to alpha channels.
        transparency = true;
    }

    //Expands PNG with less than 8bits per channel to 8 bits.
    if (depth < 8) {
        png_set_packing(
                pngPtr); //png_set_packing() shall expand image to 1 pixel per byte for bit-depths 1, 2 and 4 without changing the order of the pixels. If png_set_packing() is not called, PNG files pack pixels of bit_depths 1, 2 and 4 into bytes as small as possible, for example, 8 pixels per byte for 1-bit files.
        //Shrinks PNG with 16 bits per color channel down to 8 bits.
    } else if (depth == 16) {
        png_set_strip_16(
                pngPtr); //png_set_strip_16() shall strip the pixels of a PNG stream with 16 bits per channel to 8 bits per channel.
    }

    //Indicates that image needs conversion to TGBA if needed.
    switch (colorType) {
        case PNG_COLOR_TYPE_PALETTE:
            png_set_palette_to_rgb(
                    pngPtr); //png_set_palette_to_rgb() shall set transformation in png_ptr such that paletted images are expanded to RGB
            format = transparency ? GL_RGBA : GL_RGB;
            break;
        case PNG_COLOR_TYPE_RGB:
            format = transparency ? GL_RGBA : GL_RGB;
            break;
        case PNG_COLOR_TYPE_RGBA:
            format = GL_RGBA;
            break;
        case PNG_COLOR_TYPE_GRAY:
            png_set_expand_gray_1_2_4_to_8(
                    pngPtr); // As of libpng version 1.2.9, png_set_expand_gray_1_2_4_to_8() was added.  It expands the sample depth without changing tRNS to alpha.
            format = transparency ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
            break;
        case PNG_COLOR_TYPE_GA:
            png_set_expand_gray_1_2_4_to_8(pngPtr);
            format = GL_LUMINANCE_ALPHA;
            break;
    }

    //Validates all transformations.
    png_read_update_info(pngPtr,
                         infoPtr); //png_read_update_info() updates the structure pointed to by info_ptr to reflect any transformations that have been requested. For example, row bytes will be updated to handle expansion of an interlaced image with png_read_update_info().

    //Get row size in bytes.
    rowSize = png_get_rowbytes(pngPtr, infoPtr); //Return number of bytes for a row
    if (rowSize <= 0) goto ERROR;
    //Creates the image buffer that will be sent to OpenGL.
    image = new png_byte[rowSize * height];
    if (!image) goto ERROR;
    //Pointers to each row of the image buffer. Row order is
    // inverted becase different coordinate system are used by
    //OpenGL (1st pixel is at bottom left) and PNGs(top left).
    rowPtrs = new png_bytep[height];
    if (!rowPtrs) goto ERROR;
    for (int i = 0; i < height; ++i) {
        rowPtrs[height - (i + 1)] = image + i * rowSize; //the index is reversed?!
    }

    //Reads image content.
    png_read_image(pngPtr, rowPtrs); // read the entire image into memory
    //Free memory and resources.
    pResource.close();
    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
    delete[] rowPtrs; //free the memory associated with read png_struct

    GLenum errorResult;
    glGenTextures(1,
                  &texture); //glGenTextures returns n texture names in textures. There is no guarantee that the names form a contiguous set of integers; however, it is guaranteed that none of the returned names was in use immediately before the call to glGenTextures.
    //The generated textures have no dimensionality; they assume the dimensionality of the texture target to which they are first bound (see glBindTexture).
    //Texture names returned by a call to glGenTextures are not returned by subsequent calls, unless they are first deleted with glDeleteTextures.
    glBindTexture(GL_TEXTURE_2D, texture); //glBindTexture lets you create or use a named texture
    //Set-up texture properties
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST); //The texture minifying function is used whenever the
    // level-of-detail function used when sampling from
    // the texture determines that the texture should be
    // minified. There are six defined minifying functions.
    // Two of them use either the nearest texture elements or
    // a weighted average of multiple texture elements to compute
    // the texture value. The other four use mipmaps.

    //GL_NEAREST :Returns the value of the texture element that is nearest (in Manhattan distance) to the specified texture coordinates.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // The texture magnification function is used whenever the level-of-detail
    // function used when sampling from the texture determines that the texture
    // should be magified. It sets the texture magnification function to either
    // GL_NEAREST or GL_LINEAR (see below). GL_NEAREST is generally faster than
    // GL_LINEAR, but it can produce textured images with sharper edges because
    // the transition between texture elements is not as smooth. The initial value
    // of GL_TEXTURE_MAG_FILTER is GL_LINEAR.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //Sets the wrap parameter for texture coordinate s to either GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, or GL_REPEAT.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Sets the wrap parameter for texture coordinate s to either GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, or GL_REPEAT.

    //Load iamge data into OpenGL.
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE,
                 image); //specify a two-dimensional texture image
    //Finished working with the texture.
    glBindTexture(GL_TEXTURE_2D,
                  0); //bind a named texture to a texturing target , 0 is the texture name
    delete[] image;
    if (glGetError() != GL_NO_ERROR) goto ERROR;
    Log::info("Texture size: %d x %d", width, height);

    //Caches the loaded texture.
    textureProperties = &mTextures[&pResource];
    textureProperties->texture = texture;
    textureProperties->width = width;
    textureProperties->height = height;
    return textureProperties;

    ERROR:
    Log::error("Error loading texture into OpenGL.");
    pResource.close();
    delete[] rowPtrs;
    delete[] image;
    if (pngPtr != NULL) {
        png_infop *infoPtrP = infoPtr != NULL ? &infoPtr : NULL;
        png_destroy_read_struct(&pngPtr, infoPtrP, NULL);
    }
    return NULL;
}

GLuint GraphicsManager::loadShader(const char *pVertexShader, const char *pFragmentShader) {
    GLint result;
    char log[256];
    GLuint vertexShader, fragmentShader, shaderProgram;

    //Builds the vertex shader.
    vertexShader = glCreateShader(GL_VERTEX_SHADER); //Create a shader
    glShaderSource(vertexShader, 1, &pVertexShader, NULL); //set the shader source code.
    glCompileShader(vertexShader); //Compile the shader program
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS,
                  &result); //get the information about shader compilation
    if (result == GL_FALSE) {
        glGetShaderInfoLog(vertexShader, sizeof(log), 0, log);
        Log::error("Vertex shader error: %s", log);
        goto ERROR;
    }

    //Builds the fragment shader
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); //create a fragment shader
    glShaderSource(fragmentShader, 1, &pFragmentShader, NULL); //set the shader source code
    glCompileShader(fragmentShader); //compile the shader program
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS,
                  &result); //get the information about shader compilation
    if (result == GL_FALSE) {
        glGetShaderInfoLog(fragmentShader, sizeof(log), 0, log);
        Log::error("Fragment shader error: %s", log);
        goto ERROR;
    }

    shaderProgram = glCreateProgram(); //create a program object where shader objects can be attached to .
    glAttachShader(shaderProgram,
                   vertexShader); //attach a shader object to the program, to indicate what operations need to be completed. A shader can be attached to a program even before its source code is set or compiled.
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(
            shaderProgram); //produce executable of a shader program to specific hardware, like graphic processor, fragment processor etc.
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &result); //get the status of a program
    glDeleteShader(
            vertexShader);//delete the allocated shader object. Here the share is flagged as deletable, however, it is not deleted until the shader is detached.
    glDeleteShader(fragmentShader);
    if (result == GL_FALSE) {
        glGetProgramInfoLog(shaderProgram, sizeof(log), 0, log);
        Log::error("Shader program error: %s", log);
        goto ERROR;
    }

    mShaders.push_back(shaderProgram);
    return shaderProgram;

    ERROR:
    Log::error("Error loading shader");
    if (vertexShader > 0) glDeleteShader(vertexShader);
    if (fragmentShader > 0) glDeleteShader(fragmentShader);
    return 0;
}

GLuint GraphicsManager::loadVertexBuffer(const void *pVertexBuffer, int32_t pVertexBufferSize) {
    GLuint vertexBuffer;
    //Upload specified memory buffer into OpenGL;
    glGenBuffers(1,
                 &vertexBuffer); //returns n buffer object names in buffers. There is no guarantee that the names form a contiguous set of integers; however, it is guaranteed that none of the returned names was in use immediately before the call to glGenBuffers.
    // Buffer object names returned by a call to glGenBuffers are not returned by subsequent calls, unless they are first deleted with glDeleteBuffers.
    //No buffer objects are associated with the returned buffer object names until they are first bound by calling glBindBuffer.


    glBindBuffer(GL_ARRAY_BUFFER,
                 vertexBuffer); //glBindBuffer binds a buffer object to the specified buffer binding point. Calling glBindBuffer with target set to one of the accepted symbolic constants and buffer set to the name of a buffer object binds that buffer object name to the target. If no buffer object with name buffer exists, one is created with that name. When a buffer object is bound to a target, the previous binding for that target is automatically broken.
    glBufferData(GL_ARRAY_BUFFER, pVertexBufferSize, pVertexBuffer,
                 GL_STATIC_DRAW);// Copy data from source to the data store pointed at the ARRAY_BUFFER.
    // DYNAMIC
    //The data store contents will be modified repeatedly and used many times.
    // DRAW
    //The data store contents are modified by the application, and used as the source for GL drawing and image specification commands.
    //Unbinds the buffer.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (glGetError() != GL_NO_ERROR) goto ERROR;

    mVertexBuffers.push_back(vertexBuffer);
    return vertexBuffer;

    ERROR:
    Log::error("Error loading vertex buffer.");
    if (vertexBuffer > 0) glDeleteBuffers(1, &vertexBuffer);
    return 0;
}

static const char *VERTEX_SHADER =
        "attribute vec2 aPosition;\n"
        "attribute vec2 aTexture;\n"
        "varying vec2 vTexture;\n"
        "void main() {\n"
        "   vTexture = aTexture;\n"
        "   gl_Position = vec4(aPosition, 1.0, 1.0);\n"
        "}\n";

static const char *FRAGMENT_SHADER =
        "precision mediump float;"
        "uniform sampler2D uTexture;\n"
        "varying vec2 vTexture;\n"
        "void main()  {\n"
        "   gl_FragColor = texture2D(uTexture, vTexture);\n"
        "}\n";

const int32_t DEFAULT_RENDER_WIDTH = 600;

status GraphicsManager::initializeRenderBuffer() {
    Log::info("Loading offscreen buffer");
    const RenderVertex vertices[] = {
            {-1.0f, -1.0f, 0.0f, 0.0f},
            {-1.0f, 1.0f,  0.0f, 1.0f},
            {1.0f,  -1.0f, 1.0f, 0.0f},
            {1.0f,  1.0f,  1.0f, 1.0f}
    };

    float screenRatio = float(mScreenHeight) / float(mScreenWidth);
    mRenderWidth = DEFAULT_RENDER_WIDTH;
    mRenderHeight = float(mRenderWidth) * screenRatio;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mScreenFrameBuffer); //get the bounded frame buffer
    glGenTextures(1, &mRenderTexture); //glGenTextures returns n texture names in textures.
    glBindTexture(GL_TEXTURE_2D, mRenderTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mRenderWidth, mRenderHeight, 0, GL_RGB,
                 GL_UNSIGNED_SHORT_5_6_5,
                 NULL); //data may be a null pointer. In this case, texture memory is allocated to accommodate a texture of width width and height height. You can then download subtextures to initialize this texture memory. The image is undefined if the user tries to apply an uninitialized portion of the texture image to a primitive.

    glGenFramebuffers(1,
                      &mRenderFrameBuffer); //glGenFramebuffers returns n framebuffer object names in ids.
    glBindFramebuffer(GL_FRAMEBUFFER,
                      mRenderFrameBuffer); //glBindFramebuffer binds the framebuffer object with name framebuffer to the framebuffer target specified by target.
    // Calling glBindFramebuffer with target set to GL_FRAMEBUFFER binds framebuffer to both the read and draw framebuffer targets
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mRenderTexture,
                           0); //glFramebufferTexture2D attaches the texture image specified by texture and level as one of the logical buffers of the currently bound framebuffer object. attachment specifies whether the texture image should be attached to the framebuffer object's color, depth, or stencil buffer. A texture image may not be attached to the default framebuffer object name 0.
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    mRenderVertexBuffer = loadVertexBuffer(vertices, sizeof(vertices));
    if (mRenderVertexBuffer == 0) goto ERROR;

    mRenderShaderProgram = loadShader(VERTEX_SHADER, FRAGMENT_SHADER);
    if (mRenderShaderProgram == 0) goto ERROR;
    aPosition = glGetAttribLocation(mRenderShaderProgram, "aPosition");
    aTexture = glGetAttribLocation(mRenderShaderProgram, "aTexture");
    uTexture = glGetUniformLocation(mRenderShaderProgram, "uTexture");
    return STATUS_OK;

    ERROR:
    Log::error("Error while loading offscreen buffer");
    return STATUS_KO;
}
