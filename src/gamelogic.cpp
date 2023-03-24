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

enum KeyFrameAction {
    BOTTOM, TOP
};

#include <timestamps.h>

double mousePositionX = 0;
double mousePositionZ = 0;

unsigned int currentKeyFrame = 0;
unsigned int previousKeyFrame = 0;

SceneNode* rootNode;
SceneNode* skyBoxNode;
SceneNode* wallNode;
SceneNode* icicleNode;

// These are heap allocated, because they should not be initialised at the start of the program
sf::SoundBuffer* buffer;
Gloom::Shader* shader;
Gloom::Shader* skyboxshader;

sf::Sound* sound;

const glm::vec3 boxDimensions(180, 90, 90);
const glm::vec3 wallDimensions(180, 180, 40);

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

    mousePositionX -= mouseSensitivity * deltaX / windowWidth;
    mousePositionZ -= mouseSensitivity * deltaY / windowHeight;

    if (mousePositionX > 1) mousePositionX = 1;
    if (mousePositionX < 0) mousePositionX = 0;
    if (mousePositionZ > 1) mousePositionZ = 1;
    if (mousePositionZ < 0) mousePositionZ = 0;

    glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);
}

//// A few lines to help you if you've never used c++ structs
struct LightSource {
    //bool a_placeholder_value;
    SceneNode *node;
};
LightSource lightSources[3]; /*Put number of light sources you want here*/

// Color vectors
glm::vec3 red = glm::vec3(1.0, 0.0, 0.0);
glm::vec3 green = glm::vec3(0.0, 1.0, 0.0);
glm::vec3 blue = glm::vec3(0.0, 0.0, 1.0);
glm::vec3 white = glm::vec3(1.0, 1.0, 1.0);
glm::vec3 yellow = glm::vec3(1.0, 1.0, 0.0);

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


    // Create meshes
    Mesh wall = cube(wallDimensions, glm::vec2(30, 40), true);
    Mesh box = cube(boxDimensions, glm::vec2(90), true, true);
    Mesh icicle = loadObj("../res/object/istapp.obj");

    // Fill buffers
    unsigned int boxVAO  = generateBuffer(box);
    unsigned int wallVAO  = generateBuffer(wall);
    unsigned int icicleVAO  = generateBuffer(icicle);


    // Creates a number of light scene nodes based on the number of light sources
    int iter = 0;
    for(LightSource &ls : lightSources) {
        ls.node = createSceneNode(POINT_LIGHT);
        ls.node->lightID = iter;
        iter++;
    }

    // offset for the light sources
    lightSources[0].node->position = glm::vec3(0.0, 2.0, 0.0);  // middle of pad

    // Assigns colors to the light nodes
    lightSources[0].node->color = yellow;

    // Construct scene
    rootNode = createSceneNode(GEOMETRY);

    skyBoxNode  = createSceneNode(TEXTURE_CUBE_MAP);
    wallNode  = createSceneNode(GEOMETRY);
    icicleNode = createSceneNode(GEOMETRY);

    rootNode->children.push_back(skyBoxNode);
    skyBoxNode->children.push_back(wallNode);
    skyBoxNode->children.push_back(icicleNode);

    // Assigns the VAO and IDs to the different scene nodes
    skyBoxNode->vertexArrayObjectID  = boxVAO;
    skyBoxNode->VAOIndexCount        = box.indices.size();
    skyBoxNode->textureID            = loadCubemap(skyboxFaces);
    // skyBoxNode->isSkybox             = true;

    wallNode->vertexArrayObjectID  = wallVAO;
    wallNode->VAOIndexCount        = wall.indices.size();
    // wallNode->textureID           = ll.ID;

    icicleNode->vertexArrayObjectID = icicleVAO;
    icicleNode->VAOIndexCount       = icicle.indices.size();
    // icicleNode->textureID           = ll.ID;

    getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;

    std::cout << "Ready. Click to start!" << std::endl;
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

            double ballYCoord;

            KeyFrameAction currentOrigin = keyFrameDirections.at(currentKeyFrame);
            KeyFrameAction currentDestination = keyFrameDirections.at(currentKeyFrame + 1);
        }
    }
    glm::vec3 cameraPosition = glm::vec3(0, 2, -20);

    skyBoxNode->rotation.y += timeDelta / 10;

    // Some math to make the camera move in a nice way
    float lookRotation = -0.6 / (1 + exp(-5 * (mousePositionX-0.5))) + 0.3;
    glm::mat4 cameraTransform =
                    glm::rotate(0.3f + 0.2f * float(-mousePositionZ*mousePositionZ), glm::vec3(1, 0, 0)) *
                    glm::rotate(lookRotation, glm::vec3(0, 1, 0)) *
                    glm::translate(-cameraPosition);

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

    for(LightSource &ls : lightSources) {
        // The light positions in the node update by multiplying the origin
        // of the coordinate space by the current transformation matrix.
        glm::vec4 lightPos = ls.node->currentTransformationMatrix * glm::vec4(0.0, 0.0, 0.0, 1.0);

        // Finds the lightsources struct from the shader and updates the location and color based on the current rendered light node
        GLint location = shader->getUniformFromName("LightSources[" + std::to_string(ls.node->lightID) + "].position");
        GLint color = shader->getUniformFromName("LightSources[" + std::to_string(ls.node->lightID) + "].color");
        glUniform3fv(location, 1, glm::value_ptr(lightPos));
        glUniform3fv(color, 1, glm::value_ptr(ls.node->color));
    }

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
