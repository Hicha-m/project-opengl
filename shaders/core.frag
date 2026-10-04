#version 330 core
in vec3 FragPos, Normal;
uniform vec3 viewPos;
uniform float emission;
layout(location=0) out vec4 frag_color;
layout(location=1) out vec4 bloomSource;
void main() {
    bloomSource = vec4(0,0,0,1);
    float facing = clamp(dot(normalize(Normal),normalize(viewPos-FragPos)),0.0,1.0);
    vec3 thermal = mix(vec3(1,0.3,0.025),vec3(1,0.97,0.85),smoothstep(0.0,0.7,facing));
    frag_color = vec4(thermal*emission,1);
}
