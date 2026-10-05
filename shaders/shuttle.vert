#version 330 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 tangent;
uniform mat4 model, view, projection;
uniform int isExhaust;
uniform float exhaustTime, exhaustAnchor, exhaustPhase;
out vec3 FragPos, Normal, Tangent;
out vec2 TexCoord;
void main()
{
    vec3 animatedPos = pos;
    if (isExhaust != 0)
    {
        float along = 1.0 - texCoord.y;
        float pulse =
            sin(exhaustTime * 17.0 + exhaustPhase) * 0.06 + sin(exhaustTime * 29.0 + exhaustPhase) * 0.025;
        // Stretch the plume downstream while keeping its engine attachment fixed.
        animatedPos.z = exhaustAnchor + (pos.z - exhaustAnchor) * (1.15 + pulse);
        animatedPos.xy +=
            along * along * 0.006 *
            vec2(sin(exhaustTime * 13.0 + exhaustPhase), cos(exhaustTime * 11.0 + exhaustPhase));
    }
    vec4 world = model * vec4(animatedPos, 1);
    FragPos = world.xyz;
    Normal = transpose(inverse(mat3(model))) * normal;
    Tangent = mat3(model) * tangent;
    TexCoord = texCoord;
    gl_Position = projection * view * world;
}
