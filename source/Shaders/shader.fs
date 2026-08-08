#version 330 core
// =============================================================================
// shader.fs
// FIX PASSO 3: adicionado alpha test (discard) e fog no fragment shader
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

// FIX PASSO 3: uniforms para alpha test e fog
uniform float uAlphaThreshold;  // default: 0.25
uniform bool  uAlphaTest;       // default: true
uniform bool  uFogEnable;       // default: false
uniform vec3  uFogColor;        // default: vec3(0.078, 0.078, 0.078)
uniform float uFogDensity;      // default: 0.0004
uniform float uFogStart;        // default: 2000.0
uniform float uFogEnd;          // default: 2700.0

layout (location = 0) out vec4 FragColor;

// Incluir iluminacao (common_lighting.glsl ja eh injetado pelo CShaderGL::LoadShaderProgram)
// Material struct e ComputeBlinnPhong vêm de common_lighting.glsl

void main()
{
    vec4 texColor = texture(texture1, fs_in.TexCoord);

    // FIX PASSO 3: Alpha Test (substitui glAlphaFunc(GL_GREATER, 0.25f))
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

    // FIX PASSO 3: Fog linear (substitui glEnable(GL_FOG) + glFogf)
    if (uFogEnable)
    {
        float dist = length(viewPos - fs_in.WorldPos);
        float fogFactor = clamp((uFogEnd - dist) / (uFogEnd - uFogStart), 0.0, 1.0);
        result = mix(uFogColor, result, fogFactor);
    }

    FragColor = vec4(result, texColor.a * fs_in.BakedLight.a);
}