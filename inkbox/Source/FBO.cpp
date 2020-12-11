
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "FBO.h"
#include "Common.h"

using namespace std;
using namespace glm;

FBO::FBO()
	: initialized(false)
	, width(0)
	, height(0)
	, textureId(0)
	, fboId(0)
	, Marker(0)
{
}

FBO::FBO(int width, int height, std::optional<glm::vec4> fill)
	: width(width)
	, height(height)
	, format(GL_RGBA)
	, type(GL_FLOAT)
	, internalFormat(GL_RGB16_SNORM)
	, textureId(0)
	, fboId(0)
{
	if (!Init(format, type, internalFormat, fill))
		throw exception("Failed to intialize FBO");
}

FBO::FBO(int width, int height, int format, int type, int internalformat, optional<vec4> fill)
	: width(width)
	, height(height)
	, format(format)
	, type(type)
	, internalFormat(internalformat)
	, textureId(0)
	, fboId(0)
{
	if (!Init(format, type, internalformat, fill))
		throw exception("Failed to intialize FBO");
}

bool FBO::Init(int format, int type, int internalformat, optional<vec4> fill)
{
	_GL_WRAP2(glGenTextures, 1, &textureId);
	_GL_WRAP2(glBindTexture, GL_TEXTURE_2D, textureId);

	_GL_WRAP3(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	_GL_WRAP3(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	_GL_WRAP3(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	_GL_WRAP3(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	_GL_WRAP2(glGenFramebuffers, 1, &fboId);
	_GL_WRAP2(glBindFramebuffer, GL_FRAMEBUFFER, fboId);
	
	_GL_WRAP9(glTexImage2D, GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, nullptr);

	_GL_WRAP5(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;

	_GL_WRAP4(glViewport, 0, 0, width, height);
	if (fill)
	{
		_GL_WRAP4(glClearColor, fill->r, fill->g, fill->b, fill->a);
	}
	else
	{
		_GL_WRAP4(glClearColor, 0, 0, 0, 0);
	}
	_GL_WRAP1(glClear, GL_COLOR_BUFFER_BIT);

	_GL_WRAP2(glBindFramebuffer, GL_FRAMEBUFFER, 0);
	_GL_WRAP2(glBindTexture, GL_TEXTURE_2D, 0);

	initialized = true;
	return true;
}

FBO::~FBO()
{
	if (fboId != 0)
	{
		_GL_WRAP2(glDeleteFramebuffers, 1, &fboId);
	}

	if (textureId != 0)
	{
		_GL_WRAP2(glDeleteTextures, 1, &textureId);
	}
}

void FBO::Clear(float r, float g, float b, float a)
{
	Bind();
	_GL_WRAP4(glClearColor, r, g, b, a);
	_GL_WRAP1(glClear, GL_COLOR_BUFFER_BIT);
	_GL_WRAP2(glBindFramebuffer, GL_FRAMEBUFFER, 0);
}

void FBO::Bind()
{
	_GL_WRAP2(glBindFramebuffer, GL_FRAMEBUFFER, fboId);
	GLenum buffers[1] = { GL_COLOR_ATTACHMENT0 };
	_GL_WRAP1(glDrawBuffer, GL_COLOR_ATTACHMENT0, buffers);
}

void FBO::BindTexture(int unitId)
{
	_GL_WRAP1(glActiveTexture, GL_TEXTURE0 + unitId);
	_GL_WRAP2(glBindTexture, GL_TEXTURE_2D, textureId);
}

void FBO::Resize(int w, int h, GLShaderProgram& shader, VertexList& quad)
{
	width = w;
	height = h;

	int tex_id = textureId;
	int fbo_id = fboId;

	textureId = 0;
	fboId = 0;

	if (!Init(format, type, internalFormat))
		throw exception("Failed to resize FBO");

	Bind();
	shader.Use();
	shader.SetInt("field", 0);
	_GL_WRAP1(glActiveTexture, GL_TEXTURE0);
	_GL_WRAP2(glBindTexture, GL_TEXTURE_2D, tex_id);
	_GL_WRAP1(glBindVertexArray, quad.VAO);
	_GL_WRAP4(glDrawElements, GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}
