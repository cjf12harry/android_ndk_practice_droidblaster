//
// Created by cjf12 on 2019-10-14.
//

#include "include/SpriteBatch.h"
#include "include/Log.h"
#include <GLES2/gl2.h>

SpriteBatch::SpriteBatch(TimeManager &pTimeManager, GraphicsManager &pGraphicsManager)
        : mTimeManager(pTimeManager),
          mGraphicsManager(pGraphicsManager),
          mSprites(),
          mVertices(),
          mIndexes(),
          mShaderProgram(0),
          aPosition(-1), aTexture(-1), uProjection(-1), uTexture(-1) {
    mGraphicsManager.registerComponent(this);
}

SpriteBatch::~SpriteBatch() {
    std::vector<Sprite*>::iterator spriteIt;
    for (spriteIt = mSprites.begin(); spriteIt < mSprites.end();
    ++spriteIt){
        delete(*spriteIt);
    }
}

Sprite *SpriteBatch::registerSprite(Resource &pTextureResource,
                                    int32_t pHeight, int32_t pWidth) {
    int32_t spriteCount = mSprites.size();
    int32_t index = spriteCount * 4;
    //Precomputes the index buffer. Each sprite is formed by
    //2 trianglar vertices, each with 3 indexes, so there are
    // 6 indexes
    mIndexes.push_back(index+0);
    mIndexes.push_back(index+1);
    mIndexes.push_back(index+2);
    mIndexes.push_back(index+2);
    mIndexes.push_back(index+1);
    mIndexes.push_back(index+3);
    for (int i = 0; i < 4; ++i) {
        mVertices.push_back(Sprite::Vertex());
    }

    //Appends a new sprite to the sprite array.
    mSprites.push_back(new Sprite(mGraphicsManager, pTextureResource, pHeight, pWidth));
    return mSprites.back();
}

static const char *VERTEX_SHADER =
        "attribute vec4 aPosition;\n" //The attribute qualifier is effectively equivalent to an input qualifier in vertex shaders. It cannot be used in any other shader stage. It cannot be used in interface blocks. The qualifier attribute is used to define a read-only variable that contains per-vertex information, such as colour and position, in the vertex shader. attribute cannot be used within the fragment shader.
        "attribute vec2 aTexture;\n" //vectors in openGL is column vectors
        "varying vec2 vTexture;\n"  //The varying qualifier is equivalent to the input of a fragment shader or the output of a vertex shader. It cannot be used in any other shader stages. It cannot be used in interface blocks.
        "uniform mat4 uProjection;\n" //Global variables and Interface Blocks can be declared with the uniform qualifier. This means that the value does not change between multiple executions of a shader during the rendering of a primitive
        "void main(){\n"
        "   vTexture = aTexture;\n"
        "   gl_Position = uProjection * aPosition;\n" //gl_Position is a global output variable in shader program. The projection matrix transform a coordinate from model view -> world view -> camera view. The MVP matrix http://www.opengl-tutorial.org/beginners-tutorials/tutorial-3-matrices/ .
        "}";

static const char *FRAGMENT_SHADER =
        "precision mediump float;\n"  // define the precision specification of the entire program
        "varying vec2 vTexture;\n" // The qualifier varying are used to define variables that can pass values from the vertex shader to the fragment shader.
        "uniform sampler2D u_texture;\n"
        "void main() {\n"
        "   gl_FragColor = texture2D(u_texture, vTexture);\n" //Sample the color of u_texture at location vTexture. Before calling the program, caller code needs to load the texture and bind the texture into texture2D. gl_BindTexture.
        "}";

status SpriteBatch::load() {

    GLint result;
    int32_t spriteCount;
    mShaderProgram = mGraphicsManager.loadShader(VERTEX_SHADER, FRAGMENT_SHADER); //convert the shader into executable.
    if (mShaderProgram == 0) return STATUS_KO;
    aPosition = glGetAttribLocation(mShaderProgram, "aPosition"); //Get a handle to a variable in the shader program. The handle can be set by glBindAttribLocation. But only go into effect when the program is linked.
    aTexture = glGetAttribLocation(mShaderProgram, "aTexture");
    uProjection = glGetUniformLocation(mShaderProgram, "uProjection"); //Get a handle to a uniform variable in the shader program.
                                                                       //A uniform variable is constant throughout the program,
                                                                       //but it is changeable by external program via glUniform()
                                                                       //function to set the value.
    uTexture = glGetUniformLocation(mShaderProgram, "u_texture");

    //Loads sprites.
    std::vector<Sprite*>::iterator spriteIt;
    for (spriteIt = mSprites.begin(); spriteIt < mSprites.end();++spriteIt) {
        if ((*spriteIt)->load(mGraphicsManager)!=STATUS_OK) goto ERROR;
    }

    return STATUS_OK;

    ERROR:
    Log::error("Error loading sprite batch");
    return STATUS_KO;
}

