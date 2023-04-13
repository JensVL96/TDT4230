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

SceneNode* rootNode;
SceneNode* skyBoxNode;
SceneNode* padNode;
// SceneNode* wallNode;
SceneNode* icicleNode;
SceneNode* lightNode;
SceneNode* ballNode;

// These are heap allocated, because they should not be initialised at the start of the program
sf::SoundBuffer* buffer;
Gloom::Shader* shader;
Gloom::Shader* skyboxshader;
Gloom::Shader* iceshader;

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
IcicleList Icicles[5]; /*Put number of light sources you want here*/

// Color vectors
glm::vec3 red = glm::vec3(1.0, 0.0, 0.0);
glm::vec3 green = glm::vec3(0.0, 1.0, 0.0);
glm::vec3 blue = glm::vec3(0.0, 0.0, 1.0);
glm::vec3 white = glm::vec3(1.0, 1.0, 1.0);
glm::vec3 yellow = glm::vec3(0.42, 0.39, 0.19);

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

    shader = new Gloom::Shader();
    shader->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");
    shader->activate();

    skyboxshader = new Gloom::Shader();
    skyboxshader->makeBasicShader("../res/shaders/skybox.vert", "../res/shaders/skybox.frag");
    skyboxshader->activate();

    iceshader = new Gloom::Shader();
    iceshader->makeBasicShader("../res/shaders/ice.vert", "../res/shaders/ice.frag");
    iceshader->activate();

    PNGImage waterDrop = loadPNGFile("../res/textures/waterDrop_col.png");
    GenTextures(&waterDrop);
    PNGImage brick_col = loadPNGFile("../res/textures/Brick03_col.png");
    GenTextures(&brick_col);
    PNGImage brick_nrm = loadPNGFile("../res/textures/Brick03_nrm.png");
    GenTextures(&brick_nrm);

    // Create meshes
    Mesh pad = cube(padDimensions, glm::vec2(30, 40), true);
    // Mesh wall = cube(padDimensions, glm::vec2(30, 40), true);
    Mesh box = cube(boxDimensions, glm::vec2(90), true, true);
    Mesh icicle = loadObj("../res/object/Cone.obj");
    Mesh sphere = generateSphere(1.0, 40, 40);

    // Fill buffers
    unsigned int boxVAO  = generateBuffer(box);
    // unsigned int wallVAO  = generateBuffer(wall);
    unsigned int icicleVAO  = generateBuffer(icicle);
    unsigned int padVAO  = generateBuffer(pad);
    unsigned int sunVAO = generateBuffer(sphere);

    // Creates a number of light scene nodes based on the number of light sources
    
    lightNode = createSceneNode(POINT_LIGHT);
    lightNode->lightID = 0;

    // offset for the light sources
    lightNode->position      = glm::vec3(0.0, 0.0, ballRadius);  // middle of pad
    //lightNode->scale         = glm::vec3(100.0, 100.0, 100.0);
    lightNode->color         = yellow;

    // Construct scene
    rootNode = createSceneNode(GEOMETRY);
    skyBoxNode  = createSceneNode(TEXTURE_CUBE_MAP);

    padNode = createSceneNode(GEOMETRY);
    ballNode = createSceneNode(GEOMETRY);
    // wallNode  = createSceneNode(GEOMETRY);
    // icicleNode = createSceneNode(GEOMETRY);

    // Puts the Scene nodes in hierarchical order
    rootNode->children.push_back(skyBoxNode);
    // skyBoxNode->children.push_back(wallNode);

    rootNode->children.push_back(padNode);
    skyBoxNode->children.push_back(ballNode);
    // padNode->children.push_back(Icicles[0].node);

    // Puts the light source scene nodes to the different objects
    ballNode->children.push_back(lightNode);  // Root node if aiming to put in the box

    // Assigns the VAO and IDs to the different scene nodes
    skyBoxNode->vertexArrayObjectID = boxVAO;
    skyBoxNode->VAOIndexCount       = box.indices.size();
    skyBoxNode->textureID           = loadCubemap(skyboxFaces);
    skyBoxNode->isSkybox            = true;

    padNode->vertexArrayObjectID    = padVAO;
    padNode->VAOIndexCount          = pad.indices.size();

    ballNode->vertexArrayObjectID   = sunVAO;
    ballNode->VAOIndexCount         = sphere.indices.size();
    ballNode->position              = glm::vec3(-85.0, 30.0, 120.0);

    // wallNode->vertexArrayObjectID  = wallVAO;
    // wallNode->VAOIndexCount        = wall.indices.size();

    int iter = 0;
    for(IcicleList &il : Icicles) {
        il.node = createSceneNode(ICE);
        padNode->children.push_back(il.node);
        il.node->vertexArrayObjectID = icicleVAO;
        il.node->VAOIndexCount       = icicle.indices.size();
        il.node->rotation            = glm::vec3(M_PI, -M_PI/2, 0.0);
        il.node->textureID           = brick_col.ID;

        iter++;
    }
    Icicles[0].node->position            = glm::vec3(0.0, 150.0, 0.0);
    Icicles[0].node->scale               = glm::vec3(7.0, 7.0, 7.0);

    Icicles[1].node->position            = glm::vec3(-50.0, 150.0, 0.0);
    Icicles[1].node->scale               = glm::vec3(7.0, 7.0, 7.0);
    // icicleNode->textureID           = ll.ID;

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

void updateFrame(GLFWwindow* window) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    double timeDelta = getTimeDeltaSeconds();

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
        case SPOT_LIGHT: break;
        case POINT_LIGHT: break;
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

                glUniform1i(10, 0); // 3D geometry
                glUniform1i(11, 0); // normal mapped
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
                GLint location = shader->getUniformFromName("LightSources[" + std::to_string(lightNode->lightID) + "].position");
                GLint color = shader->getUniformFromName("LightSources[" + std::to_string(lightNode->lightID) + "].color");
                glUniform3fv(location, 1, glm::value_ptr(lightPos));
                glUniform3fv(color, 1, glm::value_ptr(lightNode->color));


                // Transforming the normal matrix without unnecessary translations and send them to the shader
                glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(node->currentTransformationMatrix)));
                glUniformMatrix3fv(8, 1, GL_FALSE, glm::value_ptr(normalMatrix));

                glUniform1i(10, 0); // 3D geometry
                glUniform1i(11, 0); // not normal mapped

                glBindTextureUnit(3, node->textureID);
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
