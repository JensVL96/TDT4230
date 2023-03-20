#version 430 core

in layout(location = 2) vec3 vertexPos;

uniform layout(binding = 2) samplerCube cubemap;

out vec4 colorOut;

colorOut.rgba = texture(cubemap, vertexPos);