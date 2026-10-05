#version 330 core
in vec2 TexCoord;
uniform sampler2D surfaceMap;
uniform float exhaustTime, exhaustPhase;
layout(location = 0) out vec4 color;
layout(location = 1) out vec4 bloomSource;

void main()
{
    float along = 1.0 - TexCoord.y;
    float time = exhaustTime;
    // Warp the existing silhouette without wrapping its transparent borders.
    vec2 uv = TexCoord;
    uv.x += 0.018 * along * sin(along * 24.0 - time * 15.0 + exhaustPhase);
    uv.y += 0.012 * sin(along * 32.0 - time * 19.0 + exhaustPhase) * along * (1.0 - along);
    if (any(lessThan(uv, vec2(0))) || any(greaterThan(uv, vec2(1))))
    {
        discard;
    }
    vec4 flame = texture(surfaceMap, uv);
    // Traveling bands move from the nozzle (v=1) toward the tail (v=0).
    float flow = 0.78 + 0.22 * sin(along * 42.0 - time * 22.0 + exhaustPhase +
                                   2.0 * sin(uv.x * 18.0 + along * 11.0 - time * 7.0));
    float fade = 1.0 - smoothstep(0.72, 1.0, along);
    float alpha = flame.a * fade * 0.48;
    if (alpha < 0.003)
    {
        discard;
    }
    vec3 emission = flame.rgb * (1.5 + 0.3 * sin(time * 17.0 + exhaustPhase)) * flow;
    color = vec4(emission, alpha);
    bloomSource = vec4(emission * 1.8, alpha);
}
