#include "stdafx.h"
#include "CShaderGL.h"

#ifdef SHADER_VERSION_TEST
#include "Utilities/Log/muConsoleDebug.h"
#include <fstream>
#include <sstream>

CShaderGL::CShaderGL()
    : shader_id(0),
      shader_terrain_id(0),
      shader_glow_id(0),
      shader_character_id(0),
      shader_colorized_id(0),
      shader_skinning_id(0),
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
    this->shader_skinning_id =
        this->LoadShaderProgram("Shaders/skinning.vs", "Shaders/skinning.fs");


    m_bInitialized = (shader_id != 0);

    if (!m_bInitialized)
    {
        g_ConsoleDebug->Write(MCD_ERROR, "[CShaderGL] Falha ao inicializar shaders");
    }
}

void CShaderGL::Shutdown()
{
    if (shader_id != 0) glDeleteProgram(shader_id);
    if (shader_terrain_id != 0) glDeleteProgram(shader_terrain_id);
    if (shader_glow_id != 0) glDeleteProgram(shader_glow_id);
    if (shader_character_id != 0) glDeleteProgram(shader_character_id);
    if (shader_colorized_id != 0) glDeleteProgram(shader_colorized_id);
    if (shader_skinning_id != 0) glDeleteProgram(shader_skinning_id);

    shader_id = 0;
    shader_terrain_id = 0;
    shader_glow_id = 0;
    shader_character_id = 0;
    shader_colorized_id = 0;
    shader_skinning_id = 0;
    m_bInitialized = false;

    ClearUniformCache();
}

void CShaderGL::RenderShader(ShaderType type)
{
    GLuint shader = 0;

    switch (type)
    {
        case SHADER_TERRAIN:
            shader = shader_terrain_id;
            break;
        case SHADER_GLOW:
            shader = shader_glow_id;
            break;
        case SHADER_CHARACTER:
            shader = shader_character_id;
            break;
        case SHADER_COLORIZED:
            shader = shader_colorized_id;
            break;
        default:
            shader = shader_id;
            break;
    }

    if (shader != 0)
    {
        glUseProgram(shader);
    }
}

bool CShaderGL::CheckedShader(ShaderType type) const
{
    switch (type)
    {
        case SHADER_TERRAIN:
            return shader_terrain_id != 0;
        case SHADER_GLOW:
            return shader_glow_id != 0;
        case SHADER_CHARACTER:
            return shader_character_id != 0;
        case SHADER_COLORIZED:
            return shader_colorized_id != 0;
        default:
            return shader_id != 0;
    }
}

GLuint CShaderGL::GetShaderId() const
{
    return shader_id;
}

GLuint CShaderGL::GetShaderTerrainId() const
{
    return shader_terrain_id;
}

GLuint CShaderGL::GetShaderGlowId() const
{
    return shader_glow_id;
}

GLuint CShaderGL::GetShaderCharacterId() const
{
    return shader_character_id;
}

GLuint CShaderGL::GetShaderColorizedId() const
{
    return shader_colorized_id;
}

GLuint CShaderGL::GetShaderSkinningId() const
{
    return shader_skinning_id;
}

GLuint CShaderGL::LoadShaderProgram(const char* vertexShaderFile,
                                    const char* fragmentShaderFile)
{
    std::string vertexShaderSource, fragmentShaderSource;

    if (!this->readshader(vertexShaderFile, vertexShaderSource) ||
        !this->readshader(fragmentShaderFile, fragmentShaderSource))
        return 0;

    GLuint vertexShader =
        this->run_shader(vertexShaderSource.c_str(), GL_VERTEX_SHADER);
    GLuint fragmentShader =
        this->run_shader(fragmentShaderSource.c_str(), GL_FRAGMENT_SHADER);

    if (vertexShader == 0 || fragmentShader == 0)
        return 0;

    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertexShader);
    glAttachShader(programId, fragmentShader);
    glLinkProgram(programId);

    GLint success;
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(programId, 512, NULL, infoLog);
        g_ConsoleDebug->Write(MCD_ERROR, "Shader Program Link Error (%s/%s): %s",
                              vertexShaderFile, fragmentShaderFile, infoLog);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return programId;
}

