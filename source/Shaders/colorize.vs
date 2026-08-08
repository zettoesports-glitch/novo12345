#version 330 core
// =============================================================================
// colorize.vs
// -----------------------------------------------------------------------------
// Vertex shader para colorização dinâmica (item grade, classe, time).
// FIX: layout VAO corrigido para bater com ZzzBMD.cpp::CreateVertexBuffer:
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

uniform int uColorMode;       // 0=None, 1=ItemGrade, 2=Class, 3=Custom, 4=Elemental, 5=Team
uniform int uColorValue;      // Índice para lookup na paleta
uniform sampler1D uColorPalette;

out VS_OUT {
    vec2  TexCoord;
    vec4  BakedLight;
    vec3  WorldNormal;
    vec3  WorldPos;
} vs_out;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);

    vs_out.WorldPos    = worldPos.xyz;
    vs_out.WorldNormal = normalize(normalMatrix * aNormal);
    vs_out.TexCoord    = aTexCoord;

    // Colorização via paleta 1D
    if (uColorMode > 0)
    {
        float idx = float(uColorValue) / 255.0;
        vec3 palette = texture(uColorPalette, idx).rgb;
        vs_out.BakedLight = vec4(aColor.rgb * palette, aColor.a);
    }
    else
    {
        vs_out.BakedLight = aColor;
    }

    gl_Position = projection * view * worldPos;
}