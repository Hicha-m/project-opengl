#version 330 core

in vec2 TexCoord;

layout(location=0) out vec4 frag_color;
layout(location=1) out vec4 bloomSource;

uniform sampler2D sunMap;
uniform float emission;
uniform float bloomEmission;

void main()
{
    bloomSource = vec4(0,0,0,1);
    vec3 color =
        texture(sunMap, TexCoord).rgb;

    // Emission :
    // la texture représente directement
    // la lumière émise par la surface.
    bloomSource = vec4(vec3(1.0,0.65,0.18) * bloomEmission, 1);
    color *= emission;

    frag_color =
        vec4(color, 1.0);
}