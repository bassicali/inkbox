
#pragma once

#include <glad/glad.h>

#include "Texture.h"
#include "Shader.h"
#include "VertexList.h"

struct GLFWwindow;

class IFBO
{
public:
	virtual void Bind() = 0;
	virtual void BindTexture(int unit_id) = 0;

	virtual int Id() = 0;
	virtual int TextureId() = 0;

	virtual void Resize(int w, int h, GLShaderProgram& shader, VertexList& quad) = 0;
	virtual void Clear(float r = 0.f, float g = 0.f, float b = 0.f, float a = 0.0f) = 0;
};

class FBO : public IFBO
{
public:
	FBO();
	FBO(int width, int height, int depth = 0, int channels = 3);
	FBO(int width, int height, int depth, int format, int type, int internalformat = 0);
	~FBO();
	bool Init();
	virtual void Clear(float r = 0.f, float g = 0.f, float b = 0.f, float a = 0.0f) override;
	virtual void Bind() override;
	virtual void BindTexture(int unit_id) override { texture.Bind(unit_id); }

	virtual int Id() override { return fboId; }
	virtual int TextureId() override { return texture.Id(); }

	virtual void Resize(int w, int h, GLShaderProgram& shader, VertexList& quad) override;
	int Width() const { return width; }
	int Height() const { return height; }

private:
	bool initialized;
	int width;
	int height;
	int depth;
	Texture texture;

	unsigned int fboId;
};

class SwapFBO : public IFBO
{
public:
	SwapFBO(int width, int height, int depth)
		: w0(width, height, depth)
		, w1(width, height, depth)
		, ptr0(&w0)
		, ptr1(&w1)
	{}

	void Swap()
	{
		FBO* temp = ptr0;
		ptr0 = ptr1;
		ptr1 = temp;
	}

	virtual void Bind() override { ptr0->Bind(); }
	virtual void BindTexture(int unitId) override { ptr0->BindTexture(unitId); }
	virtual int Id() override { return ptr0->Id(); }
	virtual int TextureId() override { return ptr0->TextureId(); }
	virtual void Resize(int w, int h, GLShaderProgram& shader, VertexList& quad) override
	{ 
		w0.Resize(w, h, shader, quad);
		w1.Resize(w, h, shader, quad);
	}
	virtual void Clear(float r = 0.f, float g = 0.f, float b = 0.f, float a = 0.0f) override
	{
		w0.Clear(r, g, b, a);
		w1.Clear(r, g, b, a);
	}

	FBO& Front() const { return *ptr0; }
	FBO& Back() const { return *ptr1; }

private:
	FBO w0;
	FBO w1;

	FBO* ptr0;
	FBO* ptr1;
};