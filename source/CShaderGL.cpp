#include "StdAfx.h"
#include "CShaderGL.h"

#ifdef SHADER_VERSION_TEST
#include "Utilities/Log/muConsoleDebug.h"
#endif

CShaderGL::CShaderGL()
    : shader_id(0),
      shader_terrain_id(0),
      shader_glow_id(0),
      shader_character_id(0),
      shader_colorized_id(0),
      shader_forward_transparent_id(0),
      m_ProjectionMatrix(glm::mat4(1.0f)),
      m_bInitialized(false)
{
}

CShaderGL::~CShaderGL()
{
    Shutdown();
}

CShaderGL* CShaderGL::Instance()
{
    static CShaderGL sInstance;
    return &sInstance;
}

void CShaderGL::Init()
{
    if (m_bInitialized)
        return;

    // FIX: eliminado #ifdef SHADER_VERSION_TEST. Agora SEMPRE carrega shaders.
    // O caminho legado foi removido do ZzzBMD.cpp.
    this->shader_id =
        this->LoadShaderProgram("Shaders/shader.vs", "Shaders/shader.fs");
    this->shader_terrain_id =
        this->LoadShaderProgram("Shaders/terrain.vs", "Shaders/terrain.fs");
    this->shader_glow_id =
        this->LoadShaderProgram("Shaders/glow.vs", "Shaders/glow.fs");
    this->shader_character_id =
        this->LoadShaderProgram("Shaders/character.vs", "Shaders/character.fs");
    this->shader_colorized_id =
        this->LoadShaderProgram("Shaders/colorize.vs", "Shaders/colorize.fs");
    this->shader_forward_transparent_id =
        this->LoadShaderProgram("Shaders/shader.vs", "Shaders/forward_transparent.fs");

    m_bInitialized = (shader_id != 0);

    if (!m_bInitialized)
    {
#ifdef SHADER_VERSION_TEST
        g_ConsoleDebug->Write(MCD_ERROR, "[CShaderGL] Falha ao inicializar shaders");
#endif
    }
}

void CShaderGL::Shutdown()
{
    if (shader_id != 0) glDeleteProgram(shader_id);
    if (shader_terrain_id != 0) glDeleteProgram(shader_terrain_id);
    if (shader_glow_id != 0) glDeleteProgram(shader_glow_id);
    if (shader_character_id != 0) glDeleteProgram(shader_character_id);
    if (shader_colorized_id != 0) glDeleteProgram(shader_colorized_id);
    if (shader_forward_transparent_id != 0) glDeleteProgram(shader_forward_transparent_id);

    shader_id = 0;
    shader_terrain_id = 0;
    shader_glow_id = 0;
    shader_character_id = 0;
    shader_colorized_id = 0;
    shader_forward_transparent_id = 0;
    m_bInitialized = false;
}

void CShaderGL::RenderShader(ShaderType type)
{
    GLuint shader = 0;
    switch (type)
    {
        case SHADER_TERRAIN:             shader = shader_terrain_id; break;
        case SHADER_GLOW:                shader = shader_glow_id; break;
        case SHADER_CHARACTER:           shader = shader_character_id; break;
        case SHADER_COLORIZED:           shader = shader_colorized_id; break;
        case SHADER_FORWARD_TRANSPARENT: shader = shader_forward_transparent_id; break;
        default:                         shader = shader_id; break;
    }
    if (shader != 0)
        glUseProgram(shader);
}

bool CShaderGL::CheckedShader(ShaderType type) const
{
    switch (type)
    {
        case SHADER_TERRAIN:             return shader_terrain_id != 0;
        case SHADER_GLOW:                return shader_glow_id != 0;
        case SHADER_CHARACTER:           return shader_character_id != 0;
        case SHADER_COLORIZED:           return shader_colorized_id != 0;
        case SHADER_FORWARD_TRANSPARENT: return shader_forward_transparent_id != 0;
        default:                         return shader_id != 0;
    }
}

GLuint CShaderGL::GetShaderId() const { return shader_id; }
GLuint CShaderGL::GetShaderTerrainId() const { return shader_terrain_id; }
GLuint CShaderGL::GetShaderGlowId() const { return shader_glow_id; }
GLuint CShaderGL::GetShaderCharacterId() const { return shader_character_id; }
GLuint CShaderGL::GetShaderColorizedId() const { return shader_colorized_id; }
GLuint CShaderGL::GetShaderForwardTransparentId() const { return shader_forward_transparent_id; }

