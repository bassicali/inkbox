
#pragma once

struct VertexList
{
	VertexList();
	~VertexList();
	void Init(float* vertices, int num_vertices, int* indices, int num_indices);
	unsigned int VAO;
	unsigned int VBO;
	unsigned int EBO;
};