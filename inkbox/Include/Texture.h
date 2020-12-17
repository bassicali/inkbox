#pragma once

#include <glad/glad.h>

class Texture
{
public:
    Texture();
    ~Texture();
    Texture(int width, int height, int depth, int channels);
    Texture(int width, int height, int depth, int format, int type, int internalformat);
    bool Init(int width, int height, int depth);
    bool Init(int width, int height, int depth, int format, int type, int internalformat);

    void Bind(int unit_id);
    void BindToImage(int unit_idx, int access);

    int Id() const { return id; }
    int Width() const { return width; }
    int Height() const { return height; }
    int Depth() const { return depth; }

    int TexTarget() const { return depth == 0 ? GL_TEXTURE_2D : GL_TEXTURE_3D; }

private:
    unsigned int id;
    int width;
    int height;
    int depth;
    int format;
    int type;
    int internalFormat;
};

class SwapTexture
{
public:
    SwapTexture(int width, int height, int depth, int channels)
        : tex0(width, height, depth, channels)
        , tex1(width, height, depth, channels)
    {
        ptr0 = &tex0;
        ptr1 = &tex1;
    }

    void Swap()
    {
        Texture* temp = ptr0;
        ptr0 = ptr1;
        ptr1 = temp;
    }

    Texture& Front() { return *ptr0; }
    Texture& Back() { return *ptr1; }

private:
    Texture* ptr0;
    Texture* ptr1;
    Texture tex0;
    Texture tex1;
};