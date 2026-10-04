#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

layout(location=0) out vec4 frag_color;
layout(location=1) out vec4 bloomSource;

uniform sampler2D cloudMap;
uniform vec3 sunDirection;

void main()
{
    bloomSource = vec4(0,0,0,1);
    vec3 normal =
        normalize(Normal);

    vec3 lightDir =
        normalize(-sunDirection);

    float NdotL =
        dot(normal, lightDir);

    // Les nuages sont visibles principalement
    // sur la face éclairée.
    float dayFactor =
        smoothstep(-0.05, 0.20, NdotL);

    vec4 cloud =
        texture(cloudMap, TexCoord);

    float alpha =
        cloud.r * dayFactor;

    if (alpha < 0.03)
        discard;

    frag_color =
        vec4(
            1.0,
            1.0,
            1.0,
            alpha
        );
}