#version 330 core
in vec2 uv;
uniform sampler2D source, emissionSource;
out vec4 color;
void main()
{
    vec3 c = max(texture(source, uv).rgb, texture(emissionSource, uv).rgb);
    float peak = max(max(c.r, c.g), c.b);
    color = vec4(c * max(peak - 1.0, 0.0) / max(peak, 0.0001), 1);
}
