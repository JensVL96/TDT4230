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

double padPositionX = 0;
double padPositionZ = 0;

unsigned int currentKeyFrame = 0;
unsigned int previousKeyFrame = 0;

SceneNode* rootNode;
SceneNode* boxNode;
SceneNode* ballNode;
SceneNode* padNode;
SceneNode* textNode;
SceneNode* SBNode;
SceneNode* scoreNode;
SceneNode* HSBNode;
SceneNode* highScoreNode;
double ballRadius = 3.0f;

// Current and high score values
int singleScore = 100;  // Set to three digit number so the score can transistion without bugs
FILE* HIGHSCORES;
char inHighscore[10];

// Default text size
float textRatio = 1.3;
float textWidth = 0.8;

// These are heap allocated, because they should not be initialised at the start of the program
sf::SoundBuffer* buffer;
Gloom::Shader* shader;
sf::Sound* sound;

const glm::vec3 boxDimensions(180, 90, 90);
const glm::vec3 padDimensions(30, 3, 40);

glm::vec3 ballPosition(0, ballRadius + padDimensions.y, boxDimensions.z / 2);
glm::vec3 ballDirection(1, 1, 0.2f);

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
LightSource lightSources[3]; /*Put number of light sources you want here*/

// Color vectors
glm::vec3 red = glm::vec3(1.0, 0.0, 0.0);
glm::vec3 green = glm::vec3(0.0, 1.0, 0.0);
glm::vec3 blue = glm::vec3(0.0, 0.0, 1.0);
glm::vec3 white = glm::vec3(1.0, 1.0, 1.0);

// Function that creates a new VAO and assigns it to the inputted scene node (for rendering dynamic nodes)
void newScoreMesh(SceneNode* node, int num, int amp = 1.0, int offset = 0.1) {
    Mesh currentNUM = generateTextGeometryBuffer(std::to_string(num), textRatio, textWidth);
    unsigned int VAOid  = generateBuffer(currentNUM);

    // Differs the hardcode for current score tracker and the high score
    if (node == scoreNode) {
        node->scale = glm::vec3(0.1*amp, 0.1*amp, 1.0);
        node->position = glm::vec3(textWidth+offset, 0.0, 0.0);
    } else if (node == highScoreNode) {
        node->scale = glm::vec3(0.1, 0.2, 1.0);
        node->position = glm::vec3(textWidth, 0.45, 0.0);
    }

    node->vertexArrayObjectID = VAOid;
}

// unsigned int loadCubemap(std::string path) {
//     GLuint textureID;
//     glGenTextures(1, &textureID);
//     glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

//     std::string files[6] = {"right.png", "let.png", "top.png", "bottom.png", "front.png", "wood.png"};


//     int width, height, nrChannels;
//     for (unsigned int i = 0; i < 6; i++)
//     {
//         auto texture = loadPNGFile(path + files[i]);
//         glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
//                          0, GL_RGB, texture.width, texture.height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture.pixels.data()
//             );
//         }
//     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//     glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

//     return textureID;
// }