bool CShaderGL::readshader(const char* filename, std::string& shader_text)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        // Try adding Data/ prefix if missing, common in MuOnline
        std::string altPath = "Data/";
        altPath += filename;
        file.open(altPath);
        if (!file.is_open())
        {
            g_ConsoleDebug->Write(MCD_ERROR, "Failed to open shader file: %s",
                                  filename);
            return false;
        }
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    shader_text = buffer.str();
    file.close();
    return true;
}

GLuint CShaderGL::run_shader(const char* shader_text, GLenum type)
{
    GLuint shaderId = glCreateShader(type);
    glShaderSource(shaderId, 1, &shader_text, NULL);
    glCompileShader(shaderId);

    GLint success;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shaderId, 512, NULL, infoLog);
        g_ConsoleDebug->Write(MCD_ERROR, "Shader Compilation Error: %s", infoLog);
        return 0;
    }
    return shaderId;
}

static glm::mat4 g_ProjectionMatrix = glm::mat4(1.0f);

void CShaderGL::run_projection()
{
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    if (program > 0)
    {
        GLint loc = glGetUniformLocation(program, "projection");
        if (loc != -1)
        {
            glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(g_ProjectionMatrix));
        }
    }
}

void CShaderGL::SetPerspective(float Fov, float Aspect, float ZNear,
                               float ZFar)
{
    g_ProjectionMatrix = glm::perspective(glm::radians(Fov), Aspect, ZNear, ZFar);
    m_ProjectionMatrix = g_ProjectionMatrix;
    this->run_projection();
}

void CShaderGL::ClearUniformCache()
{
    m_UniformCache.clear();
}

GLint CShaderGL::GetUniformLocationCached(GLuint program, const char* name) const
{
    auto progIt = m_UniformCache.find(program);
    if (progIt == m_UniformCache.end())
    {
        auto& newMap = const_cast<std::unordered_map<GLuint, std::unordered_map<std::string, GLint>>&>(m_UniformCache)[program];
        GLint loc = glGetUniformLocation(program, name);
        newMap[name] = loc;
        return loc;
    }
    auto nameIt = progIt->second.find(name);
    if (nameIt != progIt->second.end())
        return nameIt->second;
    GLint loc = glGetUniformLocation(program, name);
    const_cast<std::unordered_map<std::string, GLint>&>(progIt->second)[name] = loc;
    return loc;
}

void CShaderGL::setBool(const char* name, bool value) const
{
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    if (program > 0)
        glUniform1i(GetUniformLocationCached(program, name), (int)value);
}

void CShaderGL::setInt(const char* name, int value) const
{
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    if (program > 0)
        glUniform1i(GetUniformLocationCached(program, name), value);
}

void CShaderGL::setFloat(const char* name, float value) const
{
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    if (program > 0)
        glUniform1f(GetUniformLocationCached(program, name), value);
}

void CShaderGL::setVec2(const char* name, float x, float y) const
{
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    if (program > 0)
        glUniform2f(GetUniformLocationCached(program, name), x, y);
}

void CShaderGL::setVec3(const char* name, float x, float y, float z) const
{
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    if (program > 0)
        glUniform3f(GetUniformLocationCached(program, name), x, y, z);
}

void CShaderGL::setVec4(const char* name, float x, float y, float z,
                        float w) const
{
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    if (program > 0)
        glUniform4f(GetUniformLocationCached(program, name), x, y, z, w);
}

void CShaderGL::setMat4(const char* name, glm::mat4& matrix) const
{
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    if (program > 0)
        glUniformMatrix4fv(GetUniformLocationCached(program, name), 1, GL_FALSE,
                           glm::value_ptr(matrix));
}

#endif // SHADER_VERSION_TEST
