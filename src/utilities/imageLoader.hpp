#pragma once

#include "lodepng.h"
#include <vector>
#include <string>
#include <glad/glad.h>

typedef struct PNGImage {
	unsigned int width;
	unsigned int height;
	std::vector<unsigned char> pixels;
	GLuint ID;
} PNGImage;

PNGImage loadPNGFile(std::string fileName);

void GenTextures(PNGImage *image);

