#version 330 compatibility

out vec2 TexCoord;
out vec4 to_light;
out vec3 FragPos;
out vec3 Normal;

void main()
{
    FragPos = vec3(gl_ModelViewMatrix * gl_Vertex);
    Normal = gl_NormalMatrix * gl_Normal;
    
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    TexCoord = gl_MultiTexCoord0.xy;
    to_light = gl_Color;
}
