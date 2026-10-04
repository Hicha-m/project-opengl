#version 330 core
in vec3 FragPos, Normal;
in vec2 TexCoord;
uniform sampler2D ringMap;
uniform vec3 sunPosition;
layout(location=0) out vec4 color;
layout(location=1) out vec4 bloomSource;
void main() {
    vec4 ring=texture(ringMap,TexCoord);
    if(ring.a<0.02) discard;
    float diffuse=abs(dot(normalize(Normal),normalize(sunPosition-FragPos)));
    color=vec4(ring.rgb*(0.3+0.7*diffuse),ring.a);
    bloomSource=vec4(0,0,0,ring.a);
}
