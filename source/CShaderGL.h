#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <string>

/**
 * @file CShaderGL.h
 * @brief Gerenciador de shaders com suporte a colorizacao dinamica
 */

class CShaderGL
{
public:
    enum ShaderType
    {
        SHADER_DEFAULT = 0,
        SHADER_TERRAIN = 1,
        SHADER_GLOW = 2,
        SHADER_CHARACTER = 3,
        SHADER_COLORIZED = 4,
        SHADER_FORWARD_TRANSPARENT = 5
    };

    CShaderGL();
    virtual ~CShaderGL();

    void Init();
    void Shutdown();

    void RenderShader(ShaderType type = SHADER_DEFAULT);
    bool CheckedShader(ShaderType type = SHADER_DEFAULT) const;

    GLuint GetShaderId() const;
    GLuint GetShaderTerrainId() const;
    GLuint GetShaderGlowId() const;
    GLuint GetShaderCharacterId() const;
    GLuint GetShaderColorizedId() const;
    GLuint GetShaderForwardTransparentId() const;

    GLuint LoadShaderProgram(const char* vertexShaderFile, const char* fragmentShaderFile);
    bool readshader(const char* filename, std::string& shader_text);
    GLuint run_shader(const char* shader_text, GLenum type);

    // FIX PASSO 3: setar defaults dos novos uniforms (alpha test + fog)
    void SetDefaultUniforms(GLuint program);

    void setBool(const char* name, bool value) const;
    void setInt(const char* name, int value) const;
    void setFloat(const char* name, float value) const;
    void setVec2(const char* name, float x, float y) const;
    void setVec3(const char* name, float x, float y, float z) const;
    void setVec4(const char* name, float x, float y, float z, float w) const;
    void setMat4(const char* name, const glm::mat4& matrix) const;

    void SetPerspective(float fov, float aspect, float nearPlane, float farPlane);
    void run_projection();

    static CShaderGL* Instance();

private:
    GLuint shader_id;
    GLuint shader_terrain_id;
    GLuint shader_glow_id;
    GLuint shader_character_id;
    GLuint shader_colorized_id;
    GLuint shader_forward_transparent_id;

    glm::mat4 m_ProjectionMatrix;
    bool m_bInitialized;
};

#define gShaderGL (CShaderGL::Instance())