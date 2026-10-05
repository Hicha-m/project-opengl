#version 330 core
in vec3 FragPos, Normal, Tangent;
in vec2 TexCoord;
uniform sampler2D surfaceMap, normalMap;
uniform int hasSurfaceMap, hasNormalMap;
uniform vec3 baseColor, sunPosition, viewPos;
uniform float metallic, roughness;
layout(location = 0) out vec4 color;
layout(location = 1) out vec4 bloomSource;
void main()
{
    vec3 n = normalize(Normal), t = Tangent - n * dot(Tangent, n);
    if (hasNormalMap != 0 && length(t) > 0.001)
    {
        t = normalize(t);
        n = normalize(mat3(t, cross(n, t), n) * (texture(normalMap, TexCoord).rgb * 2.0 - 1.0));
    }
    vec3 base = baseColor;
    if (hasSurfaceMap != 0)
    {
        vec4 surface = texture(surfaceMap, TexCoord);
        if (surface.a < 0.1)
        {
            discard;
        }
        base *= surface.rgb;
    }
    vec3 light = normalize(sunPosition - FragPos), view = normalize(viewPos - FragPos);
    float diffuse = max(dot(n, light), 0);
    vec3 halfVector = light + view;
    halfVector /= max(length(halfVector), 0.0001);
    float specular = pow(max(dot(n, halfVector), 0), mix(64.0, 8.0, roughness));
    // Only sunlight illuminates the hull; the small ambient term preserves detail.
    // Gate highlights by incidence so the unlit side cannot shine toward the camera.
    color = vec4(base * (0.035 + 0.95 * diffuse) +
                     vec3(1.0, 0.95, 0.8) * specular * diffuse * mix(0.04, 0.18, metallic),
                 1);
    bloomSource = vec4(0, 0, 0, 1);
}
