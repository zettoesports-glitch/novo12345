#version 330 core
// =============================================================================
// colorize.fs
// -----------------------------------------------------------------------------
// Fragment shader para colorização dinâmica.
// FIX: texture2D() removido (obsoleto), usando texture() genérico.
// FIX: interface padronizada VS_OUT.
// =============================================================================

in VS_OUT {
    vec2  TexCoord;
    vec4  BakedLight;
    vec3  WorldNormal;
    vec3  WorldPos;
} fs_in;

uniform sampler2D uTexture;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;

layout (location = 0) out vec4 FragColor;

void main()
{
    vec4 texColor = texture(uTexture, fs_in.TexCoord);

    if (texColor.a < 0.5)
        discard;

    vec3 baseColor = texColor.rgb * fs_in.BakedLight.rgb;

    vec3 N = normalize(fs_in.WorldNormal);
    vec3 L = normalize(-uLightDir);
    float diff = max(dot(N, L), 0.0);

    vec3 result = baseColor * (uAmbient + uLightColor * diff);
    FragColor = vec4(result, texColor.a);
}