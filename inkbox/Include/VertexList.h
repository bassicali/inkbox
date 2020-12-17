
#pragma once

struct VertexList
{
	VertexList();
	~VertexList();
	void Init(float* vertices, int num_vertices, unsigned int* indices, int num_indices);
	int NumVertices;
	unsigned int VAO;
	unsigned int VBO;
	unsigned int EBO;
};