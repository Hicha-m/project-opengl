#version 330 core
in vec3 FragPos, Normal;
uniform vec3 sunDirection, sunColor;
uniform float sunIntensity;
struct PointLight
{
    vec3 position;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
};
uniform int pointLightCount;
uniform PointLight pointLights[32];
layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 bloomSource;
void main()
{
    bloomSource = vec4(0, 0, 0, 1);
    vec3 n = normalize(Normal);
    if (!gl_FrontFacing)
    {
        n = -n;
    }
    vec3 rock = vec3(0.12, 0.055, 0.025);
    vec3 color = rock * (0.03 + max(dot(n, normalize(-sunDirection)), 0.0) * sunIntensity * sunColor);
    for (int i = 0; i < min(pointLightCount, 32); ++i)
    {
        vec3 delta = pointLights[i].position - FragPos;
        float d = length(delta);
        float attenuation =
            1.0 / (pointLights[i].constant + pointLights[i].linear * d + pointLights[i].quadratic * d * d);
        color += rock * pointLights[i].color * pointLights[i].intensity * attenuation *
                 max(dot(n, normalize(delta)), 0.0);
    }
    frag_color = vec4(color, 1);
}
