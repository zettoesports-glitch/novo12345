#version 330 core
// =============================================================================
// colorize.fs
// FIX PASSO 3: adicionado alpha test e fog
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

// Colorizacao
uniform int   uColorMode;      // 0=none, 1=item, 2=class, 3=element, 4=team
uniform vec3  uColorTint;      // cor de tint
uniform float uColorIntensity; // 0.0 a 1.0

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

    // Aplicar colorizacao
    vec3 finalColor = texColor.rgb;
    if (uColorMode > 0)
    {
        finalColor = mix(finalColor, finalColor * uColorTint, uColorIntensity);
    }

    Material mat;
    mat.albedo           = finalColor * fs_in.BakedLight.rgb;
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