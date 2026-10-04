#version 330 core
in vec3 Normal;
in vec2 TexCoord;
layout(location=0) out vec4 frag_color;
layout(location=1) out vec4 bloomSource;
uniform sampler2D rockMap;
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform float sunIntensity;
void main()
{
    bloomSource = vec4(0,0,0,1);
    float diffuse = max(dot(normalize(Normal), normalize(-sunDirection)), 0.0);
    vec3 rock = texture(rockMap, TexCoord).rgb * vec3(0.65, 0.55, 0.45);
    frag_color = vec4(rock * (vec3(0.2) + diffuse * sunColor * sunIntensity), 1.0);
}
