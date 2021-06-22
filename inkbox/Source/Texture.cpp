#pragma once

#include "Texture.h"
#include "Common.h"
#include "IniConfig.h"

#include <exception>

using namespace std;

Texture::Texture()
	: width(0)
	, height(0)
	, depth(0)
	, format(0)
	, internalFormat(0)
	, type(0)
	, id(0)
{
}

Texture::Texture(int width, int height, int depth, int channels)
{
	bool snorm = IniConfig::Get().UseSnormTextures;
	int component_width = IniConfig::Get().TextureComponentWidth;

	if (channels == 4)
	{
		format = GL_RGBA;
		internalFormat = snorm ? GL_RGBA16_SNORM : (component_width == 32 ? GL_RGBA32F : GL_RGBA16F);
	}
	else if (channels == 3)
	{
		format = GL_RGB;
		internalFormat = snorm ? GL_RGB16_SNORM : (component_width == 32 ? GL_RGB32F : GL_RGB16F);
	}
	else if (channels == 2)
	{
		format = GL_RG;
		internalFormat = snorm ? GL_RG16_SNORM : (component_width == 32 ? GL_RG32F : GL_RG16F);
	}
	else if (channels == 1)
	{
		format = GL_RED;
		internalFormat = snorm ? GL_R16_SNORM : (component_width == 32 ? GL_R32F : GL_R16F);
	}
	else
	{
		throw exception("Invalid number of channels");
	}

	if (!Init(width, height, depth, format, GL_FLOAT, internalFormat))
		throw exception("Failed to create texture");
}

Texture::~Texture()
{
	if (id != 0)
	{
		_GL_WRAP2(glDeleteTextures, 1, &id);
	}
}

Texture::Texture(int width, int height, int depth, int format, int type, int internalformat)
{
	if (!Init(width, height, depth, format, type, internalformat))
		throw exception("Failed to create texture");
}

bool Texture::Init(int width, int height, int depth)
{
	return Init(width, height, depth, format, type, internalFormat);
}

bool Texture::Init(int width, int height, int depth, int format, int type, int internalformat)
{
	this->width = width;
	this->height = height;
	this->depth = depth;
	this->format = format;
	this->type = type;
	this->internalFormat = internalformat;

	int target = depth == 0 ? GL_TEXTURE_2D : GL_TEXTURE_3D;

	_GL_WRAP2(glGenTextures, 1, &id);
	_GL_WRAP2(glBindTexture, target, id);

	_GL_WRAP3(glTexParameteri, target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	_GL_WRAP3(glTexParameteri, target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	_GL_WRAP3(glTexParameteri, target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	_GL_WRAP3(glTexParameteri, target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (depth == 0)
	{
		_GL_WRAP9(glTexImage2D, target, 0, internalformat, width, height, 0, format, type, nullptr);
	}
	else
	{
		_GL_WRAP10(glTexImage3D, target, 0, internalformat, width, height, depth, 0, format, type, nullptr);
	}

	_GL_WRAP2(glBindTexture, target, 0);

	return true;
}

void Texture::Bind(int unit_id)
{
	_GL_WRAP1(glActiveTexture, GL_TEXTURE0 + unit_id);
	_GL_WRAP2(glBindTexture, TexTarget(), id);
}

void Texture::BindToImage(int unit_idx, int access)
{
	bool layered = depth > 0;
	_GL_WRAP7(glBindImageTexture, unit_idx, id, 0, layered, 0, (GLenum)access, internalFormat);
}