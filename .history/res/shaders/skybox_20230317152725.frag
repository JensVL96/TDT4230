#version 430 core


in layout(location = 2) vec3 vertexPos;

uniform layout(binding = 2) samplerCube cubemap;

out vec4 colorOut;

vec4 colorOut.rgba = texture(cubemap, vertexPos);
// vec3 normalMap = texture(normalMapTexture, textureCoordinates).xyz*vec3(1, 1, -1) * 2 - 1;

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
        // if (hasTexture != 0) {
        //     NN = TBN * normalize(normalMap);
        // } else {
        //     NN = normalize(normal);
        // }
        NN = normalize(normal);

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


            // shadows hard_light
            vec3 fragToBall = ballPos - vertexPos;
            vec3 fragToLight = lightPos - vertexPos;
            vec3 rejection = reject(fragToBall, fragToLight);
            // rules for casting a shadow
            bool castShadow = length(rejection) < radius                        // light only when the recjection vector is smaller than the radius
                        && length(fragToLight) > length(fragToBall) - radius    // light source closer to fragment than ball (minus the radius of the ball)
                        && dot(fragToLight, fragToBall) > 0.0;                  // vectors shouldn't point in opposite directions


            // soft edge on shadows
            float hard_light = length(rejection);
            hard_light = (hard_light - light_soft_inner) / (light_soft_outer - light_soft_inner);
            hard_light = max(0, hard_light);
            hard_light = min(1, hard_light);
            // Rules for when to apply soft shadow
            if (length(fragToLight) < length(fragToBall) - radius || dot(fragToLight, fragToBall) <= 0.0)
                hard_light = 1.0;


            // Applies diffuse, specular, light intensity and soft/ hard light when it doesn't apply shadows
            if(!castShadow) {
                finalColor += diffuseIntensity * diffuse * L * hard_light;
                finalColor += specularIntensity * specular * L * hard_light;
            }
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