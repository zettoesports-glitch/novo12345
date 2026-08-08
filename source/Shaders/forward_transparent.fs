#version 330 core
// =============================================================================
// forward_transparent.fs
// FIX PASSO 3: adicionado alpha test
// =============================================================================

in VS_OUT {
    vec2  TexCoord;
    vec3  WorldPos;
    vec3  WorldNormal;
    vec4  BakedLight;
} fs_in;

uniform sampler2D texture1;
uniform vec3  lightPos;
uniform vec3  lightColor;
uniform vec3  viewPos;
uniform float ambientStrength;
uniform float specularStrength;
uniform float shininess;

uniform float uAlphaThreshold;
uniform bool  uAlphaTest;
uniform bool  uFogEnable;
uniform vec3  uFogColor;
uniform float uFogStart;
uniform float uFogEnd;

layout (location = 0) out vec4 FragColor;

void main()
{
    vec4 texColor = texture(texture1, fs_in.TexCoord);

    if (uAlphaTest && texColor.a * fs_in.BakedLight.a < uAlphaThreshold)
        discard;

    Material mat;
    mat.albedo           = texColor.rgb * fs_in.BakedLight.rgb;
    mat.ambientStrength  = ambientStrength;
    mat.specularStrength = specularStrength;
    mat.shininess        = shininess;

    vec3 result = ComputeBlinnPhong(
        fs_in.WorldPos, fs_in.WorldNormal, viewPos,
        lightPos, lightColor, mat);

    if (uFogEnable)
    {
        float dist = length(viewPos - fs_in.WorldPos);
        float fogFactor = clamp((uFogEnd - dist) / (uFogEnd - uFogStart), 0.0, 1.0);
        result = mix(uFogColor, result, fogFactor);
    }

    FragColor = vec4(result, texColor.a * fs_in.BakedLight.a);
}