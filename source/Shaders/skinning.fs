#version 330 core

in  vec2 TexCoord;
in  vec4 to_light;
out vec4 FragColor;

uniform sampler2D texture1;
uniform int       uMode;
uniform float     Alpha;
uniform float     uAlphaCutoff = 0.05;

void main()
{
    if (uMode == 3)
    {
        FragColor = vec4(to_light.rgb, Alpha);
        return;
    }

    vec4 t = texture(texture1, TexCoord);
    float a = t.a * Alpha;
    // Alpha-to-coverage em shader + preserva alpha (sem MSAA, sem forçar 1.0)
    float af = fwidth(a);
    float coverage = smoothstep(uAlphaCutoff - af, uAlphaCutoff + af, a);
    if (coverage <= 0.004) discard;

    if (uMode == 2)
        FragColor = vec4(t.rgb, a * coverage);
    else
        FragColor = vec4(t.rgb * to_light.rgb, a * coverage);
}