// std::vector<std::string> faces = {
//     "../res/textures/right.jpg",
//     "../res/textures/left.jpg",
//     "../res/textures/top.jpg",
//     "../res/textures/bottom.jpg",
//     "../res/textures/front.jpg",
//     "../res/textures/Wood.jpg"
// };
// unsigned int cubemapTexture = loadCubemap(faces); 


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


    // PNGImage sky_left = loadPNGFile("../res/textures/left.png");

    // Loads the textures from the resource folder
    PNGImage ll = loadPNGFile("../res/textures/charmap.png");
    GenTextures(&ll);
    PNGImage brick_col = loadPNGFile("../res/textures/Brick03_col.png");
    GenTextures(&brick_col);
    PNGImage brick_nrm = loadPNGFile("../res/textures/Brick03_nrm.png");
    GenTextures(&brick_nrm);

    // Reads the current high score from the highscore text file
    HIGHSCORES = fopen("../res/highscores.txt", "r");
    fgets(inHighscore, 10, HIGHSCORES);
    fclose(HIGHSCORES);

    // Create meshes
    Mesh pad = cube(padDimensions, glm::vec2(30, 40), true);
    Mesh box = cube(boxDimensions, glm::vec2(90), true, true);
    Mesh sphere = generateSphere(1.0, 40, 40);
    Mesh initText = generateTextGeometryBuffer("Click to start", textRatio, textWidth);
    Mesh SB = generateTextGeometryBuffer("Score", 1.2, 0.5);
    Mesh score = generateTextGeometryBuffer(std::to_string(singleScore), textRatio, textWidth);
    Mesh HSB = generateTextGeometryBuffer("Highscore", 1.2, 1.0);
    Mesh highScore = generateTextGeometryBuffer(inHighscore, textRatio, textWidth);

    // Fill buffers
    unsigned int ballVAO = generateBuffer(sphere);
    unsigned int boxVAO  = generateBuffer(box);
    unsigned int padVAO  = generateBuffer(pad);
    unsigned int textVAO  = generateBuffer(initText);
    unsigned int SBVAO  = generateBuffer(SB);
    unsigned int scoreVAO  = generateBuffer(score);
    unsigned int HSBVAO  = generateBuffer(HSB);
    unsigned int highScoreVAO  = generateBuffer(highScore);

    // Creates a number of light scene nodes based on the number of light sources
    int iter = 0;
    for(LightSource &ls : lightSources) {
        ls.node = createSceneNode(POINT_LIGHT);
        ls.node->lightID = iter;
        iter++;
    }

    // offset for the light sources
    lightSources[0].node->position = glm::vec3(0.0, 2.0, 0.0);  // middle of pad

    // Extra positions
    // lightSources[1].node->position = glm::vec3(-85, 30, -120);   // left corner of box
    // lightSources[2].node->position = glm::vec3(85, 30, -120);    // right corner of box
    // lightSources[0].node->position = glm::vec3(0.0, 10.0, -60.0);// middle of the room

    // Assigns colors to the light nodes
    // lightSources[0].node->color = red;
    // lightSources[1].node->color = blue;
    // lightSources[2].node->color = green;
    lightSources[0].node->color = white;

    // Construct scene
    rootNode = createSceneNode(GEOMETRY);
    boxNode  = createSceneNode(NORMAL_MAPPED_GEOMETRY);
    padNode  = createSceneNode(GEOMETRY);
    ballNode = createSceneNode(GEOMETRY);
    textNode = createSceneNode(GEOMETRY_2D);
    SBNode = createSceneNode(GEOMETRY_2D);
    scoreNode = createSceneNode(GEOMETRY_2D);
    HSBNode = createSceneNode(GEOMETRY_2D);
    highScoreNode = createSceneNode(GEOMETRY_2D);


    rootNode->children.push_back(boxNode);
    rootNode->children.push_back(padNode);
    rootNode->children.push_back(ballNode);
    rootNode->children.push_back(textNode);
    rootNode->children.push_back(SBNode);
    rootNode->children.push_back(scoreNode);
    rootNode->children.push_back(HSBNode);
    rootNode->children.push_back(highScoreNode);

    // Puts the light source scene nodes to the different objects
    padNode->children.push_back(lightSources[0].node);
    // rootNode->children.push_back(lightSources[0].node);  // Root node if aiming to put in the box

    // offset for the text
    textNode->position = glm::vec3(-textWidth/2, 0.0, 0.0);

    // Transformations on the scoreboard logo, current score, high score logo and current high score
    SBNode->scale = glm::vec3(0.5, 0.5, 0.7);
    SBNode->position = glm::vec3(0.75, 0.15, 0.0);

    scoreNode->scale = glm::vec3(0.1, 0.1, 1.0);
    scoreNode->position = glm::vec3(textWidth+0.1, 0.0, 0.0);

    HSBNode->scale = glm::vec3(0.5, 0.5, 0.7);
    HSBNode->position = glm::vec3(0.5, 0.6, 0.0);

    highScoreNode->scale = glm::vec3(0.1, 0.2, 1.0);
    highScoreNode->position = glm::vec3(textWidth, 0.45, 0.0);

    // Assigns the VAO and IDs to the different scene nodes
    boxNode->vertexArrayObjectID  = boxVAO;
    boxNode->VAOIndexCount        = box.indices.size();
    boxNode->textureID            = brick_col.ID;
    boxNode->normalMapID          = brick_nrm.ID;
    // boxNode->textureID            = loadCubemap("../res/textures/");
    boxNode->isSkybox             = false;

    padNode->vertexArrayObjectID  = padVAO;
    padNode->VAOIndexCount        = pad.indices.size();

    ballNode->vertexArrayObjectID = ballVAO;
    ballNode->VAOIndexCount       = sphere.indices.size();

    textNode->vertexArrayObjectID = textVAO;
    textNode->VAOIndexCount       = initText.indices.size();
    textNode->textureID           = ll.ID;

    SBNode->vertexArrayObjectID = SBVAO;
    SBNode->VAOIndexCount       = SB.indices.size();
    SBNode->textureID           = ll.ID;

    scoreNode->vertexArrayObjectID = scoreVAO;
    scoreNode->VAOIndexCount       = score.indices.size();
    scoreNode->textureID           = ll.ID;

    HSBNode->vertexArrayObjectID = HSBVAO;
    HSBNode->VAOIndexCount       = HSB.indices.size();
    HSBNode->textureID           = ll.ID;

    highScoreNode->vertexArrayObjectID = highScoreVAO;
    highScoreNode->VAOIndexCount       = highScore.indices.size();
    highScoreNode->textureID           = ll.ID;

    getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;

    std::cout << "Ready. Click to start!" << std::endl;
}

