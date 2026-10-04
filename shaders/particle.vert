#version 330 core
layout(location = 0) in vec4 positionSize;
layout(location = 1) in float opacity;
uniform mat4 view;
uniform mat4 projection;
out vec2 uv;
out float alpha;
void main()
{
    vec2 corners[4] = vec2[](vec2(-1,-1), vec2(1,-1), vec2(-1,1), vec2(1,1));
    uv = corners[gl_VertexID];
    vec4 center = view * vec4(positionSize.xyz, 1.0);
    center.xy += uv * positionSize.w * 0.5;
    gl_Position = projection * center;
    alpha = opacity;
}
