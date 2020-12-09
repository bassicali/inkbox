
#pragma once

#include <optional>

#include "Shader.h"

struct GLFWwindow;

class IFBO
{
public:
	virtual void Bind() = 0;
	virtual void BindTexture(int unitId) = 0;

	virtual int Id() = 0;
	virtual int TextureId() = 0;

};

class FBO : public IFBO
{
public:
	FBO();
	FBO(int width, int height, std::optional<glm::vec4> fill = std::optional<glm::vec4>());
	FBO(int width, int height, int format, int type, int internalformat = 0, std::optional<glm::vec4> fill = std::optional<glm::vec4>());
	~FBO();
	bool Init(int format, int type, int internalformat = 0, std::optional<glm::vec4> fill = std::optional<glm::vec4>());
	void Clear(float r = 0.f, float g = 0.f, float b = 0.f, float a = 0.0f);
	virtual void Bind() override;
	virtual void BindTexture(int unitId) override;

	virtual int Id() override { return fboId; }
	virtual int TextureId() override { return textureId; }

	void SetDimensions(int w, int h);
	int Width() const { return width; }
	int Height() const { return height; }

	int Marker;

private:
	bool initialized;
	int width;
	int height;

	unsigned int textureId;
	unsigned int fboId;
};

class SwapFBO : public IFBO
{
public:
	SwapFBO(int width, int height)
		: w0(width, height)
		, w1(width, height)
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

	FBO& Front() const { return *ptr0; }
	FBO& Back() const { return *ptr1; }

private:
	FBO w0;
	FBO w1;

	FBO* ptr0;
	FBO* ptr1;
};