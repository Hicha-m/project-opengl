#version 330 core

in vec2 TexCoord;

out vec4 frag_color;

uniform sampler2D starMap;


void main()
{

    vec3 color =
    texture(starMap, TexCoord).rgb;

    color *= 2.0;
    frag_color = vec4(color, 1.0);

}