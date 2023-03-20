#version 430 core

in layout(location = 2) vec3 vertexPos;

uniform layout(binding = 2) samplerCube cubemap;

out vec4 colorOut;

void main()
{ 
    colorOut.rgba = texture(cubemap, normalize(vertexPos));
}