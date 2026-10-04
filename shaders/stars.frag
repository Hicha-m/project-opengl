#version 330 core

in vec2 TexCoord;

layout(location=0) out vec4 frag_color;
layout(location=1) out vec4 bloomSource;

uniform sampler2D starMap;


void main()
{
    bloomSource = vec4(0,0,0,1);

    vec3 color =
    texture(starMap, TexCoord).rgb;

    color *= 2.0;
    frag_color = vec4(color, 1.0);

}