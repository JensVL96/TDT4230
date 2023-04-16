#include <chrono>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <SFML/Audio/SoundBuffer.hpp>
#include <utilities/shader.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <utilities/timeutils.h>
#include <utilities/mesh.h>
#include <utilities/shapes.h>
#include <utilities/glutils.h>
#include <SFML/Audio/Sound.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fmt/format.h>
#include "gamelogic.h"
#include "sceneGraph.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <fstream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "utilities/imageLoader.hpp"
#include "utilities/glfont.h"

#include "glm/ext.hpp"
#define __DEBUG__

enum KeyFrameAction {
    BOTTOM, TOP
};

#include <timestamps.h>

double padPositionX = 0;
double padPositionZ = 0;

unsigned int currentKeyFrame = 0;
unsigned int previousKeyFrame = 0;

// Texture images
PNGImage sunFlare = loadPNGFile("../res/textures/uni-white2.png");
PNGImage lensFlare = loadPNGFile("../res/textures/lens-flare.png");
PNGImage lensBubble = loadPNGFile("../res/textures/flare_bubble.png");
PNGImage lensBubble2 = loadPNGFile("../res/textures/flare_bubble2.png");
PNGImage waterDrops = loadPNGFile("../res/textures/blue_dots.png");

// Generate textures
void genFiles() { 
    GenTextures(&waterDrops);
    GenTextures(&sunFlare);
    GenTextures(&lensFlare);
    GenTextures(&lensBubble);
    GenTextures(&lensBubble2);
}

// Rendered scene nodes
SceneNode* rootNode;
SceneNode* skyBoxNode;
SceneNode* padNode;
SceneNode* icicleNode;
SceneNode* lightNode;
SceneNode* ballNode;
SceneNode* quadNode;

// These are heap allocated, because they should not be initialised at the start of the program
sf::SoundBuffer* buffer;
Gloom::Shader* shader;
Gloom::Shader* skyboxshader;
Gloom::Shader* iceshader;
Gloom::Shader* sunShader;
Gloom::Shader* flare;

sf::Sound* sound;

const glm::vec3 boxDimensions(180, 90, 90);
const glm::vec3 padDimensions(30, 3, 40);
// const glm::vec3 padDimensions(180, 180, 1);

double ballRadius = 3.0f;
glm::vec3 ballPosition(0, ballRadius + 60, (boxDimensions.z / 2) - 100);

CommandLineOptions options;

bool hasStarted        = false;
bool hasLost           = false;
bool jumpedToNextFrame = false;
bool isPaused          = false;

bool mouseLeftPressed   = false;
bool mouseLeftReleased  = false;
bool mouseRightPressed  = false;
bool mouseRightReleased = false;


// Modify if you want the music to start further on in the track. Measured in seconds.
const float debug_startTime = 0;
double totalElapsedTime = debug_startTime;
double gameElapsedTime = debug_startTime;

double mouseSensitivity = 1.0;
double lastMouseX = windowWidth / 2;
double lastMouseY = windowHeight / 2;
void mouseCallback(GLFWwindow* window, double x, double y) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    double deltaX = x - lastMouseX;
    double deltaY = y - lastMouseY;

    //std::cout << windowHeight << windowWidth << std::endl;

    padPositionX -= mouseSensitivity * deltaX / windowWidth;
    padPositionZ -= mouseSensitivity * deltaY / windowHeight;

    if (padPositionX > 1) padPositionX = 1;
    if (padPositionX < 0) padPositionX = 0;
    if (padPositionZ > 1) padPositionZ = 1;
    if (padPositionZ < 0) padPositionZ = 0;

    glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);
}

//// A few lines to help you if you've never used c++ structs
struct LightSource {
    //bool a_placeholder_value;
    SceneNode *node;
};
LightSource lightSources[1]; /*Put number of light sources you want here*/

struct IcicleList {
    //bool a_placeholder_value;
    SceneNode *node;
};
IcicleList Icicles[5]; /*Put number of icicles you want here*/

// Color vectors
glm::vec3 red = glm::vec3(1.0, 0.0, 0.0);
glm::vec3 green = glm::vec3(0.0, 1.0, 0.0);
glm::vec3 blue = glm::vec3(0.0, 0.0, 1.0);
glm::vec3 white = glm::vec3(1.0, 1.0, 1.0);
glm::vec3 yellow = glm::vec3(0.42, 0.39, 0.19);

// Loads and binds given images to a cube map
unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

// 6 sides of the cube map
std::vector<std::string> skyboxFaces = {
    "../res/skybox/right.jpg",
    "../res/skybox/left.jpg",
    "../res/skybox/top.jpg",
    "../res/skybox/bottom.jpg",
    "../res/skybox/front.jpg",
    "../res/skybox/negz.jpg",
};

