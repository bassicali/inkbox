
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
	, depth(0)
	, fboId(0)
{
}

FBO::FBO(int width, int height, int depth, int channels)
	: width(width)
	, height(height)
	, depth(depth)
	, texture(width, height, depth, channels)
	, fboId(0)
{
	if (!Init())
		throw exception("Failed to intialize FBO");
}

FBO::FBO(int width, int height, int depth, int format, int type, int internalformat)
	: width(width)
	, height(height)
	, depth(depth)
	, texture(width, height, depth, format, type, internalformat)
	, fboId(0)
{
	if (!Init())
		throw exception("Failed to intialize FBO");
}

bool FBO::Init()
{
	_GL_WRAP2(glGenFramebuffers, 1, &fboId);
	_GL_WRAP2(glBindFramebuffer, GL_FRAMEBUFFER, fboId);
	
	// Attach texture to this fbo
	if (depth == 0)
	{
		_GL_WRAP5(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture.TexTarget(), texture.Id(), 0);
	}
	else
	{
		_GL_WRAP6(glFramebufferTexture3D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture.TexTarget(), texture.Id(), 0, 0);
	}

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;

	_GL_WRAP4(glViewport, 0, 0, width, height);
	_GL_WRAP4(glClearColor, 0, 0, 0, 0);
	_GL_WRAP1(glClear, GL_COLOR_BUFFER_BIT);

	_GL_WRAP2(glBindFramebuffer, GL_FRAMEBUFFER, 0);

	initialized = true;
	return true;
}

FBO::~FBO()
{
	if (fboId != 0)
	{
		_GL_WRAP2(glDeleteFramebuffers, 1, &fboId);
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
	_GL_WRAP1(glDrawBuffer, GL_COLOR_ATTACHMENT0);
}

void FBO::Resize(int w, int h, GLShaderProgram& shader, VertexList& quad)
{
	width = w;
	height = h;

	// Temp
	unsigned int tex_id = texture.Id();
	unsigned int fbo_id = fboId;

	fboId = 0;

	if (!texture.Init(width, height, depth))
		throw exception("Failed to resize texture");

	if (!Init())
		throw exception("Failed to resize FBO");

	// Draw old texture onto new one
	Bind();
	shader.Use();
	shader.SetInt("field", 0);
	_GL_WRAP1(glActiveTexture, GL_TEXTURE0);
	_GL_WRAP2(glBindTexture, texture.TexTarget(), tex_id);
	_GL_WRAP1(glBindVertexArray, quad.VAO);
	_GL_WRAP4(glDrawElements, GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

	// Delete old stuff
	_GL_WRAP2(glDeleteTextures, 1, &tex_id);
	_GL_WRAP2(glDeleteFramebuffers, 1, &fbo_id);
}
