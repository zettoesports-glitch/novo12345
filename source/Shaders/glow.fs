#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 to_light;
in vec3 FragPos;

uniform sampler2D texture1;
uniform float time;
uniform float glowIntensity;
uniform vec3 glowColor;
uniform int uMode;
uniform float Alpha;

// FIX PASSO 3: alpha test
uniform float uAlphaThreshold;
uniform bool  uAlphaTest;

void main() {
    // Modo 3: cor solida
    if (uMode == 3) {
        FragColor = vec4(to_light.rgb, Alpha);
        return;
    }

    vec4 textureColor = texture(texture1, TexCoord);

    // FIX PASSO 3: Alpha Test
    if (uAlphaTest && textureColor.a * to_light.a < uAlphaThreshold)
        discard;

    // Modo 2: so textura
    if (uMode == 2) {
        FragColor = vec4(textureColor.rgb, textureColor.a * to_light.a);
        return;
    }

    // Modo padrao: glow com pulsacao
    vec4 lightColor = to_light;
    float maxIntensity = 1.5;
    lightColor.rgb = min(lightColor.rgb, vec3(maxIntensity));

    FragColor = textureColor * lightColor;

    float pulse = sin(time * 3.0) * 0.5 + 0.5;
    vec3 glow = glowColor * glowIntensity * pulse;
    FragColor.rgb += glow;

    FragColor.a *= Alpha;
}