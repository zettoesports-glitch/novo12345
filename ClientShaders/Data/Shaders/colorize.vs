#version 330 compatibility

// Colorize shader (compatibility) - safe defaults

out vec2 TexCoord;
out vec3 vColor;
out vec3 vNormal;
out vec3 vPos;

uniform int uColorMode = 0;      // 0 = none
uniform int uColorValue = 0;     // 0..255
uniform sampler1D uColorPalette; // optional (bind to unit 0 or any)

void main()
{
    TexCoord = gl_MultiTexCoord0.xy;
    vPos     = vec3(gl_ModelViewMatrix * gl_Vertex);
    vNormal  = normalize(gl_NormalMatrix * gl_Normal);

    vec3 baseColor = gl_Color.rgb;

    if (uColorMode > 0)
    {
        float idx = clamp(float(uColorValue) / 255.0, 0.0, 1.0);
        vec3 pal  = texture1D(uColorPalette, idx).rgb;
        vColor    = baseColor * pal;
    }
    else
    {
        vColor = baseColor;
    }

    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
