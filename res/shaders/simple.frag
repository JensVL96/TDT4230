#version 430 core

in layout(location = 0) vec3 normal;
in layout(location = 1) vec2 textureCoordinates;
in layout(location = 2) vec3 vertexPos;
in layout(location = 3) mat3 TBN;

uniform layout(location = 10) int renderText;
uniform layout(location = 11) int hasTexture;

uniform layout(binding = 0) sampler2D diffuseTexture;
uniform layout(binding = 1) sampler2D normalMapTexture;

// uniform layout(location = 12) int isSkybox;
// uniform layout(binding = 2) samplerCube cubemap;

out vec4 colorOut;

float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float dither(vec2 uv) { return (rand(uv)*2.0-1.0) / 256.0; }
vec3 reject(vec3 from, vec3 onto) { return from - onto*dot(from, onto)/dot(onto, onto); }

// vec3 light[3] = {lightPos1.xyz, lightPos2.xyz, lightPos3.xyz};   //TODO remove connecting variables

struct LightSource {
    vec3 position;
    vec3 color;
};
uniform LightSource LightSources[3];

vec4 diffuseTextureColor = texture(diffuseTexture, textureCoordinates);
vec3 normalMap = texture(normalMapTexture, textureCoordinates).xyz*vec3(1, 1, -1) * 2 - 1;

// Phong light components
vec3 ambient = vec3(0.1);
vec3 diffuse = (hasTexture != 0) ? diffuseTextureColor.xyz : vec3(1.0);;
vec3 specular = vec3(1.0);
vec3 emission = vec3(0.0);


// Default camera angle
vec3 cameraPos = vec3(0.0);

// Variables for light and shadows
float shininess = 32.0;
float radius = 3.0;
float alpha = 1.0;
float softEdge = 1.2;

// Size of soft shadow edge
//float light_soft_inner = radius / softEdge;
float light_soft_inner = radius;
float light_soft_outer = radius * softEdge;

// Variables for the light intensity (attenuation)
float la = 1.0;
float lb = 0.007;
float lc = 0.00013;

void main()
{
    // Default color set to black
    vec3 finalColor = vec3(0.0);


    if(renderText != 0) {   // 2D
        colorOut = diffuseTextureColor;
    } else {                // 3D
        // Normalizes the normal vector from the vertex shader or the normal map texture
        vec3 NN;
        if (hasTexture != 0) {
            NN = TBN * normalize(normalMap);
        } else {
            NN = normalize(normal);
        }

        for(int i = 0; i < 3; i++) {
            // Light pos and vector
            vec3 lightPos = LightSources[i].position;
            vec3 lightDirection = normalize(lightPos - vertexPos);
            vec3 specularDirection = normalize(cameraPos - vertexPos);


            // Light intensity
            float d = length(lightPos - vertexPos);
            // float L = (1 / (la + d * lb + d*d * lc));                        // Without color
            vec3 L = (1 / (la + d * lb + d*d * lc)) * LightSources[i].color;    // With color


            // diffuse
            float diffuseIntensity = max(dot(lightDirection, NN), 0.0);


            // specular
            vec3 reflected = reflect(-lightDirection, NN);
            float specularIntensity = pow(max(dot(reflected, specularDirection), 0.0), shininess);

            // Applies diffuse, specular, light intensity and soft/ hard light when it doesn't apply shadows
            
            finalColor += diffuseIntensity * diffuse * L;
            finalColor += specularIntensity * specular * L;
        }

        // Adds extra components to the final ligth color
        finalColor += ambient;
        finalColor += emission;
        finalColor += dither(textureCoordinates);   // Noice

        // Resulting color
        colorOut = vec4(finalColor, alpha);
        //colorOut = vec4(NN * 0.5 + 0.5 , 1.0);
    }

}