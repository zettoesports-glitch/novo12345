#version 330 core
// =============================================================================
// common_lighting.glsl
// -----------------------------------------------------------------------------
// Funções de iluminação compartilhadas por TODOS os fragment shaders.
// GLSL 330 core puro — sem extensões, sem inicializadores em uniforms.
// Quem seta valores default é o C++ (CShaderGL), não o shader.
// =============================================================================

// ---- Estrutura de material ----
struct Material
{
    vec3  albedo;
    float ambientStrength;
    float specularStrength;
    float shininess;
};

// ---- Blinn-Phong clássico ----
// Todos os vetores em WORLD SPACE (consistente em todo o motor).
vec3 ComputeBlinnPhong(
    vec3 worldPos,
    vec3 worldNormal,
    vec3 viewPos,
    vec3 lightPos,
    vec3 lightColor,
    Material mat)
{
    vec3 N = normalize(worldNormal);
    vec3 L = normalize(lightPos - worldPos);
    vec3 V = normalize(viewPos - worldPos);
    vec3 H = normalize(L + V);

    vec3 ambient  = mat.ambientStrength * mat.albedo;
    float diff    = max(dot(N, L), 0.0);
    vec3 diffuse  = diff * mat.albedo * lightColor;
    float spec    = pow(max(dot(N, H), 0.0), mat.shininess);
    vec3 specular = mat.specularStrength * spec * lightColor;

    return ambient + diffuse + specular;
}

// ---- Encode/decode de normal para G-Buffer (RGB8) ----
// Normais [-1,1] -> textura [0,1]
vec3 EncodeNormal(vec3 n)
{
    return n * 0.5 + 0.5;
}

vec3 DecodeNormal(vec3 encoded)
{
    return normalize(encoded * 2.0 - 1.0);
}

// ---- Alpha cutout padrão ----
bool ShouldDiscardByAlpha(float alpha, float threshold)
{
    return alpha < threshold;
}