void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
    std::cerr << "GL CALLBACK: " <<
    //"source: " << source <<
    //" type:" << type <<
    //" id: " << id <<
    //" severity: " << severity <<
    //" length: " << length <<
    " message: " << message << std::endl;

}


void initGame(GLFWwindow* window, CommandLineOptions gameOptions) {
    buffer = new sf::SoundBuffer();
    if (!buffer->loadFromFile("../res/Hall of the Mountain King.ogg")) {
        return;
    }

    options = gameOptions;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetCursorPosCallback(window, mouseCallback);

    // creating the shaders
    shader = new Gloom::Shader();
    shader->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");
    shader->activate();

    skyboxshader = new Gloom::Shader();
    skyboxshader->makeBasicShader("../res/shaders/skybox.vert", "../res/shaders/skybox.frag");
    skyboxshader->activate();

    iceshader = new Gloom::Shader();
    iceshader->makeBasicShader("../res/shaders/ice.vert", "../res/shaders/ice.frag");
    iceshader->activate();

    sunShader = new Gloom::Shader();
    sunShader->makeBasicShader("../res/shaders/sun.vert", "../res/shaders/sun.frag");
    sunShader->activate();

    flare = new Gloom::Shader();
    flare->makeBasicShader("../res/shaders/flare.vert", "../res/shaders/flare.frag");
    flare->activate();

    // loading the textures
    genFiles();

    // Create meshes
    Mesh pad = cube(padDimensions, glm::vec2(30, 40), true);
    Mesh box = cube(boxDimensions, glm::vec2(90), true, true);
    Mesh icicle = loadObj("../res/object/Cone.obj");
    Mesh sphere = generateSphere(1.0, 40, 40);

    // Redefines the normals from the cone to a cylinder around 
    for (int i = 0; i < icicle.vertices.size(); i++) {
        auto x = icicle.normals[i].x;
        auto y = icicle.vertices[i].y;
        auto z = icicle.normals[i].z;
        //std::cout << atan2(x-0, z-0) /(2 * M_PI) + 0.5 << ", " << y << std::endl;
        icicle.textureCoordinates[atan2(x, z) / (2 * M_PI) + 0.5, y / 24.0];
    }

    // Creates the mesh for a sun sprite
    Mesh quad;
    quad.vertices = {
        {-1, -1, 0},
        { 1, -1, 0},
        { 1,  1, 0},
        {-1,  1, 0},
    };
    quad.textureCoordinates = {
        {0, 0},
        {1, 0},
        {1, 1},
        {0, 1},
    };
    quad.indices = {
        0, 1, 2,
        0, 2, 3,
    };



    // Fill buffers
    unsigned int boxVAO  = generateBuffer(box);
    unsigned int icicleVAO  = generateBuffer(icicle);
    unsigned int padVAO  = generateBuffer(pad);
    unsigned int sunVAO = generateBuffer(sphere);
    unsigned int quadVAO = generateBuffer(quad);

    // Creates the light node for the sun
    lightNode = createSceneNode(POINT_LIGHT);
    lightNode->lightID = 0;
    lightNode->position      = glm::vec3(0.0, 0.0, ballRadius);
    lightNode->color         = yellow;

    // Construct scene
    rootNode = createSceneNode(GEOMETRY);
    skyBoxNode  = createSceneNode(TEXTURE_CUBE_MAP);

    padNode = createSceneNode(GEOMETRY);
    ballNode = createSceneNode(POINT_LIGHT);

    quadNode = createSceneNode(SPOT_LIGHT);
    quadNode->vertexArrayObjectID = quadVAO;
    quadNode->VAOIndexCount = 6;

    // Puts the Scene nodes in hierarchical order
    rootNode->children.push_back(skyBoxNode);
    rootNode->children.push_back(padNode);
    skyBoxNode->children.push_back(ballNode);
    ballNode->children.push_back(lightNode);
    rootNode->children.push_back(quadNode);

    // Assigns the VAO and IDs to the different scene nodes
    skyBoxNode->vertexArrayObjectID = boxVAO;
    skyBoxNode->VAOIndexCount       = box.indices.size();
    skyBoxNode->textureID           = loadCubemap(skyboxFaces);
    skyBoxNode->isSkybox            = true;

    padNode->vertexArrayObjectID    = -1; //padVAO;
    padNode->VAOIndexCount          = pad.indices.size();

    ballNode->vertexArrayObjectID   = sunVAO;
    ballNode->VAOIndexCount         = sphere.indices.size();
    ballNode->position              = glm::vec3(-85.0, 30.0, 120.0);
    ballNode->color                = white;

    // creates multiple icicle nodes
    int iter = 0;
    for(IcicleList &il : Icicles) {
        il.node = createSceneNode(ICE);
        padNode->children.push_back(il.node);
        il.node->vertexArrayObjectID = icicleVAO;
        il.node->VAOIndexCount       = icicle.indices.size();
        il.node->rotation            = glm::vec3(M_PI, -M_PI/2, 0.0);
        il.node->textureID           = waterDrops.ID;

        iter++;
    }
    // Defines the icicle coordinates and size
    Icicles[0].node->position            = glm::vec3(27.0, 150.0, 0.0);
    Icicles[0].node->scale               = glm::vec3(7.0, 6.3, 7.0);

    Icicles[1].node->position            = glm::vec3(13.0, 150.0, 0.0);
    Icicles[1].node->scale               = glm::vec3(7.0, 6.0, 7.0);

    Icicles[2].node->position            = glm::vec3(0.0, 150.0, 0.0);
    Icicles[2].node->scale               = glm::vec3(7.0, 7.0, 7.0);

    Icicles[3].node->position            = glm::vec3(-10.0, 150.0, 0.0);
    Icicles[3].node->scale               = glm::vec3(8.0, 5.0, 7.0);

    Icicles[4].node->position            = glm::vec3(-23.0, 150.0, 0.0);
    Icicles[4].node->scale               = glm::vec3(7.0, 5.0, 7.0);

    getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;

    std::cout << "Ready. Click to start!" << std::endl;

    #ifdef __DEBUG__
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(MessageCallback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_OTHER , GL_DONT_CARE, 0, nullptr, false);
    #endif
}

