#version 330 core

#define MAX_POINT_LIGHTS 32

in vec3 FragPos;
in vec3 LocalPosition;
in vec3 Normal;
in vec2 TexCoord;

in vec3 Tangent;
in vec3 Bitangent;

out vec4 frag_color;

// --------------------------------------------------
// Earth textures
// --------------------------------------------------

uniform sampler2D dayMap;
uniform sampler2D nightMap;
uniform sampler2D specularMap;
uniform sampler2D normalMap;
uniform sampler2D damageMap;
uniform sampler2D heatMap;
uniform float destructionLevel;


// --------------------------------------------------
// Camera
// --------------------------------------------------

uniform vec3 viewPos;


// --------------------------------------------------
// Directional light
// --------------------------------------------------

uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform float sunIntensity;


// --------------------------------------------------
// Point lights
// --------------------------------------------------

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

uniform PointLight pointLights[MAX_POINT_LIGHTS];


// --------------------------------------------------
// Main
// --------------------------------------------------

// Deterministic 3D cellular field sampled on the local sphere. No UV seam or
// pole singularity, and no world-space/time input that could make cracks slide.
vec3 crackHash(vec3 cell)
{
    return fract(sin(vec3(dot(cell, vec3(127.1, 311.7, 74.7)),
                          dot(cell, vec3(269.5, 183.3, 246.1)),
                          dot(cell, vec3(113.5, 271.9, 124.6)))) * 43758.5453);
}

float crustCracks(float level, float nearbyDamage)
{
    if (level <= 0.001) return 0.0;
    vec3 p = normalize(LocalPosition) * 8.0;
    vec3 cell = floor(p);
    vec3 local = fract(p);
    float nearest = 100.0, second = 100.0;
    vec3 nearestCell = cell;
    for (int z = -1; z <= 1; ++z)
    for (int y = -1; y <= 1; ++y)
    for (int x = -1; x <= 1; ++x)
    {
        vec3 offset = vec3(x, y, z);
        vec3 candidate = cell + offset;
        vec3 delta = offset + 0.2 + 0.6 * crackHash(candidate) - local;
        float distanceSquared = dot(delta, delta);
        if (distanceSquared < nearest) {
            second = nearest; nearest = distanceSquared; nearestCell = candidate;
        } else second = min(second, distanceSquared);
    }
    float edge = sqrt(second) - sqrt(nearest);
    // Early fractures stay near damaged patches; later the network spreads globally.
    float coverage = max(smoothstep(0.18, 0.8, level),
                         nearbyDamage * smoothstep(0.03, 0.22, level));
    float threshold = crackHash(nearestCell).x * 0.8;
    float activation = smoothstep(threshold, threshold + 0.15, coverage);
    float width = mix(0.008, 0.09, level * level);
    float aa = max(fwidth(edge), 0.001);
    return (1.0 - smoothstep(width, width + aa, edge)) * activation * coverage;
}