void SpriteBatch::draw() {
    glUseProgram(mShaderProgram); //set a program to be in use. Install a program as part of the current rendering state
                                    //After a program is in-use, the shader objects are free to change, but not the linking part.
                                    //If a link is successful, the linked object will be installed.
    glUniformMatrix4fv(uProjection, 1, GL_FALSE, mGraphicsManager.getProjectionMatrix()); //load the uniform variable with a 4x4 matrix
    glUniform1i(uTexture, 0); //load the uniform variable with integer value

    glEnableVertexAttribArray(aPosition); //Enable attribute array when drawing a vertex (official
                                            // explanation  If enabled, the values in the generic
                                            // vertex attribute array will be accessed and used for
                                            // rendering when calls are made to vertex array commands
                                            // such as glDrawArrays, glDrawElements, glDrawRangeElements,
                                            // glMultiDrawElements, or glMultiDrawArrays.
    glVertexAttribPointer(aPosition,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Sprite::Vertex),
                          &(mVertices[0].x));  //set the vertex attribute pointer. Basically, this method
                                                // define loading value of type (GL_FLOAT) from an array
                                                // pointed by &(mVertices[0].x into aPosition attribute
                                                // in the shader program. The next value is size(Spite::Vertex)
                                                // apart from the first one.
    glEnableVertexAttribArray(aTexture);
    glVertexAttribPointer(aTexture,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Sprite::Vertex),
                          &(mVertices[0].u));

    glEnable(GL_BLEND);//In RGBA mode, pixels can be drawn using a function that blends the incoming
                        // (source) RGBA values with the RGBA values that are already in the frame
                        // buffer (the destination values). Blending is initially disabled. Use
                        // glEnable and glDisable with argument GL_BLEND to enable and disable blending.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //Transparency is best implemented using
                                                       // blend function (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
                                                       // with primitives sorted from farthest to nearest.
                                                       // Note that this transparency calculation
                                                       // does not require the presence of alpha
                                                       // bitplanes in the frame buffer.

                                                       // Incoming (source) alpha is correctly thought
                                                       // of as a material opacity, ranging from 1.0 ( K A ),
                                                       // representing complete opacity, to 0.0 (0),
                                                       // representing complete transparency.
                                                       //When more than one color buffer is enabled
                                                       // for drawing, the GL performs blending separately
                                                       // for each enabled buffer, using the contents
                                                       // of that buffer for destination color.
                                                       // (See glDrawBuffer.)

    const int32_t vertexPerSprite = 4;
    const int32_t indexPerSprite = 6;
    float timeStep = mTimeManager.elapsed();
    int32_t spriteCount = mSprites.size();
    int32_t currentSprite = 0, firstSprite = 0;
    while (bool canDraw = (currentSprite < spriteCount)) {
        //Switches texture.
        Sprite *sprite = mSprites[currentSprite];
        GLuint currentTexture = sprite->mTexture;
        glActiveTexture(GL_TEXTURE0);       //glActiveTexture selects which texture unit subsequent
                                            // texture state calls will affect. The number of texture
                                            // units an implementation supports is implementation
                                            // dependent, but must be at least 80
        glBindTexture(GL_TEXTURE_2D, sprite->mTexture);  // Create or use a named texture generated by
                                                         // glGenTextures

        //generate sprite vertices for current textures.
        do {
            sprite = mSprites[currentSprite];
            if (sprite->mTexture == currentTexture) {
                Sprite::Vertex *vertices = (&mVertices[currentSprite * 4]);
                sprite->draw(vertices, timeStep);
            } else {
                break;
            }
        } while (canDraw = (++currentSprite < spriteCount));
        glDrawElements(GL_TRIANGLES,
                //Number of indexes
                       (currentSprite - firstSprite) * indexPerSprite,
                       GL_UNSIGNED_SHORT,
                       &mIndexes[firstSprite * indexPerSprite]);//When glDrawElements is called, it
                                                                // uses count sequential elements
                                                                // from an enabled array, starting
                                                                // at indices to construct a sequence
                                                                // of geometric primitives. mode specifies
                                                                // what kind of primitives are constructed
                                                                // and how the array elements construct
                                                                // these primitives. If more than one
                                                                // array is enabled, each is used.
        firstSprite = currentSprite;
    }

    glUseProgram(0);
    glDisableVertexAttribArray(aPosition);
    glDisableVertexAttribArray(aTexture);
    glDisable(GL_BLEND);
}

