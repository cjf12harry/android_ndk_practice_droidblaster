//
// Created by cjf12 on 2019-10-14.
//

#include "include/StarField.h"
#include "include/Log.h"

StarField::StarField(android_app *pApplication, TimeManager &pTimeManager,
                     GraphicsManager &pGraphicsManager, int32_t pStaCount,
                     Resource &pTextureResource) :
        mTimeManager(pTimeManager),
        mGraphicsManager(pGraphicsManager),
        mStarCount(pStaCount),
        mTextureResource(pTextureResource),
        mVertexBuffer(0), mTexture(-1), mShaderProgram(0),
        aPosition(-1),
        uProjection(-1), uHeight(-1), uTime(-1), uTexture(-1) {
    mGraphicsManager.registerComponent(this);
}

static const char *VERTEX_SHADER =
        "attribute vec4 aPosition;\n"
        "uniform mat4 uProjection;\n"
        "uniform float uHeight;\n"
        "uniform float uTime;\n"
        "void main() {\n"
        "   const float speed = -800.0;\n"
        "   const float size = 8.0;\n"
        "   vec4 position = aPosition;\n"
        "   position.x = aPosition.x;\n"
        "   position.y = mod(aPosition.y + (uTime * speed * aPosition.z),"
        "                                                       uHeight);\n"
        "   position.z = 0.0;\n"
        "   gl_Position = uProjection * position;\n"
        "   gl_PointSize = aPosition.z * size;"
        "}";

static const char *FRAGMENT_SHADER =
        "precision mediump float;\n"
        "uniform sampler2D uTexture;\n"
        "void main() {\n"
        "       gl_FragColor = texture2D(uTexture, gl_PointCoord);\n"
        "}";

status StarField::load() {
    Log::info("Loading star field.");
    TextureProperties *textureProperties;

    //Allocates a temporary buffer and populate it with point data:
    //1 vertices composed of 3 floats (X/Y/Z) per point.
    Vertex *vertexBuffer = new Vertex[mStarCount];
    for (int i = 0; i < mStarCount; ++i) {
        vertexBuffer[i].x = RAND(mGraphicsManager.getRenderWidth());
        vertexBuffer[i].y = RAND(mGraphicsManager.getRenderHeight());
        vertexBuffer[i].z = RAND(1.0f);
    }

    //Loads the vertex buffer into OpenGL.
    mVertexBuffer = mGraphicsManager.loadVertexBuffer(
            (uint8_t *) vertexBuffer, mStarCount * sizeof(Vertex));
    delete[] vertexBuffer;
    if (mVertexBuffer == 0) goto ERROR;

    textureProperties = mGraphicsManager.loadTexture(mTextureResource);
    if (textureProperties == NULL) goto ERROR;
    mTexture = textureProperties->texture;

    //Creates and retrieves shader attributes and uniforms.
    mShaderProgram = mGraphicsManager.loadShader(VERTEX_SHADER, FRAGMENT_SHADER);
    if (mShaderProgram == 0) goto ERROR;
    aPosition = glGetAttribLocation(mShaderProgram, "aPosition");
    uProjection = glGetUniformLocation(mShaderProgram, "uProjection");
    uHeight = glGetUniformLocation(mShaderProgram, "uHeight");
    uTime = glGetUniformLocation(mShaderProgram, "uTime");
    uTexture = glGetUniformLocation(mShaderProgram, "uTexture");
    return STATUS_OK;

    ERROR:
    Log::error("Error loading starfield");
    return STATUS_KO;
}

void StarField::draw() {
    glDisable(GL_BLEND);
    //Selects the vertex buffer and indicates how data is stored.
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    glEnableVertexAttribArray(aPosition);
    glVertexAttribPointer(aPosition, //atrribute index
                          3,// Number of components
                          GL_FLOAT, //Data type
                          GL_FALSE, //Normalized
                          3 * sizeof(GLfloat), //Stride
                          (GLvoid *) 0);

    //Selects the texture.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexture);

    //Selects the shader and passes parameters.
    glUseProgram(mShaderProgram);
    glUniformMatrix4fv(uProjection, 1, GL_FALSE,
            mGraphicsManager.getProjectionMatrix());
    glUniform1f(uHeight, mGraphicsManager.getRenderHeight());
    glUniform1f(uTime, mTimeManager.elapsedTotal());
    glUniform1i(uTexture, 0);

    //Renders the star field.
    glDrawArrays(GL_POINTS, 0, mStarCount);

    //Restores device state.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}
