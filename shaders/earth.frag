#version 330 core

#define MAX_POINT_LIGHTS 32

in vec3 FragPos;
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

    frag_color =
        vec4(color, 1.0);
}
