#version 330 compatibility
out vec4 FragColor;

in vec2 TexCoord;
in vec4 to_light;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture1;
uniform vec3 lightPos = vec3(0.0, 1000.0, 1000.0);
uniform vec3 viewPos;
uniform float ambientStrength = 0.5;
uniform float specularStrength = 0.3;

// uMode: 0/1 = iluminado (ambient+diffuse+specular, padrao)
//        2   = RENDER_DARK  -> so textura, sem luz (sombra/silhueta)
//        3   = RENDER_BRIGHT/RENDER_COLOR -> cor solida (hit flash, highlight)
uniform int uMode = 0;
uniform float Alpha = 1.0;

void main() {
    // Modo 3: cor solida, ignora textura e iluminacao
    if (uMode == 3) {
        FragColor = vec4(to_light.rgb, Alpha);
        return;
    }

    vec4 textureColor = texture(texture1, TexCoord);
    float alpha = textureColor.a * to_light.a;
    if (alpha < 0.1) discard;

    // Modo 2: so textura, sem iluminacao
    if (uMode == 2) {
        FragColor = vec4(textureColor.rgb, alpha);
        return;
    }

    // Modo padrao: Phong completo
    // Ambient
    vec3 ambient = ambientStrength * to_light.rgb;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * to_light.rgb;
    
    // Specular
    vec3 viewDir = normalize(viewPos - FragPos); 
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * vec3(1.0);
    
    vec3 result = (ambient + diffuse + specular) * textureColor.rgb;
    
    FragColor = vec4(result, alpha);
}