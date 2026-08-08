#version 330 core
// =============================================================================
// skinning.vs
// -----------------------------------------------------------------------------
// Vertex shader para skinning (personagens com esqueleto).
// FIX: removido gl_Color (compatibility), usando aColor (core).
// FIX: layout padronizado para bater com ZzzBMD.cpp::CreateVertexBuffer.
// =============================================================================

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;
layout (location = 3) in vec3 aNormal;
layout (location = 4) in ivec4 aBoneIDs;
layout (location = 5) in vec4 aWeights;

const int MAX_BONES = 100;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;
uniform mat4 gBones[MAX_BONES];

out VS_OUT {
    vec2  TexCoord;
    vec3  WorldPos;
    vec3  WorldNormal;
    vec4  BakedLight;
} vs_out;

void main()
{
    mat4 BoneTransform = gBones[aBoneIDs[0]] * aWeights[0];
    BoneTransform += gBones[aBoneIDs[1]] * aWeights[1];
    BoneTransform += gBones[aBoneIDs[2]] * aWeights[2];
    BoneTransform += gBones[aBoneIDs[3]] * aWeights[3];

    vec4 worldPos = model * BoneTransform * vec4(aPos, 1.0);

    vs_out.WorldPos    = worldPos.xyz;
    vs_out.WorldNormal = normalize(mat3(model * BoneTransform) * aNormal);
    vs_out.TexCoord    = aTexCoord;
    vs_out.BakedLight  = aColor;

    gl_Position = projection * view * worldPos;
}