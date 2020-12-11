
#include "Shader.h"

#include <iostream>
#include <exception>
#include <fstream>
#include <filesystem>
#include <regex>
#include <sstream>

#include <glad/glad.h>

#include "FBO.h"
#include "Common.h"

using namespace std;

regex re_include("^[ \\t]*#include[ \\t]+[<\"]([\\.\\w]+?)[>\"][ \\t]*$", regex_constants::optimize | regex_constants::ECMAScript);


///////////////////////////
///   GLShaderProgram   ///
///////////////////////////

GLShaderProgram::GLShaderProgram()
	: id(0)
{
}

GLShaderProgram::~GLShaderProgram()
{
	if (id != 0)
	{
		_GL_WRAP1(glDeleteProgram, id);
	}
}

void GLShaderProgram::Init()
{
	id = _GL_WRAP0(glCreateProgram);
}

void GLShaderProgram::Attach(const GLShader& shader)
{
	_GL_WRAP2(glAttachShader, id, shader.Id());
}

void GLShaderProgram::Detach(const GLShader& shader)
{
	_GL_WRAP2(glDetachShader, id, shader.Id());
}

void GLShaderProgram::Use()
{
	_GL_WRAP1(glUseProgram, id);
}

void GLShaderProgram::SetInt(std::string name, int value)
{
	int loc;
	if ((loc = GetUniformLoc(name)) != -1)
		_GL_WRAP2(glUniform1i, loc, value);
}

void GLShaderProgram::SetFloat(std::string name, float value)
{
	int loc;
	if ((loc = GetUniformLoc(name)) != -1)
		_GL_WRAP2(glUniform1f, loc, value);
}

void GLShaderProgram::SetVec2(string name, glm::vec2 value)
{
	int loc;
	if ((loc = GetUniformLoc(name)) != -1)
		_GL_WRAP3(glUniform2fv, loc, 1, &value[0]);
}

void GLShaderProgram::SetVec3(std::string name, glm::vec3 value)
{
	int loc;
	if ((loc = GetUniformLoc(name)) != -1)
		_GL_WRAP3(glUniform3fv, loc, 1, &value[0]);
}

void GLShaderProgram::SetVec2(std::string name, float x, float y)
{
	int loc;
	if ((loc = GetUniformLoc(name)) != -1)
		_GL_WRAP3(glUniform2f, loc, x, y);
}

void GLShaderProgram::SetVec4(std::string name, glm::vec4 value)
{
	int loc;
	if ((loc = GetUniformLoc(name)) != -1)
		_GL_WRAP3(glUniform4fv, loc, 1, &value[0]);
}

void GLShaderProgram::SetTexture(std::string name, IFBO& fbo, int value)
{
	SetInt(name, value);
	fbo.BindTexture(value);
}

void GLShaderProgram::UseNone()
{
	_GL_WRAP1(glUseProgram, 0);
}

void GLShaderProgram::LoadIncludeFile(const char* file)
{
	using namespace std::filesystem;

	string source = ReadFile(file);
	string name = path(file).filename().string();
	_GL_WRAP5(glNamedStringARB, GL_SHADER_INCLUDE_ARB, name.length(), name.c_str(), source.length(), source.c_str());
}

bool GLShaderProgram::Link()
{
	_GL_WRAP1(glLinkProgram, id);

	if (HasLinkErrors())
	{
		return false;
	}

	return true;
}

int GLShaderProgram::GetUniformLoc(std::string name)
{
	auto search = uniformLocLookup.find(name);
	if (search != uniformLocLookup.end())
	{
		return search->second;
	}

	int loc = _GL_WRAP2(glGetUniformLocation, id, name.c_str());
	uniformLocLookup.insert({ name, loc });

	return loc;
}


bool GLShaderProgram::HasLinkErrors()
{
	int success;
	char message[1024];

	_GL_WRAP3(glGetProgramiv, id, GL_LINK_STATUS, &success);
	if (!success)
	{
		_GL_WRAP4(glGetShaderInfoLog, id, 1024, nullptr, message);
		cout << "Shader program link error(s):" << message << endl;
		return true;
	}

	return false;
}

bool GLShaderProgram::Validate()
{
	glValidateProgram(id);

	int success;
	char message[1024];

	_GL_WRAP3(glGetProgramiv, id, GL_VALIDATE_STATUS, &success);
	if (!success)
	{
		_GL_WRAP4(glGetShaderInfoLog, id, 1024, nullptr, message);
		LOG_ERROR("Shader program has validation error(s): %s", message);
		return true;
	}
	else
	{
		LOG_INFO("Shader program is valid");
	}

	return false;
}

////////////////////////////
///        GLShader      ///
////////////////////////////
GLShader::GLShader(const char* path, ShaderType shader_type)
	: id(0)
	, type(shader_type)
{
	sourceFile = string(path);
	sourceCode = ProcessSourceCode(path);
}

string GLShader::ProcessSourceCode(std::string file)
{
	string ln;
	smatch match;
	stringstream processed;

	filesystem::path source_file(file);

	ifstream fin(file);
	while (getline(fin, ln))
	{
		if (regex_match(ln, match, re_include))
		{
			string include_file_path = absolute(source_file).parent_path().append(filesystem::path(match[1].str()).filename().string()).string();
			if (!filesystem::exists(include_file_path))
			{
				throw exception(string("Could not find include file: ").append(include_file_path).c_str());
			}

			processed << ProcessSourceCode(include_file_path) << endl;
		}
		else
		{
			processed << ln << endl;
		}
	}

	return processed.str();
}

bool GLShader::Compile()
{
	int type_id;
	switch (type)
	{
	case ShaderType::Vertex:
		type_id = GL_VERTEX_SHADER;
		break;
	case ShaderType::Fragment:
		type_id = GL_FRAGMENT_SHADER;
		break;
	default:
		throw runtime_error(string("Invalid shader type"));
	}

	id = _GL_WRAP1(glCreateShader, type_id);

	const char* src_ptr = sourceCode.c_str();
	int len = sourceCode.size();

	_GL_WRAP4(glShaderSource, id, 1, &src_ptr, &len);
	_GL_WRAP1(glCompileShader, id);

	if (HasCompileErrors(id))
	{
		return false;
	}

	return true;
}

void GLShader::Discard()
{
	if (id != 0)
	{
		_GL_WRAP1(glDeleteShader, id);
		id = 0;
	}
}

bool GLShader::HasCompileErrors(int id)
{
	int success;
	char message[1024];

	_GL_WRAP3(glGetShaderiv, id, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		_GL_WRAP4(glGetShaderInfoLog, id, 1024, nullptr, message);
		cout << "Shader compilation error(s) (" << sourceFile << "): " << message << endl;
		return true;
	}

	return false;
}