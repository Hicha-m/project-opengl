#version 330 core
in vec3 FragPos, Normal;
in vec2 TexCoord;
uniform float opacity;
layout(location=0) out vec4 color;
layout(location=1) out vec4 bloomSource;
void main() {
    // Broad mesh coverage, thin antialiased center line even in distant views.
    float aa=max(fwidth(TexCoord.x),0.001);
    float coverage=1.0-smoothstep(0.025,0.025+aa,abs(TexCoord.x-0.5));
    color=vec4(0.4,0.48,0.6,0.5*opacity*coverage);
    bloomSource=vec4(0,0,0,color.a);
}
