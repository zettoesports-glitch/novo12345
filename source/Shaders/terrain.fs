#version 330 core
// =============================================================================
// terrain.fs
// -----------------------------------------------------------------------------
// Fragment shader para terreno.
// FIX: convertido de compatibility para core profile.
// FIX: fórmula de brightness corrigida (era errada no original).
// =============================================================================

in VS_OUT {
    vec2  TexCoord;
    vec3  FragPos;
    vec3  Normal;
    vec4  VertColor;
} fs_in;

uniform sampler2D texture1;
uniform float brightness;
uniform float contrast;

layout (location = 0) out vec4 FragColor;

void main()
{
    vec4 texColor = texture(texture1, fs_in.TexCoord);

    // Mix com cor do vértice (lightmap baked)
    vec3 result = texColor.rgb * fs_in.VertColor.rgb;

    // FIX: brightness/contrast corrigidos
    // Antes (ERRADO): result = (result - 0.5) * contrast + 0.5 + (brightness - 1.0);
    // Agora (CORRETO):
    result = (result - 0.5) * contrast + 0.5;
    result = result * brightness;

    FragColor = vec4(result, texColor.a * fs_in.VertColor.a);
}