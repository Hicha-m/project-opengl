#version 330 core

in vec2 TexCoord;

out vec4 frag_color;

uniform sampler2D sunMap;

void main()
{
    vec3 color =
        texture(sunMap, TexCoord).rgb;

    // Emission :
    // la texture représente directement
    // la lumière émise par la surface.
    color *= 2.0;

    frag_color =
        vec4(color, 1.0);
}