#version 430 core

in layout(location = 0) vec3 position;
in layout(location = 2) vec2 textureCoordinates_in;

uniform layout(location = 0) vec3 uvz;
uniform layout(location = 1) vec2 scale;


out layout(location = 2) vec2 textureCoords;

void main()
{
    textureCoords = textureCoordinates_in;
    gl_Position = vec4(position * vec3(scale, 1.0), 1.0f) + vec4(uvz, 0);
}
