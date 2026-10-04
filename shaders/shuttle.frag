#version 330 core
in vec3 FragPos, Normal, Tangent;
in vec2 TexCoord;
uniform sampler2D surfaceMap, normalMap;
uniform int hasSurfaceMap, hasNormalMap;
uniform vec3 baseColor, sunPosition, viewPos;
uniform float metallic, roughness;
layout(location=0) out vec4 color;
layout(location=1) out vec4 bloomSource;
void main() {
    vec3 n=normalize(Normal), t=Tangent-n*dot(Tangent,n);
    if(hasNormalMap!=0 && length(t)>0.001) {
        t=normalize(t);
        n=normalize(mat3(t,cross(n,t),n)*(texture(normalMap,TexCoord).rgb*2.0-1.0));
    }
    vec3 base=baseColor;
    if(hasSurfaceMap!=0) {
        vec4 surface=texture(surfaceMap,TexCoord);
        // The supplied exhaust sprites contain transparent backgrounds.
        if(surface.a<0.1) discard;
        base*=surface.rgb;
    }
    vec3 light=normalize(sunPosition-FragPos), view=normalize(viewPos-FragPos);
    float diffuse=max(dot(n,light),0), fill=max(dot(n,view),0);
    float specular=pow(max(dot(n,normalize(light+view)),0),mix(64.0,8.0,roughness));
    // Gentle camera fill keeps the nose readable when the Sun is behind the craft.
    color=vec4(base*(0.4+0.65*diffuse+0.65*fill)+specular*mix(0.08,0.3,metallic),1);
    bloomSource=vec4(0,0,0,1);
}
