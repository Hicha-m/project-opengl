#version 330 core
in vec2 uv;
uniform sampler2D source;
uniform vec2 axis;
out vec4 color;
void main() {
    vec2 stepUV = axis/vec2(textureSize(source,0));
    vec3 sum = texture(source,uv).rgb*0.227027;
    float weights[4] = float[](0.194595,0.121622,0.054054,0.016216);
    for(int i=1;i<=4;++i)
        sum += (texture(source,uv+stepUV*i).rgb+texture(source,uv-stepUV*i).rgb)*weights[i-1];
    color = vec4(sum,1);
}
