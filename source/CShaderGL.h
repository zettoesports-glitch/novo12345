#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <string>
#include <unordered_map>

/**
 * @file CShaderGL.h
 * @brief Gerenciador de shaders com suporte a coloriza��o din�mica
 * @details Integrado com CColorSystem para renderiza��o otimizada
 */

class CShaderGL
{
public:
    // Tipos de shader suportados
    enum ShaderType
    {
        SHADER_DEFAULT = 0,
        SHADER_TERRAIN = 1,
        SHADER_GLOW = 2,
        SHADER_CHARACTER = 3,
        SHADER_COLORIZED = 4
    };

    CShaderGL();
    virtual ~CShaderGL();

    // Inicializa��o
    void Init();
    void Shutdown();
    void ClearUniformCache();

    // Gerenciamento de programa de shader
    void RenderShader(ShaderType type = SHADER_DEFAULT);
    bool CheckedShader(ShaderType type = SHADER_DEFAULT) const;
    
    // Obter IDs dos programas
    GLuint GetShaderId() const;
    GLuint GetShaderTerrainId() const;
    GLuint GetShaderGlowId() const;
    GLuint GetShaderCharacterId() const;
    GLuint GetShaderColorizedId() const;
    GLuint GetShaderSkinningId() const;  // GPU Skinning

    // Carregamento de shaders
    GLuint LoadShaderProgram(const char* vertexShaderFile, const char* fragmentShaderFile);

    // Leitura de arquivos
    bool readshader(const char* filename, std::string& shader_text);

    // Compila��o de shaders
    GLuint run_shader(const char* shader_text, GLenum type);

    // Uniforms - tipos b�sicos
    void setBool(const char* name, bool value) const;
    void setInt(const char* name, int value) const;
    void setFloat(const char* name, float value) const;

    // Uniforms - vetores
    void setVec2(const char* name, float x, float y) const;
    void setVec3(const char* name, float x, float y, float z) const;
    void setVec4(const char* name, float x, float y, float z, float w) const;

    // Uniforms - matrizes
    void setMat4(const char* name, glm::mat4& matrix) const;

    // Matriz de perspectiva
    void SetPerspective(float fov, float aspect, float nearPlane, float farPlane);
    void run_projection();

    // Singleton
    static CShaderGL* Instance();

private:
    GLuint shader_id;
    GLuint shader_terrain_id;
    GLuint shader_glow_id;
    GLuint shader_character_id;
    GLuint shader_colorized_id;
    GLuint shader_skinning_id;  // GPU Skinning

    glm::mat4 m_ProjectionMatrix;

    bool m_bInitialized;

    std::unordered_map<GLuint, std::unordered_map<std::string, GLint>> m_UniformCache;
    GLint GetUniformLocationCached(GLuint program, const char* name) const;
};

#define gShaderGL (CShaderGL::Instance())