// Sets the projection as a global variable
glm::mat4 P;
glm::mat4 V;

float epoch = 0;

void updateFrame(GLFWwindow* window) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    double timeDelta = getTimeDeltaSeconds();
    epoch += timeDelta;

    const float cameraWallOffset = 30; // Arbitrary addition to prevent ball from going too much into camera

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1)) {
        mouseLeftPressed = true;
        mouseLeftReleased = false;
    } else {
        mouseLeftReleased = mouseLeftPressed;
        mouseLeftPressed = false;
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2)) {
        mouseRightPressed = true;
        mouseRightReleased = false;
    } else {
        mouseRightReleased = mouseRightPressed;
        mouseRightPressed = false;
    }

    if(!hasStarted) {
        if (mouseLeftPressed) {
            if (options.enableMusic) {
                sound = new sf::Sound();
                sound->setBuffer(*buffer);
                sf::Time startTime = sf::seconds(debug_startTime);
                sound->setPlayingOffset(startTime);
                sound->play();
            }

            totalElapsedTime = debug_startTime;
            gameElapsedTime = debug_startTime;
            hasStarted = true;
        }
    } else {
        totalElapsedTime += timeDelta;
        if(hasLost) {
            if (mouseLeftReleased) {
                hasLost = false;
                hasStarted = false;
                currentKeyFrame = 0;
                previousKeyFrame = 0;
            }
        } else if (isPaused) {
            if (mouseRightReleased) {
                isPaused = false;
                if (options.enableMusic) {
                    sound->play();
                }
            }
        } else {
            gameElapsedTime += timeDelta;
            if (mouseRightReleased) {
                isPaused = true;
                if (options.enableMusic) {
                    sound->pause();
                }
            }
            // Get the timing for the beat of the song
            for (unsigned int i = currentKeyFrame; i < keyFrameTimeStamps.size(); i++) {
                if (gameElapsedTime < keyFrameTimeStamps.at(i)) {
                    continue;
                }
                currentKeyFrame = i;
            }

            jumpedToNextFrame = currentKeyFrame != previousKeyFrame;
            previousKeyFrame = currentKeyFrame;

            double frameStart = keyFrameTimeStamps.at(currentKeyFrame);
            double frameEnd = keyFrameTimeStamps.at(currentKeyFrame + 1); // Assumes last keyframe at infinity

            double elapsedTimeInFrame = gameElapsedTime - frameStart;
            double frameDuration = frameEnd - frameStart;
            double fractionFrameComplete = elapsedTimeInFrame / frameDuration;

            KeyFrameAction currentOrigin = keyFrameDirections.at(currentKeyFrame);
            KeyFrameAction currentDestination = keyFrameDirections.at(currentKeyFrame + 1);
        }
    }
    glm::vec3 cameraPosition = glm::vec3(0, 2, -20);

    //skyBoxNode->rotation.y += timeDelta / 10;
    ballNode->position = ballPosition;
    ballNode->scale = glm::vec3(ballRadius);
    ballNode->rotation = { 0, totalElapsedTime*2, 0 };

    // Some math to make the camera move in a nice way
    float lookRotation = -0.6 / (1 + exp(-5 * (padPositionX-0.5))) + 0.3;
    glm::mat4 cameraTransform =
                    glm::rotate(0.3f + 0.2f * float(-padPositionZ*padPositionZ), glm::vec3(1, 0, 0)) *
                    glm::rotate(lookRotation, glm::vec3(0, 1, 0)) *
                    glm::translate(-cameraPosition);

    // Move and rotate various SceneNodes
    skyBoxNode->position = { 0, -10, -80 };

    // wallNode->position  = {
    //     boxNode->position.x - (boxDimensions.x/2) + (padDimensions.x/2) + (1 - padPositionX) * (boxDimensions.x - padDimensions.x),
    //     boxNode->position.y - (boxDimensions.y/2) + (padDimensions.y/2),
    //     boxNode->position.z - (boxDimensions.z/2) + (padDimensions.z/2) + (1 - padPositionZ) * (boxDimensions.z - padDimensions.z)
    // };

    padNode->position  = {
        skyBoxNode->position.x - (boxDimensions.x/2) + (padDimensions.x/2) + (1 - padPositionX) * (boxDimensions.x - padDimensions.x),
        skyBoxNode->position.y - (boxDimensions.y/2) + (padDimensions.y/2),
        skyBoxNode->position.z - (boxDimensions.z/2) + (padDimensions.z/2) + (1 - padPositionZ) * (boxDimensions.z - padDimensions.z)
    };

    // Puts together the view and projection for the shader
    glm::mat4 projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 350.f);
    P = projection;
    V = cameraTransform;

    updateNodeTransformations(rootNode, glm::identity<glm::mat4>());

}

