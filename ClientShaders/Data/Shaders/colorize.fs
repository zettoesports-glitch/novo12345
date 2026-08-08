#version 330 compatibility

out vec4 FragColor;

in vec2 TexCoord;
in vec3 vColor;
in vec3 vNormal;
in vec3 vPos;

// Texture (engine may bind either name to unit 0)
uniform sampler2D uTexture;
uniform sampler2D texture1;

// Simple lighting (safe defaults)
uniform vec3 uLightDir   = vec3(0.0, 0.0, 1.0);
uniform vec3 uLightColor = vec3(1.0, 1.0, 1.0);
uniform vec3 uAmbient    = vec3(1.0, 1.0, 1.0);

void main()
{
    // Prefer uTexture; if engine only sets texture1, both default to unit 0 anyway.
    vec4 texColor = texture(uTexture, TexCoord);

    float alpha = texColor.a;
    if (alpha < 0.1)
        discard;

    // Apply tint
    vec3 base = texColor.rgb * vColor;

    // Lambert diffuse
    vec3 n = normalize(vNormal);
    vec3 l = normalize(uLightDir);
    float diff = max(dot(n, l), 0.0);

    vec3 lit = base * (uAmbient + uLightColor * diff);

    FragColor = vec4(lit, alpha);
}
