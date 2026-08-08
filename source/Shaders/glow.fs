#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 to_light;
in vec3 FragPos;

uniform sampler2D texture1;
uniform float time;           // Tempo do jogo para animação
uniform float glowIntensity;  // Intensidade do brilho (0.0 a 1.0)
uniform vec3 glowColor;       // Cor do brilho (RGB)

// uMode: 0/1 = glow normal (padrao, com pulsacao)
//        2   = RENDER_DARK  -> so textura, sem luz/glow (sombra/silhueta)
//        3   = RENDER_BRIGHT/RENDER_COLOR -> cor solida (hit flash, highlight)
uniform int uMode = 0;
uniform float Alpha = 1.0;

void main() {
    // Modo 3: cor solida, ignora textura, luz e glow
    if (uMode == 3) {
        FragColor = vec4(to_light.rgb, Alpha);
        return;
    }

    vec4 textureColor = texture(texture1, TexCoord);

    // Modo 2: so textura, sem luz/glow
    if (uMode == 2) {
        FragColor = vec4(textureColor.rgb, textureColor.a * to_light.a);
        return;
    }

    // Modo padrao: glow com pulsacao
    vec4 lightColor = to_light;

    float maxIntensity = 1.5; // Permite brilho extra
    lightColor.rgb = min(lightColor.rgb, vec3(maxIntensity));

    FragColor = textureColor * lightColor;
    
    // Efeito de pulsação
    float pulse = sin(time * 3.0) * 0.5 + 0.5; // Oscila entre 0.0 e 1.0
    
    // Adicionar brilho pulsante
    vec3 glow = glowColor * glowIntensity * pulse;
    FragColor.rgb += glow;
    
    // Aumentar brilho nas áreas já claras (bloom effect)
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 0.8) {
        FragColor.rgb *= 1.2;
    }
}