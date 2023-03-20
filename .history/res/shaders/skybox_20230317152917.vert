#version 430 core

in layout(location = 0) vec3 position;

uniform layout(location = 3) mat4 M;
uniform layout(location = 4) mat4 V;
uniform layout(location = 5) mat4 P;

out layout(location = 2) vec3 vertexPos;

void main()
{   //frag - ikke legg på projection med tekst
    // global vertex position found by using the model and the position from the VAO
    vertexPos = (M * vec4(position, 1.0f)).xyz;
    gl_Position = P * V * vec4(position, 1.0f);
}
