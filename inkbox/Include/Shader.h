#pragma once

#include <string>
#include <map>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#define MEASURE_CS_TIMES 0

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
    GLShader(const char* path, ShaderType type, glm::uvec3 compute_local_size = glm::uvec3());
    bool Compile();
    void Discard();
    int Id() const { return id; }
    std::string FileName() const { return fileName; }
    glm::vec3 ComputeLocalSize() const { return computeShaderLocalSize; }

private:

    std::string ProcessSourceCode(std::string source);

    std::string sourceFile;
    std::string fileName;
    std::string sourceCode; // processed
    int id;
    ShaderType type;
    glm::uvec3 computeShaderLocalSize;

    bool HasCompileErrors(int id);
};


class GLShaderProgram
{
public:
    GLShaderProgram();
    ~GLShaderProgram();
    virtual void Init();
    bool Link();
    virtual void Attach(const GLShader& shader);
    void Detach(const GLShader& shader);
    void Use();
    int Id() const { return id; }

    void SetInt(std::string name, int value);
    void SetFloat(std::string name, float value);
    void SetVec2(std::string name, const glm::vec2& value);
    void SetVec3(std::string name, const glm::vec3& value);
    void SetVec2(std::string name, float x, float y);
    void SetVec4(std::string name, const glm::vec4& value);
    void SetMatrix4x4(std::string name, const glm::mat4& value);

    void SetTexture(std::string name, class Texture& fbo, int value);
    void SetTexture(std::string name, class IFBO& fbo, int value);
    void SetImage(std::string name, class Texture& texture, int value, int access);

    bool HasLinkErrors();
    bool Validate();

    std::string Name;

    static void UseNone();

    static void LoadIncludeFile(const char* file);

private:
    int id;
    std::map<std::string, int> uniformLocLookup;
    int GetUniformLoc(std::string name);
};

#if MEASURE_CS_TIMES
struct ShaderTimingInfo
{
    ShaderTimingInfo()
        : TotalTime(0)
        , NumExecutions(0)
    {
    }

    double AverageTime() const { return TotalTime / NumExecutions; }

    double TotalTime;
    long NumExecutions;
};
#endif

class GLComputeShader : public GLShaderProgram
{
public:
    void Init() override;
    void Execute(int x, int y, int z);
    void Execute(glm::uvec3 num_work_groups);

#if MEASURE_CS_TIMES
    ShaderTimingInfo const TimingInfo() { return timing; }
private:

    unsigned int timeQuery0;
    unsigned int timeQuery1;
    ShaderTimingInfo timing;
#endif
};