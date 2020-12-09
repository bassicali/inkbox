#pragma once

#include <string>
#include <map>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

class FBO;

enum class ShaderType
{
    None,
    Compute,
    Vertex,
    Fragment
};

class GLShader
{
public:
    GLShader(const char* path, ShaderType type);
    bool Compile();
    void Discard();
    int Id() const { return id; }


private:

    std::string ProcessSourceCode(std::string source);

    std::string sourceFile;
    std::string sourceCode; // processed
    int id;
    ShaderType type;

    bool HasCompileErrors(int id);
};


class GLShaderProgram
{
public:
    GLShaderProgram();
    ~GLShaderProgram();
    void Init();
    bool Link();
    void Attach(const GLShader& shader);
    void Detach(const GLShader& shader);
    void Use();
    int Id() const { return id; }

    //template <typename TUniform>
    //void SetUniform(std::string name, TUniform value);

    void SetInt(std::string name, int value);
    void SetFloat(std::string name, float value);
    void SetVec2(std::string name, glm::vec2 value);
    void SetVec3(std::string name, glm::vec3 value);
    void SetVec2(std::string name, float x, float y);
    void SetVec4(std::string name, glm::vec4 value);
    void SetTexture(std::string name, class IFBO& fbo, int value);

    bool HasLinkErrors();
    bool Validate();

    static void UseNone();

    static void LoadIncludeFile(const char* file);

private:
    int id;
    std::map<std::string, int> uniformLocLookup;
    int GetUniformLoc(std::string name);
};
