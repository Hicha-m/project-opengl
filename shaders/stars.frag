#version 330 core

in vec2 TexCoord;

out vec4 fragColor;

uniform sampler2D starMap;


void main()
{

    vec3 color =
    texture(starMap, TexCoord).rgb;

    color *= 2.0;
    fragColor = vec4(color, 1.0);

}