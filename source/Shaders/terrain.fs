#version 330 core
// =============================================================================
// terrain.fs
// FIX PASSO 3: adicionado alpha test e fog
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

// FIX PASSO 3: uniforms para alpha test e fog
uniform float uAlphaThreshold;
uniform bool  uAlphaTest;
uniform bool  uFogEnable;
uniform vec3  uFogColor;
uniform float uFogStart;
uniform float uFogEnd;
uniform vec3  viewPos;

layout (location = 0) out vec4 FragColor;

void main()
{
    vec4 texColor = texture(texture1, fs_in.TexCoord);

    // FIX PASSO 3: Alpha Test
    if (uAlphaTest && texColor.a * fs_in.VertColor.a < uAlphaThreshold)
        discard;

    vec3 result = texColor.rgb * fs_in.VertColor.rgb;
    result = (result - 0.5) * contrast + 0.5;
    result = result * brightness;

    // FIX PASSO 3: Fog
    if (uFogEnable)
    {
        float dist = length(-fs_in.FragPos);  // FragPos ja esta em view space
        float fogFactor = clamp((uFogEnd - dist) / (uFogEnd - uFogStart), 0.0, 1.0);
        result = mix(uFogColor, result, fogFactor);
    }

    FragColor = vec4(result, texColor.a * fs_in.VertColor.a);
}