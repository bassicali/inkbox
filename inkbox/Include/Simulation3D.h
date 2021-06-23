
#pragma once

#include "Common.h"
#include "Interface.h"
#include "Camera.h"
#include "Simulation2D.h"

struct SimulationTextures
{
	SimulationTextures(int width, int height, int depth)
		: Velocity(width, height, depth, 4)
		, Pressure(width, height, depth, 4)
		, Ink(width, height, depth, 4)
		, Divergence(width, height, depth, 1)
		, Temp(width, height, depth, 4)
	{
	}

	SwapTexture Ink;
	SwapTexture Velocity;
	SwapTexture Pressure; // TODO: Pressure only needs 1 channel
	Texture Divergence;
	Texture Temp;
};

class InkBox3DSimulation
{
public:
	InkBox3DSimulation(const InkBoxWindows& app, int width, int height, int depth);
	bool CreateScene();
	void WindowLoop();
	void ScrollCallback(double xoffset, double yoffset);

private:
	void ProcessInputs();
	void UpdatePickCoord();
	void TickDropletsMode();
	void ComputeFields();
	void SolvePoissonSystem(SwapTexture& swap, Texture& initial_value, float alpha, float beta);
	void ComputeBoundaryValues(SwapTexture& swap, float scale);
	void CopyImage(Texture& dest, Texture& src);
	void ClearFields();

	std::mutex scrollMtx;
	double scrollAcc;

	GLFWwindow* window;
	int wwidth;
	int wheight;
	FPSLimiter limiter;
	int width;
	int height;
	int depth;
	glm::mat4 cubeModel;
	glm::mat4 projection;
	glm::mat4 invProjView;
	float delta_t;
	bool paused;
	glm::uvec3 computeLocalSize;
	glm::uvec3 computeWorkGroups;
	ImpulseState impulseState;
	SimulationVars vars;
	VarTextBoxes ui;
	ControlPanel controlPanel;

	Camera camera;
	VertexList cube;
	VertexList cubeBorder;
	glm::vec3 cubeVertices[8];

	GLShaderProgram viewShader;
	GLShaderProgram borderShader;
	GLComputeShader impulseShader;
	GLComputeShader advectionShader;
	GLComputeShader jacobiShader;
	GLComputeShader divShader;
	GLComputeShader gradShader;
	GLComputeShader subtractShader;
	GLComputeShader copyShader;
	GLComputeShader clearShader;
	GLComputeShader boundaryShader;

	SimulationTextures textures;
};