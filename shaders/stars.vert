#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;

void main()
{
    TexCoord = texCoord;

    vec4 clipPosition =
        projection *
        view *
        model *
        vec4(pos, 1.0);

    // Toujours au fond de la profondeur
    clipPosition.z = clipPosition.w;

    gl_Position = clipPosition;
}