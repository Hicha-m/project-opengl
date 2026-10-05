#version 330 core
in vec2 uv;
uniform sampler2D scene, bloom;
uniform int bloomEnabled;
out vec4 color;
void main()
{
    vec3 hdr = texture(scene, uv).rgb;
    if (bloomEnabled != 0)
    {
        hdr += texture(bloom, uv).rgb * 0.12;
    }
    // Existing textures/shaders use display-referred colors. Preserve ordinary
    // values and roll off only highlights; another gamma lift washes out shadows.
    hdr = max(hdr, vec3(0));
    vec3 base = min(hdr, vec3(0.8));
    vec3 highlights = max(hdr - vec3(0.8), vec3(0));
    vec3 mapped = base + 0.2 * (vec3(1) - exp(-highlights / 0.2));
    color = vec4(mapped, 1);
}
