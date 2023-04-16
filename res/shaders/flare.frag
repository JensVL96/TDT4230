#version 430 core

in layout(location = 2) vec2 textureCoords;

uniform layout(binding = 0) sampler2D diffuse;

out vec4 colorOut;

void main()
{            
    colorOut = texture(diffuse, textureCoords);
    //colorOut = vec4(textureCoords, 0, 1.0);
}