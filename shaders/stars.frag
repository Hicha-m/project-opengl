#version 330 core
in vec2 TexCoord;
in vec3 SkyDirection;
layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 bloomSource;
uniform sampler2D starMap, milkyWayMap, galaxyMap;
uniform float milkyWayBlend, galaxyBlend, galaxyScale, galaxyOpacity;
void main()
{
    vec3 background =
        mix(texture(starMap, TexCoord).rgb * 2.0, texture(milkyWayMap, TexCoord).rgb * 1.5, milkyWayBlend);
    // The square galaxy asset is a distant image, not an equirectangular map.
    // Project it into a fixed sky direction so it does not follow camera rotation.
    vec3 axis = normalize(vec3(-0.287, -0.819, 0.497));
    vec3 right = normalize(cross(axis, vec3(0, 1, 0))), up = cross(right, axis);
    vec3 d = normalize(SkyDirection);
    float front = dot(d, axis);
    vec2 galaxyUV =
        vec2(dot(d, right), dot(d, up)) / max(front, 0.001) * 0.62 / max(galaxyScale, 0.001) + 0.5;
    vec3 galaxy = vec3(0);
    if (front > 0 && all(greaterThanEqual(galaxyUV, vec2(0))) && all(lessThanEqual(galaxyUV, vec2(1))))
    {
        vec2 border = min(galaxyUV, vec2(1) - galaxyUV);
        float edgeFade = smoothstep(0.0, 0.12, min(border.x, border.y));
        galaxy = texture(galaxyMap, galaxyUV).rgb * 0.65 * galaxyOpacity * edgeFade;
    }
    frag_color = vec4(mix(background, galaxy, galaxyBlend), 1);
    bloomSource = vec4(0, 0, 0, 1);
}
