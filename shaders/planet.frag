#version 330 core
in vec3 FragPos, Normal;
in vec2 TexCoord;
uniform sampler2D surfaceMap;
uniform vec3 sunPosition;
layout(location=0) out vec4 color;
layout(location=1) out vec4 bloomSource;
void main() {
    float diffuse=max(dot(normalize(Normal),normalize(sunPosition-FragPos)),0);
    color=vec4(texture(surfaceMap,TexCoord).rgb*(0.07+0.93*diffuse),1);
    bloomSource=vec4(0,0,0,1);
}
