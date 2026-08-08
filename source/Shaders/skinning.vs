#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;
layout(location = 3) in float aBoneIndex;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 BoneMatrices[200];

uniform int uTranslate;
uniform float BodyScale;
uniform vec3 BodyOrigin;

out vec2 TexCoord;
out vec4 to_light;

void main()
{
    // Indice do osso (float no VBO por compatibilidade; arredonda com seguranca)
    int index = int(aBoneIndex + 0.5);
    if (index < 0) index = 0;
    if (index > 199) index = 199;

    mat4 boneMatrix = BoneMatrices[index];
    // Equivale a VectorTransform (CPU): bone * pos local
    vec4 skinnedPos = boneMatrix * vec4(aPos, 1.0);

    vec3 worldPos = skinnedPos.xyz;
    // g_bCurrentTranslate / Transform(Translate): BodyScale + BodyOrigin no mundo
    if (uTranslate != 0)
    {
        worldPos = worldPos * BodyScale + BodyOrigin;
    }

    gl_Position = projection * view * model * vec4(worldPos, 1.0);
    TexCoord    = aTexCoord;
    to_light    = aColor;
}