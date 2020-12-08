
#pragma once

#include <optional>

#include "Shader.h"

struct GLFWwindow;

class FBO
{
public:
	FBO();
	FBO(int width, int height, std::optional<glm::vec4> fill = std::optional<glm::vec4>());
	~FBO();
	bool Init(std::optional<glm::vec4> fill = std::optional<glm::vec4>());
	void Clear(float r = 0.f, float g = 0.f, float b = 0.f, float a = 0.0f);
	void Bind();
	void BindTexture(int unitId);
	void CopyTo(FBO& other, GLShaderProgram& shader);

	int Id() const { return fboId; }
	int TextureId() const { return textureId; }

	void SetDimensions(int w, int h);

private:
	bool initialized;
	int width;
	int height;

	unsigned int textureId;
	unsigned int fboId;
};