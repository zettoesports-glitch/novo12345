#version 330 core
// =============================================================================
// forward_transparent.fs
// -----------------------------------------------------------------------------
// Forward shading para transparência real (alpha blend).
// Roda em pass separado DEPOIS do deferred lighting.
// Usa a mesma interface VS_OUT do shader.vs (gbuffer.vs).
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

// Parâmetros de glow (antes hardcoded em glow.fs)
uniform bool  enableGlow;
uniform float glowIntensity;
uniform vec3  glowColor;
uniform float glowPulseSpeed;
uniform float time;

layout (location = 0) out vec4 FragColor;

void main()
{
    vec4 texColor = texture(texture1, fs_in.TexCoord);
    float alpha = texColor.a * fs_in.BakedLight.a;

    // Transparência real — alpha blend, NÃO discard (exceto totalmente invisível)
    if (alpha < 0.01)
        discard;

    Material mat;
    mat.albedo           = texColor.rgb * fs_in.BakedLight.rgb;
    mat.ambientStrength  = ambientStrength;
    mat.specularStrength = specularStrength;
    mat.shininess        = shininess;

    vec3 result = ComputeBlinnPhong(
        fs_in.WorldPos, fs_in.WorldNormal, viewPos,
        lightPos, lightColor, mat);

    // Glow opcional (wings, partículas, efeitos)
    if (enableGlow)
    {
        float pulse = sin(time * glowPulseSpeed) * 0.5 + 0.5;
        result += glowColor * glowIntensity * pulse;
    }

    FragColor = vec4(result, alpha);
}