#version 430 core

in layout(location = 0) vec3 position;
in layout(location = 1) vec3 normal_in;
in layout(location = 2) vec2 textureCoordinates_in;
in layout(location = 3) vec3 tangent_in;
in layout(location = 4) vec3 bitangent_in;

uniform layout(location = 3) mat4 M;
uniform layout(location = 4) mat4 VP;

uniform layout(location = 8) mat3 normal_trick;
uniform layout(location = 10) int renderText;

layout(binding = 2) uniform samplerCube cubePos;

out layout(location = 0) vec3 normal_out;
out layout(location = 1) vec2 textureCoordinates_out;
out layout(location = 2) vec3 vertexPos;
out layout(location = 3) mat3 TBN_out;

void main()
{   //frag - ikke legg på projection med tekst
    TBN_out = transpose(mat3(
        normal_trick * normalize(tangent_in),
        normal_trick * normalize(bitangent_in),
        -normal_trick * normalize(normal_in)
    ));

    // Uses the newly calculated normal matrix to tranform the old normal
    normal_out = normalize(normal_trick * normal_in);
    textureCoordinates_out = textureCoordinates_in;

    if(renderText != 0) {
        gl_Position = M * vec4(position, 1.0f);
    } else {
        // global vertex position found by using the model and the position from the VAO
        vertexPos = (M * vec4(position, 1.0f)).xyz;
        gl_Position = VP * M * vec4(position, 1.0f);
    }
}
