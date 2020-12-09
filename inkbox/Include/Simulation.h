
#pragma once

#include <vector>

#include <glm/vec2.hpp>

#include "Shader.h"
#include "ShaderOp.h"
#include "FBO.h"
#include "VertexList.h"
#include "Interface.h"

#define NUM_JACOBI_ROUNDS 30

struct SimulationFields
{
	SimulationFields(int width, int height);
	SwapFBO Velocity;
	FBO Vorticity;
	SwapFBO Pressure;
	SwapFBO Ink;
	FBO Temp;

	FBO VelocityVis;
	FBO PressureVis;
	FBO VorticityVis;
	FBO InkVis;
};

class InkBoxSimulation
{
	friend class InkBoxWindows;

public:
	InkBoxSimulation(const InkBoxWindows& app, int width, int height);

	bool CreateScene();
	void WindowLoop();
	void Terminate();
	
	void DrawQuad();
	void SetDimensions(int w, int h);
	void CopyFBO(FBO& dest, FBO& src);
	
private:

	static InkBoxSimulation* s_Instance;

	bool CreateShaderOps();

	SimulationFields fbos;
	SimulationVars vars;
	ControlPanel controlPanel;
	float timestep;
	void ComputeFields(float delta_t);
	void ComputeBoundaryValues(SwapFBO& swap, float scale);
	void SolvePoissonSystem(SwapFBO& swap, FBO& initial_value, float alpha, float beta);
	void SolvePoissonSystem(SwapFBO& swap, float alpha, float beta);
	void TickDropletsMode();
	glm::vec2 RandPos();

	QuadShaderOp impulse;
	QuadShaderOp vorticity;
	QuadShaderOp addVorticity;
	QuadShaderOp advection;
	QuadShaderOp poissonSolver;
	QuadShaderOp gradient;
	QuadShaderOp divergence;
	QuadShaderOp subtract;
	BorderShaderOp boundaries;

	void ProcessInput();

	GLFWwindow* mainWindow;
	GLFWwindow* uiWindow;
	int width;
	int height;
	glm::vec2 rdv;
	ImpulseState impulseState;
	VarTextBoxes ui;

	VertexList quad;
	VertexList borderT;
	VertexList borderB;
	VertexList borderL;
	VertexList borderR;

	GLShaderProgram impulseShader;
	GLShaderProgram advectionShader;
	GLShaderProgram jacobiShader;
	GLShaderProgram divShader;
	GLShaderProgram gradShader;
	GLShaderProgram subtractShader;
	GLShaderProgram boundaryShader;
	GLShaderProgram vorticityShader;
	GLShaderProgram addVorticityShader;
	GLShaderProgram scalarVisShader;
	GLShaderProgram vectorVisShader;
	GLShaderProgram copyShader;
};