void updateNodeTransformations(SceneNode* node, glm::mat4 transformationThusFar) {
    glm::mat4 transformationMatrix =
              glm::translate(node->position)
            * glm::translate(node->referencePoint)
            * glm::rotate(node->rotation.y, glm::vec3(0,1,0))
            * glm::rotate(node->rotation.x, glm::vec3(1,0,0))
            * glm::rotate(node->rotation.z, glm::vec3(0,0,1))
            * glm::scale(node->scale)
            * glm::translate(-node->referencePoint);

    node->currentTransformationMatrix = transformationThusFar * transformationMatrix;

    switch(node->nodeType) {
        case GEOMETRY: break;
        case POINT_LIGHT: break;
        case SPOT_LIGHT: break;
        case GEOMETRY_2D: break;
        case NORMAL_MAPPED_GEOMETRY: break;
        case TEXTURE_CUBE_MAP: break;
        case ICE: break;
    }

    for(SceneNode* child : node->children) {
        updateNodeTransformations(child, node->currentTransformationMatrix);
    }
}

void renderNode(SceneNode* node) {
    // Sends the model to the shader
    glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));
    // Sends the view and projection to the shader
    glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(V));
    glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(P));

    // The light positions in the node update by multiplying the origin
    // of the coordinate space by the current transformation matrix.
    glm::vec4 lightPos = lightNode->currentTransformationMatrix * glm::vec4(0.0, 0.0, 0.0, 1.0);

    // Finds the lightsources struct from the shader and updates the location and color based on the current rendered light node
    GLint location = shader->getUniformFromName("LightSources[" + std::to_string(lightNode->lightID) + "].position");
    GLint color = shader->getUniformFromName("LightSources[" + std::to_string(lightNode->lightID) + "].color");
    glUniform3fv(location, 1, glm::value_ptr(lightPos));
    glUniform3fv(color, 1, glm::value_ptr(lightNode->color));

    // Transforming the normal matrix without unnecessary translations and send them to the shader
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(node->currentTransformationMatrix)));
    glUniformMatrix3fv(8, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    switch(node->nodeType) {
        case GEOMETRY:
            if(node->vertexArrayObjectID != -1) {
                glUniform1i(10, 0); // 3D geometry
                glUniform1i(11, 0); // not normal mapped
                glBindVertexArray(node->vertexArrayObjectID);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            break;
        case GEOMETRY_2D: 
            if(node->vertexArrayObjectID != -1) {
                glUniform1i(10, 1); // 10 is diffuse texture, 2D
                glUniform1i(11, 0); // not normal mapped
                glBindTextureUnit(0, node->textureID);
                glBindVertexArray(node->vertexArrayObjectID);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            break;
        case SPOT_LIGHT: 
            if(node->vertexArrayObjectID != -1) {
                flare->activate();
                // glDepthMask(GL_FALSE);
                glm::mat4 MVP = P*V*ballNode->currentTransformationMatrix;
                glm::vec4 pos = MVP * glm::vec4(0, 0, 0, 1);
                pos /= pos.w;
                float scale = 0.1;
                float aspect = 16.0/9.0;

                //auto UV = (MVP * sunNode).xy()
                //draw billboard with uv
                //for t in [0, 0.2, 0.4, 0.8]
                //-UV * (t*2-1)

                // lensflare, depth disabled
                int amount = 5;
                float t[amount] = {0.0, 0.15, 0.2, 0.35, 0.45};


                int ID[amount] = {sunFlare.ID, lensBubble.ID, lensBubble2.ID, lensBubble.ID, lensBubble2.ID};

                // Sends the model to the shader
                for (int i = 0; i < amount; i++) {
                    pos = -pos * (t[i] *2-1);
                    glUniform3f(0, pos.x, pos.y, pos.z);
                    glUniform2f(1, scale, scale*aspect);

                    glBindTextureUnit(0, ID[i]);
                    glBindVertexArray(quadNode->vertexArrayObjectID);
                    glDrawElements(GL_TRIANGLES, quadNode->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
                }
                // glDepthMask(GL_TRUE);
                shader->activate();
            }
            break;
        case POINT_LIGHT: 
            if(node->vertexArrayObjectID != -1) {
                sunShader->activate();
                // Sends the model to the shader
                glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));
                // Sends the view and projection to the shader
                glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(V));
                glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(P));

                // unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
                // glDrawBuffers(2, attachments);
                glBindVertexArray(node->vertexArrayObjectID);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
                shader->activate();
            }
        break;
        case NORMAL_MAPPED_GEOMETRY:
            if(node->vertexArrayObjectID != -1) {
                glUniform1i(10, 0); // 3D geometry
                glUniform1i(11, 1); // normal mapped
                glBindTextureUnit(0, node->textureID);
                glBindTextureUnit(1, node->normalMapID);
                glBindVertexArray(node->vertexArrayObjectID);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            break;
        case TEXTURE_CUBE_MAP: 
            if(node->vertexArrayObjectID != -1) {
                skyboxshader->activate();
                glDepthMask(GL_FALSE);
                // Sends the model to the shader
                glUniformMatrix3fv(3, 1, GL_FALSE, glm::value_ptr(glm::mat3(node->currentTransformationMatrix)));
                // Sends the view and projection to the shader
                glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(V));
                glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(P));

                glBindVertexArray(node->vertexArrayObjectID);
                glBindTextureUnit(2, node->textureID);
                //glUniform1i(12, 1);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
                shader->activate();
                glDepthMask(GL_TRUE);
            }
        break;
        case ICE: 
            if(node->vertexArrayObjectID != -1) {
                iceshader->activate();
                // Sends the model to the shader
                glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));
                // Sends the view and projection to the shader
                glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(V));
                glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(P));

                // The light positions in the node update by multiplying the origin
                // of the coordinate space by the current transformation matrix.
                glm::vec4 lightPos = lightNode->currentTransformationMatrix * glm::vec4(0.0, 0.0, 0.0, 1.0);

                // Finds the lightsources struct from the shader and updates the location and color based on the current rendered light node
                GLint location = iceshader->getUniformFromName("LightSources[" + std::to_string(lightNode->lightID) + "].position");
                GLint color = iceshader->getUniformFromName("LightSources[" + std::to_string(lightNode->lightID) + "].color");
                glUniform3fv(location, 1, glm::value_ptr(glm::vec3(lightPos)));
                glUniform3fv(color, 1, glm::value_ptr(lightNode->color));

                glUniform1f(iceshader->getUniformFromName("epoch"), epoch);

                // Transforming the normal matrix without unnecessary translations and send them to the shader
                glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(node->currentTransformationMatrix)));
                glUniformMatrix3fv(8, 1, GL_FALSE, glm::value_ptr(normalMatrix));

                glUniform1i(10, 0); // 3D geometry
                glUniform1i(11, 1); // not normal mapped

                glBindTextureUnit(0, node->textureID);
                glBindVertexArray(node->vertexArrayObjectID);

                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
                shader->activate();
            }
        break;
        /**/
    }

    for(SceneNode* child : node->children) {
        renderNode(child);
    }
}

void renderFrame(GLFWwindow* window) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    renderNode(rootNode);

}

// TODO:
    // lensflare
    // raindrop texture
    // multiple icicles