#version 330 core
in vec2 uv;
in float alpha;
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 bloomSource;
void main()
{
    bloomSource = vec4(0, 0, 0, 1);
    float radius = length(uv);
    if (radius >= 1.0)
    {
        discard;
    }
    float glow = 1.0 - smoothstep(0.15, 1.0, radius);
    fragColor = vec4(1.0, 0.55, 0.16, alpha * glow);
}
