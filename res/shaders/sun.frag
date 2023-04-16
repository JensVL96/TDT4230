#version 430 core

in layout(location = 0) vec3 normal;
in layout(location = 2) vec3 worldPos;

// layout (location = 0) out vec4 FragColor;
// layout (location = 1) out vec4 BrightColor;

out vec4 colorOut;

void main()
{            
    vec3 color = vec3(1.0);

    float shininess = 10.0;
    vec3  normalV = normalize( normal );
    vec3  eyeV    = normalize( -worldPos );
    vec3  halfV   = normalize( eyeV + normalV );
    float NdotH   = max( 0.0, dot( normalV, halfV ) );
    float glowFac = ( shininess + 2.0 ) * pow( NdotH, shininess ) / ( 2.0 * 3.14159265 );

    colorOut = vec4( 10 * (0.1 + color.rgb * glowFac * 0.5), 1.0 );
}