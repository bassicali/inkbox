#pragma once

#include <functional>

#include <glm/vec2.hpp>

#include "FBO.h"
#include "Shader.h"
#include "VertexList.h"

class ShaderOp
{
public:
	ShaderOp();
	ShaderOp(int width, int height, float gridscale);
	virtual void Compute() = 0;

	IFBO& OutputFBO() { return *outputFBO; }
	GLShaderProgram& Shader() { return *program; }

	void Use();

	void SetShader(GLShaderProgram* shader) { program = shader; }
	void SetOutput(IFBO* fbo) { outputFBO = fbo; }
	void SetDimensions(glm::vec2 dims) { dimensions = dims; }
	void SetGridScale(float scale) { gridScale = scale; }
protected:
	glm::vec2 dimensions;
	float gridScale;
	IFBO* outputFBO;
	GLShaderProgram* program;
};

template<typename TFunc>
class _QuadShaderOp : public ShaderOp
{
public:
	using ShaderOp::ShaderOp;

	virtual void Compute() override;
	void Draw();
	void SetQuad(VertexList* quad);
	void SetUniformsFunc(TFunc f);
	void SetUniforms();
protected:
	VertexList* quad;
	TFunc setUniforms;
};

class BorderShaderOp : public ShaderOp
{
public:
	using ShaderOp::ShaderOp;

	virtual void Compute() override;
	void SetLines(VertexList* t, VertexList* b, VertexList* l, VertexList* r);
private:
	VertexList* top;
	VertexList* bottom;
	VertexList* left;
	VertexList* right;
};

typedef _QuadShaderOp<std::function<void(GLShaderProgram&)>> QuadShaderOp;