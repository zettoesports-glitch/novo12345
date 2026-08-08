// MU Season 6 - Colorization Shader (Fragment)
// Portado de GLSL 120 para 330 core

#version 330 core

in vec3 fragColor;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec3 fragPos;

uniform sampler2D uTexture;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;

out vec4 FragColor;

void main()
{
    vec4 texColor = texture(uTexture, fragTexCoord);

    if (texColor.a < 0.5)
        discard;

    vec3 baseColor = texColor.rgb * fragColor;

    vec3 norm = normalize(fragNormal);
    float diff = max(dot(norm, uLightDir), 0.0);

    vec3 result = baseColor * (uAmbient + uLightColor * diff);

    FragColor = vec4(result, texColor.a);
}
