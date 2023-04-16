#version 430 core

in layout(location = 0) vec3 position;
in layout(location = 1) vec3 normal_in;
in layout(location = 2) vec2 textureCoordinates_in;

uniform layout(location = 3) mat4 M;
uniform layout(location = 4) mat4 V;
uniform layout(location = 5) mat4 P;

out layout(location = 0) vec3 normal_out;
out layout(location = 1) vec2 textureCoordinates_out;
out layout(location = 2) vec3 worldPos;

void main()
{   //frag - ikke legg på projection med tekst
    normal_out = normalize(normal_in);
    textureCoordinates_out = textureCoordinates_in;

    // wrap dråpe textur rundt istapp objectet some textur, den følger istappen dynamisk, kan legge til flere på den måten 
    // (ikke mulig med dråper på siden uten geometry)
    
    // global vertex position found by using the model and the position from the VAO
    //1d texture map - høyde langs istappen (radius)
    vec3 pos = position;// + sinus(textureCoordinates_in)
    worldPos = (M * vec4(pos, 1.0f)).xyz;
    gl_Position = P * V * M * vec4(pos, 1.0f);
}
