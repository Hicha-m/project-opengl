#version 330 core
layout(location=0) in vec3 pos;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 texCoord;
layout(location=3) in vec3 tangent;
uniform mat4 model, view, projection;
out vec3 FragPos, Normal, Tangent;
out vec2 TexCoord;
void main() {
    vec4 world=model*vec4(pos,1); FragPos=world.xyz;
    Normal=transpose(inverse(mat3(model)))*normal;
    Tangent=mat3(model)*tangent; TexCoord=texCoord;
    gl_Position=projection*view*world;
}