void main()
{
    // ------------------------------------------------
    // TBN
    // ------------------------------------------------

    vec3 N = normalize(Normal);
    vec3 T = normalize(Tangent);
    vec3 B = normalize(Bitangent);

    mat3 TBN = mat3(T, B, N);


    // ------------------------------------------------
    // NORMAL MAP
    // ------------------------------------------------

    vec3 normalMapValue =
        texture(normalMap, TexCoord).rgb;

    vec3 normalTangent = normalMapValue * 2.0 - 1.0;

    normalTangent.xy *= 5.0;

    normalTangent = normalize(normalTangent);

    vec3 normal = normalize(TBN * normalTangent);


    // ------------------------------------------------
    // VIEW
    // ------------------------------------------------

    vec3 viewDir = normalize(viewPos - FragPos);


    // ------------------------------------------------
    // SUN
    // ------------------------------------------------

    vec3 lightDir = normalize(-sunDirection);

    float NdotL = dot(normal, lightDir);

    float dayFactor = smoothstep(-0.08, 0.08, NdotL);

    // ------------------------------------------------
    // EARTH TEXTURES
    // ------------------------------------------------

    vec3 dayColor = texture(dayMap, TexCoord).rgb;

    vec3 nightColor = texture(nightMap, TexCoord).rgb;


    vec3 color =
        mix(
            nightColor,
            dayColor,
            dayFactor
        );


    // ------------------------------------------------
    // SUN SPECULAR
    // ------------------------------------------------

    vec3 halfDir =
        normalize(lightDir + viewDir);

    float specAngle =
        max(
            dot(normal, halfDir),
            0.0
        );

    float specularStrength =
        pow(specAngle, 64.0);

    float specularMapValue =
        texture(specularMap, TexCoord).r;

    vec3 sunSpecular =
        sunColor
        *
        specularStrength
        *
        specularMapValue
        *
        dayFactor
        *
        sunIntensity;

    color += sunSpecular;


    // ------------------------------------------------
    // POINT LIGHTS
    // ------------------------------------------------

    for (int i = 0; i < min(pointLightCount, MAX_POINT_LIGHTS); ++i)
    {
        PointLight light = pointLights[i];


        vec3 toLight =
            light.position - FragPos;

        float distance =
            length(toLight);

        vec3 pointLightDir =
            normalize(toLight);


        // Diffuse

        float NdotPoint =
            max(
                dot(normal, pointLightDir),
                0.0
            );


        // Attenuation

        float attenuation =
            1.0 /
            (
                light.constant +
                light.linear * distance +
                light.quadratic *
                distance *
                distance
            );


        // Diffuse contribution

        vec3 pointDiffuse =
            dayColor
            *
            NdotPoint
            *
            light.color
            *
            light.intensity
            *
            attenuation;


        // Specular

        vec3 pointHalfDir =
            normalize(
                pointLightDir +
                viewDir
            );

        float pointSpecAngle =
            max(
                dot(normal, pointHalfDir),
                0.0
            );

        float pointSpecularStrength =
            pow(
                pointSpecAngle,
                64.0
            );


        vec3 pointSpecular =
            light.color
            *
            pointSpecularStrength
            *
            specularMapValue
            *
            light.intensity
            *
            attenuation;


        color +=
            pointDiffuse +
            pointSpecular;
    }


    // ------------------------------------------------
    // OUTPUT
    // ------------------------------------------------

    // Persistent local-UV scorch mask dims surface, city lights and specular.
    float damage = texture(damageMap, TexCoord).r;
    color *= mix(vec3(1.0), vec3(0.12, 0.09, 0.07), damage);

    // Emission is added after lighting and scorch: visible even on the night side.
    float heat = max(texture(heatMap, TexCoord).r, 0.0);
    vec3 thermal = mix(vec3(1.0, 0.025, 0.002), vec3(1.0, 0.28, 0.015),
        smoothstep(0.05, 0.65, heat));
    thermal = mix(thermal, vec3(1.0, 0.8, 0.12), smoothstep(0.65, 1.5, heat));
    thermal = mix(thermal, vec3(1.0, 0.98, 0.85), smoothstep(1.5, 2.8, heat));
    color += thermal * heat;

    float level = clamp(destructionLevel, 0.0, 1.0);
    // Extend the influence slightly beyond each permanent damage patch.
    float nearbyDamage = damage;
    nearbyDamage = max(nearbyDamage, texture(damageMap, TexCoord + vec2(0.015, 0.0)).r);
    nearbyDamage = max(nearbyDamage, texture(damageMap, TexCoord - vec2(0.015, 0.0)).r);
    nearbyDamage = max(nearbyDamage, texture(damageMap, TexCoord + vec2(0.0, 0.015)).r);
    nearbyDamage = max(nearbyDamage, texture(damageMap, TexCoord - vec2(0.0, 0.015)).r);
    float cracks = crustCracks(level, nearbyDamage);
    vec3 crackColor = mix(vec3(1.0, 0.045, 0.005), vec3(1.0, 0.4, 0.025),
                         smoothstep(0.15, 0.55, level));
    crackColor = mix(crackColor, vec3(1.0, 0.95, 0.7), smoothstep(0.55, 0.95, level));
    // Dark crust and exposed emissive interior, independent of external lights.
    color *= 1.0 - 0.65 * cracks;
    color += crackColor * cracks * mix(0.5, 5.0, level * level);

    frag_color =
        vec4(color, 1.0);
}
