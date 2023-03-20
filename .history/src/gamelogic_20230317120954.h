#pragma once

#include <utilities/window.hpp>
#include "sceneGraph.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void updateNodeTransformations(SceneNode* node, glm::mat4 transformationThusFar);
void initGame(GLFWwindow* window, CommandLineOptions options);
void updateFrame(GLFWwindow* window);
void renderFrame(GLFWwindow* window);