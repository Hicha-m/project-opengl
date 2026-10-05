#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 tangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

out vec3 Tangent;
out vec3 Bitangent;

void main()
{
    FragPos = vec3(model * vec4(pos, 1.0));

    Normal = normalize(mat3(transpose(inverse(model))) * normal);

    Tangent = normalize(mat3(model) * tangent);

    Bitangent = normalize(cross(Normal, Tangent));

    TexCoord = texCoord;

    gl_Position = projection * view * model * vec4(pos, 1.0);
}