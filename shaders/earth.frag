#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

in vec3 Tangent;
in vec3 Bitangent;

out vec4 frag_color;

uniform sampler2D dayMap;
uniform sampler2D nightMap;
uniform sampler2D specularMap;
uniform sampler2D normalMap;

uniform vec3 sunDirection;
uniform vec3 viewPos;

void main()
{
    // ------------------------------------------------
    // TBN
    // ------------------------------------------------

    vec3 N = normalize(Normal);
    vec3 T = normalize(Tangent);
    vec3 B = normalize(Bitangent);

    mat3 TBN =
        mat3(T, B, N);


    // ------------------------------------------------
    // NORMAL MAP
    // ------------------------------------------------

    vec3 normalMapValue =
        texture(normalMap, TexCoord).rgb;

    // [0,1] -> [-1,1]
    vec3 normalTangent =
    normalMapValue * 2.0 - 1.0;

    normalTangent.xy *= 5.0;

    normalTangent = normalize(normalTangent);

    // Normal dans l'espace monde
    vec3 normal =
        normalize(
            TBN * normalTangent
        );


    // ------------------------------------------------
    // LIGHT
    // ------------------------------------------------

    vec3 lightDir =
        normalize(-sunDirection);

    vec3 viewDir =
        normalize(viewPos - FragPos);


    // ------------------------------------------------
    // JOUR / NUIT
    // ------------------------------------------------

    float NdotL =
        dot(normal, lightDir);

    float dayFactor =
        smoothstep(-0.08, 0.08, NdotL);


    // ------------------------------------------------
    // TEXTURES
    // ------------------------------------------------

    vec3 dayColor =
        texture(dayMap, TexCoord).rgb;

    vec3 nightColor =
        texture(nightMap, TexCoord).rgb;


    vec3 color =
        mix(
            nightColor,
            dayColor,
            dayFactor
        );


    // ------------------------------------------------
    // SPECULAR
    // ------------------------------------------------

    vec3 halfDir =
        normalize(
            lightDir + viewDir
        );

    float specAngle =
        max(
            dot(normal, halfDir),
            0.0
        );

    float specularStrength =
        pow(
            specAngle,
            64.0
        );

    float specularMapValue =
        texture(
            specularMap,
            TexCoord
        ).r;

    vec3 specular =
        vec3(1.0)
        *
        specularStrength
        *
        specularMapValue;

    // Pas de reflet côté nuit
    color +=
        specular *
        dayFactor;


    // ------------------------------------------------
    // OUTPUT
    // ------------------------------------------------

    frag_color =
        vec4(color, 1.0);
}