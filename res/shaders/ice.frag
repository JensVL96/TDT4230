/* #version 430 core

in layout(location = 0) vec3 normal;
in layout(location = 1) vec2 textureCoordinates;
in layout(location = 2) vec3 worldPos;

uniform layout(location = 11) int hasTexture;

uniform layout(binding = 2) samplerCube cubemap;
uniform layout(binding = 3) sampler2D waterDrop;


vec4 diffuseTextureColor = texture(waterDrop, textureCoordinates);
vec3 diffuse = diffuseTextureColor.xyz;

struct LightSource {
    vec3 position;
    vec3 color;
};
uniform LightSource LightSources[1];

// Default camera angle
vec3 ambient = vec3(0.1);
vec3 emission = vec3(0.0);
vec3 specular = vec3(1.0);
vec3 cameraPos = vec3(0.0);

float shininess = 32.0;

// Variables for the light intensity (attenuation)
float la = 1.0;
float lb = 0.007;
float lc = 0.00013;

out vec4 colorOut;

void main()
{ 
    vec3 tempColor = vec3(0.0);
    float ratio = 1.00 / 1.309;

    vec3 NN = normalize(normal);
    vec3 lightPos = LightSources[0].position;
    vec3 lightDirection = normalize(lightPos - worldPos);
    vec3 specularDirection = normalize(cameraPos - worldPos);

    // Light intensity
    float d = length(lightPos - worldPos);
    // float L = (1 / (la + d * lb + d*d * lc));                        // Without color
    vec3 L = (1 / (la + d * lb + d*d * lc)) * LightSources[0].color;    // With color

    // diffuse
    float diffuseIntensity = max(dot(lightDirection, NN), 0.0);

    // specular
    vec3 reflected = reflect(-lightDirection, NN);
    float specularIntensity = pow(max(dot(reflected, specularDirection), 0.0), shininess);

    // model space coordinates, inn og ut med sinus kurve

    vec3 view = normalize(worldPos - cameraPos);
    vec3 refraction = refract(view, normalize(normal), ratio);

    tempColor.rgb += diffuseIntensity * diffuse * L;
    tempColor.rgb += specularIntensity * specular * L;

    tempColor.rgb += ambient;
    tempColor.rgb += emission;
    
    // colorOut = vec4(tempColor, 1.0);
    colorOut.rgba = vec4(texture(cubemap, refraction).rgb, 1.0);
    // colorOut = vec4(1.0f);
} */


#version 430 core

in layout(location = 0) vec3 normal;
in layout(location = 1) vec2 textureCoordinates;
in layout(location = 2) vec3 worldPos;

uniform layout(location = 10) int renderText;
uniform layout(location = 11) int hasTexture;

uniform layout(binding = 2) samplerCube cubemap;
uniform layout(binding = 0) sampler2D waterDrop;

out vec4 colorOut;

float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float dither(vec2 uv) { return (rand(uv)*2.0-1.0) / 256.0; }
vec3 reject(vec3 from, vec3 onto) { return from - onto*dot(from, onto)/dot(onto, onto); }

struct LightSource {
    vec3 position;
    vec3 color;
};
uniform LightSource LightSources[3];

vec4 diffuseTextureColor = texture(waterDrop, textureCoordinates);

// Phong light components
vec3 ambient = vec3(0.1);
vec3 diffuse = (hasTexture != 0) ? diffuseTextureColor.xyz : vec3(1.0);;
vec3 specular = vec3(1.0);
vec3 emission = vec3(0.0);


// Default camera angle
vec3 cameraPos = vec3(0.0);

// Variables for light and shadows
float shininess = 64.0;
float ratio = 1.00 / 1.309;
float alpha = 1.0;

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
        vec3 NN = normalize(normal);

        for(int i = 0; i < 3; i++) {
            // Light pos and vector
            vec3 lightPos = LightSources[i].position;
            vec3 lightDirection = normalize(lightPos - worldPos);
            vec3 specularDirection = normalize(cameraPos - worldPos);


            // Light intensity
            float d = length(lightPos - worldPos);
            // float L = (1 / (la + d * lb + d*d * lc));                        // Without color
            vec3 L = (1 / (la + d * lb + d*d * lc)) * LightSources[i].color;    // With color


            // diffuse
            float diffuseIntensity = max(dot(lightDirection, NN), 0.0);


            // specular
            vec3 reflected = reflect(-lightDirection, NN);
            float specularIntensity = pow(max(dot(reflected, specularDirection), 0.0), shininess);

            vec3 view = normalize(worldPos - cameraPos);
            vec3 refraction = refract(view, normalize(normal), ratio);
            
            // colorOut = vec4(tempColor, 1.0);

            // Applies diffuse, specular, light intensity and soft/ hard light when it doesn't apply shadows
            
            finalColor += diffuseIntensity * diffuse * L;
            finalColor += specularIntensity * specular * L;
            finalColor += texture(cubemap, refraction).rgb;
        }

        // Adds extra components to the final ligth color
        finalColor += ambient;
        finalColor += emission;
        finalColor += dither(textureCoordinates);   // Noice

        // Resulting color
        colorOut = vec4(finalColor, alpha);
        // colorOut.rgba = vec4(texture(cubemap, refraction).rgb, 1.0);
        //colorOut = vec4(NN * 0.5 + 0.5 , 1.0);
    }

}