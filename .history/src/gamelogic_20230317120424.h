#pragma once

#include <utilities/window.hpp>
#include "sceneGraph.hpp"
#include <stb_image.h>

void updateNodeTransformations(SceneNode* node, glm::mat4 transformationThusFar);
void initGame(GLFWwindow* window, CommandLineOptions options);
void updateFrame(GLFWwindow* window);
void renderFrame(GLFWwindow* window);
unsigned char stbi_load(faces[i].c_str(), float width, float height, &nrChannels, 0);