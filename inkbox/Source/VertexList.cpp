
#include <glad/glad.h>

#include "Util.h"
#include "VertexList.h"

///////////////////////////
///     VertexList      ///
///////////////////////////

VertexList::VertexList()
    : VAO(0)
    , VBO(0)
    , EBO(0)
{
}

VertexList::~VertexList()
{
    if (EBO != 0)
    {
        _GL_WRAP2(glDeleteBuffers, 1, &EBO);
    }

    if (VBO != 0)
    {
        _GL_WRAP2(glDeleteBuffers, 1, &VBO);
    }

    if (VAO != 0)
    {
        _GL_WRAP2(glDeleteVertexArrays, 1, &VAO);
    }
}

void VertexList::Init(float* vertices, int num_vertices, int* indices, int num_indices)
{
    _GL_WRAP2(glGenVertexArrays, 1, &VAO);
    _GL_WRAP2(glGenBuffers, 1, &VBO);
    _GL_WRAP2(glGenBuffers, 1, &EBO);

    _GL_WRAP1(glBindVertexArray, VAO);

    _GL_WRAP2(glBindBuffer, GL_ARRAY_BUFFER, VBO);
    _GL_WRAP4(glBufferData, GL_ARRAY_BUFFER, sizeof(float) * num_vertices, vertices, GL_STATIC_DRAW);

    _GL_WRAP2(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, EBO);
    _GL_WRAP4(glBufferData, GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * num_indices, indices, GL_STATIC_DRAW);

    _GL_WRAP6(glVertexAttribPointer, 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    _GL_WRAP1(glEnableVertexAttribArray, 0);

    _GL_WRAP1(glBindVertexArray, 0);
}

