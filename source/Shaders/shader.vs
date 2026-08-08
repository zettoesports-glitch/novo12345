#version 330 core
// =============================================================================
// shader.vs
// -----------------------------------------------------------------------------
// Vertex shader principal para objetos/personagens.
// FIX: convertido de compatibility para core profile.
// Layout compatível com ZzzBMD.cpp::CreateVertexBuffer:
//   location 0 = posição (vec3)
//   location 1 = texCoord (vec2)
//   location 2 = cor/luz baked (vec4)
//   location 3 = normal (vec3)
// =============================================================================

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;
layout (location = 3) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

out VS_OUT {
    vec2  TexCoord;
    vec3  WorldPos;
    vec3  WorldNormal;
    vec4  BakedLight;
} vs_out;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);

    vs_out.WorldPos    = worldPos.xyz;
    vs_out.WorldNormal = normalize(normalMatrix * aNormal);
    vs_out.TexCoord    = aTexCoord;
    vs_out.BakedLight  = aColor;

    gl_Position = projection * view * worldPos;
}