// Sets the projection as a global variable
glm::mat4 P;
glm::mat4 V;

// Function to update the numbers on the screen (size depends on amount of numbers)
void updateScore(SceneNode* score, int num) {
    if (num <= 9) {
        newScoreMesh(score, num, 1.0);
    } else {
        newScoreMesh(score, num, 2.0);
    }
}

void updateFrame(GLFWwindow* window) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    double timeDelta = getTimeDeltaSeconds();

    const float ballBottomY = boxNode->position.y - (boxDimensions.y/2) + ballRadius + padDimensions.y;
    const float ballTopY    = boxNode->position.y + (boxDimensions.y/2) - ballRadius;
    const float BallVerticalTravelDistance = ballTopY - ballBottomY;

    const float cameraWallOffset = 30; // Arbitrary addition to prevent ball from going too much into camera

    const float ballMinX = boxNode->position.x - (boxDimensions.x/2) + ballRadius;
    const float ballMaxX = boxNode->position.x + (boxDimensions.x/2) - ballRadius;
    const float ballMinZ = boxNode->position.z - (boxDimensions.z/2) + ballRadius;
    const float ballMaxZ = boxNode->position.z + (boxDimensions.z/2) - ballRadius - cameraWallOffset;

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
            // removes the initText node and resets the current tracked score
            textNode->vertexArrayObjectID = -2;
            singleScore = 0;

            totalElapsedTime = debug_startTime;
            gameElapsedTime = debug_startTime;
            hasStarted = true;
        }

        ballPosition.x = ballMinX + (1 - padPositionX) * (ballMaxX - ballMinX);
        ballPosition.y = ballBottomY;
        ballPosition.z = ballMinZ + (1 - padPositionZ) * ((ballMaxZ+cameraWallOffset) - ballMinZ);
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

            // Synchronize ball with music
            if (currentOrigin == BOTTOM && currentDestination == BOTTOM) {
                ballYCoord = ballBottomY;
            } else if (currentOrigin == TOP && currentDestination == TOP) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance;
            } else if (currentDestination == BOTTOM) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance * (1 - fractionFrameComplete);
            } else if (currentDestination == TOP) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance * fractionFrameComplete;
            }

            // Make ball move
            const float ballSpeed = 60.0f;
            ballPosition.x += timeDelta * ballSpeed * ballDirection.x;
            ballPosition.y = ballYCoord;
            ballPosition.z += timeDelta * ballSpeed * ballDirection.z;

            // Make ball bounce
            if (ballPosition.x < ballMinX) {
                ballPosition.x = ballMinX;
                ballDirection.x *= -1;
            } else if (ballPosition.x > ballMaxX) {
                ballPosition.x = ballMaxX;
                ballDirection.x *= -1;
            }
            if (ballPosition.z < ballMinZ) {
                ballPosition.z = ballMinZ;
                ballDirection.z *= -1;
            } else if (ballPosition.z > ballMaxZ) {
                ballPosition.z = ballMaxZ;
                ballDirection.z *= -1;
            }

            if(options.enableAutoplay) {
                padPositionX = 1-(ballPosition.x - ballMinX) / (ballMaxX - ballMinX);
                padPositionZ = 1-(ballPosition.z - ballMinZ) / ((ballMaxZ+cameraWallOffset) - ballMinZ);
            }

            // Check if the ball is hitting the pad when the ball is at the bottom.
            // If not, you just lost the game! (hehe)
            if (jumpedToNextFrame && currentOrigin == BOTTOM && currentDestination == TOP) {
                double padLeftX  = boxNode->position.x - (boxDimensions.x/2) + (1 - padPositionX) * (boxDimensions.x - padDimensions.x);
                double padRightX = padLeftX + padDimensions.x;
                double padFrontZ = boxNode->position.z - (boxDimensions.z/2) + (1 - padPositionZ) * (boxDimensions.z - padDimensions.z);
                double padBackZ  = padFrontZ + padDimensions.z;

                singleScore += 1;

                if (   ballPosition.x < padLeftX
                    || ballPosition.x > padRightX
                    || ballPosition.z < padFrontZ
                    || ballPosition.z > padBackZ
                ) {
                    hasLost = true;
                    if (options.enableMusic) {
                        sound->stop();
                        delete sound;
                    }
                    singleScore -= 1;
                    // textNode->vertexArrayObjectID = start;

                    // Read old high score from external score
                    std::ifstream in_HS;
                    in_HS.open("../res/highscores.txt");
                    std::string line;
                    std::getline(in_HS, line);
                    // std::cout << line << std::endl;
                    in_HS.close();

                    // OverWrite if new score is better than high score
                    std::ofstream out_HS;
                    out_HS.open("../res/highscores.txt", std::ios::trunc);
                    if (std::stoi(line) < singleScore) {
                        out_HS << singleScore;
                        updateScore(highScoreNode, singleScore);
                        // std::cout << line << " -> Old high score is bigger than ->" << singleScore << std::endl;
                    } else {
                        updateScore(highScoreNode, std::stoi(line));
                        out_HS << line;
                    }
                    } 
                    out_HS.close();
                }
            }
        }
    }
    glm::vec3 cameraPosition = glm::vec3(0, 2, -20);



    // Some math to make the camera move in a nice way
    float lookRotation = -0.6 / (1 + exp(-5 * (padPositionX-0.5))) + 0.3;
    glm::mat4 cameraTransform =
                    glm::rotate(0.3f + 0.2f * float(-padPositionZ*padPositionZ), glm::vec3(1, 0, 0)) *
                    glm::rotate(lookRotation, glm::vec3(0, 1, 0)) *
                    glm::translate(-cameraPosition);

    // Move and rotate various SceneNodes
    boxNode->position = { 0, -10, -80 };

    ballNode->position = ballPosition;
    ballNode->scale = glm::vec3(ballRadius);
    ballNode->rotation = { 0, totalElapsedTime*2, 0 };

    padNode->position  = {
        boxNode->position.x - (boxDimensions.x/2) + (padDimensions.x/2) + (1 - padPositionX) * (boxDimensions.x - padDimensions.x),
        boxNode->position.y - (boxDimensions.y/2) + (padDimensions.y/2),
        boxNode->position.z - (boxDimensions.z/2) + (padDimensions.z/2) + (1 - padPositionZ) * (boxDimensions.z - padDimensions.z)
    };

    // Puts together the view and projection for the shader
    glm::mat4 projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 350.f);
    P = projection;
    V = cameraTransform;

    // Gives the position of the ball for shadows (or light nodes if chosen)
    glUniform3fv(9, 1, glm::value_ptr(ballNode->position));

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
    glUniformMatrix3fv(8, 1, GL_FALSE, glm::value_ptr(node->position));

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
                    glUniform1i(10, 0); // 3D geometry
                    glUniform1i(11, 0); // normal mapped
                    glBindVertexArray(node->vertexArrayObjectID);
                    glBindTextureUnit(GL_TEXTURE_CUBE_MAP, node->textureID);
                    glUniform1i(12, 1);
                    glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
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