bool CShaderGL::readshader(const char* filename, std::string& shader_text)
{
    FILE* file = fopen(filename, "rb");
    if (!file)
        return false;

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    shader_text.resize(size);
    fread(&shader_text[0], 1, size, file);
    fclose(file);
    return true;
}

GLuint CShaderGL::run_shader(const char* shader_text, GLenum type)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shader_text, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
#ifdef SHADER_VERSION_TEST
        g_ConsoleDebug->Write(MCD_ERROR, "[CShaderGL] Shader compile error: %s", infoLog);
#endif
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// =============================================================================
// FIX CRÍTICO: LoadShaderProgram agora injeta common_lighting.glsl
// automaticamente no fragment shader, logo após a linha #version.
// Isso elimina duplicação de código de iluminação entre shaders.
// =============================================================================
GLuint CShaderGL::LoadShaderProgram(const char* vertexShaderFile, const char* fragmentShaderFile)
{
    std::string vsCode, fsCode;
    if (!readshader(vertexShaderFile, vsCode))
    {
#ifdef SHADER_VERSION_TEST
        g_ConsoleDebug->Write(MCD_ERROR, "[CShaderGL] Failed to load vertex shader: %s", vertexShaderFile);
#endif
        return 0;
    }
    if (!readshader(fragmentShaderFile, fsCode))
    {
#ifdef SHADER_VERSION_TEST
        g_ConsoleDebug->Write(MCD_ERROR, "[CShaderGL] Failed to load fragment shader: %s", fragmentShaderFile);
#endif
        return 0;
    }

    // FIX: injetar common_lighting.glsl no fragment shader
    std::string commonLighting;
    if (readshader("Shaders/common_lighting.glsl", commonLighting))
    {
        size_t versionEnd = fsCode.find('\n');
        if (versionEnd != std::string::npos)
        {
            std::string versionLine = fsCode.substr(0, versionEnd + 1);
            std::string rest = fsCode.substr(versionEnd + 1);
            fsCode = versionLine + commonLighting + rest;
        }
    }

    GLuint vs = run_shader(vsCode.c_str(), GL_VERTEX_SHADER);
    GLuint fs = run_shader(fsCode.c_str(), GL_FRAGMENT_SHADER);

    if (vs == 0 || fs == 0)
        return 0;

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
#ifdef SHADER_VERSION_TEST
        g_ConsoleDebug->Write(MCD_ERROR, "[CShaderGL] Program link error: %s", infoLog);
#endif
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

void CShaderGL::setBool(const char* name, bool value) const
{
    GLint loc = glGetUniformLocation(shader_id, name);
    if (loc >= 0) glUniform1i(loc, (int)value);
}
void CShaderGL::setInt(const char* name, int value) const
{
    GLint loc = glGetUniformLocation(shader_id, name);
    if (loc >= 0) glUniform1i(loc, value);
}
void CShaderGL::setFloat(const char* name, float value) const
{
    GLint loc = glGetUniformLocation(shader_id, name);
    if (loc >= 0) glUniform1f(loc, value);
}
void CShaderGL::setVec2(const char* name, float x, float y) const
{
    GLint loc = glGetUniformLocation(shader_id, name);
    if (loc >= 0) glUniform2f(loc, x, y);
}
void CShaderGL::setVec3(const char* name, float x, float y, float z) const
{
    GLint loc = glGetUniformLocation(shader_id, name);
    if (loc >= 0) glUniform3f(loc, x, y, z);
}
void CShaderGL::setVec4(const char* name, float x, float y, float z, float w) const
{
    GLint loc = glGetUniformLocation(shader_id, name);
    if (loc >= 0) glUniform4f(loc, x, y, z, w);
}
void CShaderGL::setMat4(const char* name, const glm::mat4& matrix) const  // FIX: const&
{
    GLint loc = glGetUniformLocation(shader_id, name);
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));
}

void CShaderGL::SetPerspective(float fov, float aspect, float nearPlane, float farPlane)
{
    m_ProjectionMatrix = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

void CShaderGL::run_projection()
{
    // placeholder para compatibilidade
}