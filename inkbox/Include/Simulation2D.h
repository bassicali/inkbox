
#pragma once

#include <mutex>

#include <glm/vec2.hpp>

#include "Common.h"
#include "Shader.h"
#include "ShaderOp.h"
#include "FBO.h"
#include "VertexList.h"
#include "Interface.h"

#define NUM_JACOBI_ROUNDS 30

struct SimulationFields
{
	SimulationFields(int width, int height, int depth = 0);
	FBO& Get(SimulationField field);
	void Resize(int w, int h, GLShaderProgram& shader, VertexList& quad);
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

class InkBox2DSimulation
{
	friend struct InkBoxWindows;

public:
	InkBox2DSimulation(const InkBoxWindows& app, int width, int height);

	bool CreateScene();
	void WindowLoop();
	void Terminate();
	
	void DrawQuad();
	void SetDimensions(int w, int h);
	void CopyFBO(FBO& dest, FBO& src);
	
private:

	bool CreateShaderOps();
	void ProcessInputs();

	SimulationFields fbos;
	SimulationVars vars;
	ControlPanel controlPanel;
	float delta_t;
	void ComputeFields();
	void ComputeBoundaryValues(SwapFBO& swap, float scale);
	void SolvePoissonSystem(SwapFBO& swap, FBO& initial_value, float alpha, float beta);
	void SolvePoissonSystem(SwapFBO& swap, float alpha, float beta);
	void TickDropletsMode();
	glm::vec2 RandPos();

	QuadShaderOp impulse;
	QuadShaderOp radialImpulse;
	QuadShaderOp vorticity;
	QuadShaderOp addVorticity;
	QuadShaderOp advection;
	QuadShaderOp poissonSolver;
	QuadShaderOp gradient;
	QuadShaderOp divergence;
	QuadShaderOp subtract;

	GLFWwindow* window;
	FPSLimiter limiter;
	int width;
	int height;
	glm::vec2 rdv;
	ImpulseState impulseState;
	VarTextBoxes ui;
	bool paused;

	VertexList quad;
	VertexList borderT;
	VertexList borderB;
	VertexList borderL;
	VertexList borderR;

	GLShaderProgram impulseShader;
	GLShaderProgram radialImpulseShader;
	GLShaderProgram advectionShader;
	GLShaderProgram jacobiShader;
	GLShaderProgram divShader;
	GLShaderProgram gradShader;
	GLShaderProgram subtractShader;
	GLShaderProgram boundaryShader;
	GLShaderProgram vorticityShader;
	GLShaderProgram addVorticityShader;
	GLShaderProgram scalarVisShader;
	GLShaderProgram inkVisShader;
	GLShaderProgram vectorVisShader;
	GLShaderProgram copyShader;
};