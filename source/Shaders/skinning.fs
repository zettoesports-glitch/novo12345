#version 330 core
// =============================================================================
// skinning.fs
// -----------------------------------------------------------------------------
// Fragment shader para skinning (personagens com esqueleto).
// FIX: removidos inicializadores de uniform (ilegais em GLSL 330 puro).
// FIX: interface padronizada VS_OUT.
// Usa common_lighting.glsl para iluminação Blinn-Phong.
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

layout (location = 0) out vec4 FragColor;

void main()
{
    vec4 texColor = texture(texture1, fs_in.TexCoord);

    Material mat;
    mat.albedo           = texColor.rgb * fs_in.BakedLight.rgb;
    mat.ambientStrength  = ambientStrength;
    mat.specularStrength = specularStrength;
    mat.shininess        = shininess;

    vec3 result = ComputeBlinnPhong(
        fs_in.WorldPos, fs_in.WorldNormal, viewPos,
        lightPos, lightColor, mat);

    FragColor = vec4(result, texColor.a * fs_in.BakedLight.a);
}