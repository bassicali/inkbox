
#include <stdexcept>

#include <glad/glad.h>

#include "Util.h"
#include "ShaderOp.h"

using namespace std;

///////////////////////////
///      ShaderOp       ///
///////////////////////////
ShaderOp::ShaderOp()
	: dimensions(0, 0)
	, gridScale(0)
	, outputFBO(nullptr)
	, program(nullptr)
{
}

ShaderOp::ShaderOp(int width, int height, float gridscale)
	: dimensions(width, height)
	, gridScale(gridscale)
	, outputFBO(nullptr)
	, program(nullptr)
{
}

void ShaderOp::Use()
{
	if (program)
		program->Use();
}


///////////////////////////
///     QuadShaderOp    ///
///////////////////////////

template<typename TFunc>
void _QuadShaderOp<TFunc>::Compute()
{
	if (outputFBO == nullptr)
		throw runtime_error("Output buffer not set for shader op");

	program->Use();
	outputFBO->Bind();
	SetUniforms();
	Draw();
}

template<typename TFunc>
void _QuadShaderOp<TFunc>::Draw()
{
	_GL_WRAP1(glBindVertexArray, quad->VAO);
	_GL_WRAP4(glDrawElements, GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

template<typename TFunc>
void _QuadShaderOp<TFunc>::SetQuad(VertexList* quad)
{
	this->quad = quad;
}

template<typename TFunc>
void _QuadShaderOp<TFunc>::SetUniformsFunc(TFunc f)
{
	setUniforms = f;
}

template<typename TFunc>
void _QuadShaderOp<TFunc>::SetUniforms()
{
	if (setUniforms) 
		setUniforms(*program);
}

template void _QuadShaderOp<std::function<void(GLShaderProgram&)>>::Draw();
template void _QuadShaderOp<std::function<void(GLShaderProgram&)>>::Compute();
template void _QuadShaderOp<std::function<void(GLShaderProgram&)>>::SetQuad(VertexList* quad);
template void _QuadShaderOp<std::function<void(GLShaderProgram&)>>::SetUniformsFunc(std::function<void(GLShaderProgram&)> f);
template void _QuadShaderOp<std::function<void(GLShaderProgram&)>>::SetUniforms();

///////////////////////////
///   BorderShaderOp    ///
///////////////////////////

void BorderShaderOp::Compute()
{
	program->Use();
	outputFBO->Bind();

	program->SetVec2("offset", 0, -1); // 1 texel down
	_GL_WRAP1(glBindVertexArray, top->VAO);
	_GL_WRAP4(glDrawElements, GL_LINES, 2, GL_UNSIGNED_INT, nullptr);

	program->SetVec2("offset", 0, 1); // 1 texel up
	_GL_WRAP1(glBindVertexArray, bottom->VAO);
	_GL_WRAP4(glDrawElements, GL_LINES, 2, GL_UNSIGNED_INT, nullptr);

	program->SetVec2("offset", 1, 0); // 1 texel to the right
	_GL_WRAP1(glBindVertexArray, left->VAO);
	_GL_WRAP4(glDrawElements, GL_LINES, 2, GL_UNSIGNED_INT, nullptr);

	program->SetVec2("offset", -1, 0); // 1 texel to the left
	_GL_WRAP1(glBindVertexArray, right->VAO);
	_GL_WRAP4(glDrawElements, GL_LINES, 2, GL_UNSIGNED_INT, nullptr);
}

void BorderShaderOp::SetLines(VertexList* t, VertexList* b, VertexList* l, VertexList* r)
{
	top = t;
	bottom = b;
	left = l;
	right = r;
}
