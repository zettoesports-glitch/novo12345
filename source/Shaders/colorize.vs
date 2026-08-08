// MU Season 6 - Colorization Shader (Vertex)
// Portado de GLSL 120 para 330 core

#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

uniform int uColorMode;
uniform int uColorValue;
uniform sampler1D uColorPalette;

out vec3 fragColor;
out vec3 fragNormal;
out vec2 fragTexCoord;
out vec3 fragPos;

void main()
{
    if (uColorMode > 0)
    {
        float colorIndex = float(uColorValue) / 255.0;
        vec3 paletteColor = texture(uColorPalette, colorIndex).rgb;
        fragColor = aColor * paletteColor;
    }
    else
    {
        fragColor = aColor;
    }

    fragNormal = normalize(mat3(uModel) * vec3(0.0, 0.0, 1.0));
    fragTexCoord = aTexCoord;
    fragPos = vec3(uModel * vec4(aPosition, 1.0));

    gl_Position = uProj * uView * uModel * vec4(aPosition, 1.0);
}
