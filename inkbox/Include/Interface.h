
#pragma once

#include <chrono>

#include <glm/vec2.hpp>

#include "FBO.h"

#define TEXTBOX_LEN 16
#define TEXTBUFF_LEN (TEXTBOX_LEN * sizeof(char))
#define MAIN_WINDOW_TITLE " i n k b o x "
#define CONTROLS_WINDLW_TITLE " c o n t r o l s "
#define FRAME_DELAY_ADJUSTMENT_MOD 4

struct GLFWwindow;

struct InkBoxWindows
{
	InkBoxWindows();
	~InkBoxWindows();

	bool InitGLContexts(int width, int height, int ctrl_width, int ctrl_height);

	GLFWwindow* Main;
	GLFWwindow* Controls;

	glm::vec2 ViewportSize;
};

enum class SimulationField
{
	Ink,
	Velocity,
	Pressure,
	Vorticity
};

struct ImpulseState
{
	ImpulseState();

	void Update(float x, float y, bool left_down, bool right_down);
	void Reset();
	bool IsActive() const { return InkActive || ForceActive; }

	glm::vec3 LastPos;
	glm::vec3 CurrentPos;
	bool ForceActive;
	bool InkActive;
	bool Radial;
	glm::vec3 Delta;
};

struct SimulationVars
{
	SimulationVars();

	bool SelfAdvect;
	bool AdvectInk;
	bool DiffuseVelocity;
	bool DiffuseInk;
	bool PressureEnabled;
	bool AddVorticity;
	bool BoundariesEnabled;
	bool DropletsMode;
	float GridScale;
	float SplatRadius;
	float InkVolume;
	float Viscosity;
	float InkViscosity;
	float Vorticity;
	float AdvectionDissipation;
	float InkAdvectionDissipation;
	float Gravity;
	float ForceMultiplier;
	glm::vec4 InkColour;
	SimulationField DisplayField;
};

struct VarTextBoxes
{
	VarTextBoxes();
	void SetValues(float gridscale, float viscosity, float ink_viscosity, float vorticity, float splat_radius, float adv_dissipation, float ink_adv_dissipation, float ink_volume, float gravity, float fmult);
	void SetValues(SimulationVars& vars);
	void UpdateVars(SimulationVars& vars);
	char GridScale[TEXTBOX_LEN];
	char AdvDissipation[TEXTBOX_LEN];
	char InkAdvDissipation[TEXTBOX_LEN];
	char SplatRadius[TEXTBOX_LEN];
	char Viscosity[TEXTBOX_LEN];
	char InkViscosity[TEXTBOX_LEN];
	char Vorticity[TEXTBOX_LEN];
	char Gravity[TEXTBOX_LEN];
	char InkVolume[TEXTBOX_LEN];
	char ForceMultiplier[TEXTBOX_LEN];
};

class ControlPanel
{
public:
	ControlPanel();
	ControlPanel(GLFWwindow* win, SimulationVars* vars, VarTextBoxes* texts, ImpulseState* impulse, FBO* ufbo, FBO* pfbo, FBO* ifbo, FBO* vfbo);
	ControlPanel(GLFWwindow* win, SimulationVars* vars, VarTextBoxes* texts, ImpulseState* impulse);
	void Render(bool& update_vars, bool& clear_buffers);
	GLFWwindow* WindowPtr() const { return window; }

private:
	GLFWwindow* window;
	SimulationVars* simvars;
	VarTextBoxes* texts;
	ImpulseState* impulse;
	bool is3D;
	FBO* velocity;
	FBO* vorticity;
	FBO* pressure;
	FBO* ink;
};

class FPSLimiter
{
public:
	FPSLimiter(int fps);
	void Regulate();
	float AverageFPS() const { return 1000 / avgFrameTime; }
private:
	float simFrameTime;
	float avgFrameTime;
	int fpsDelay;
	int adjustmentCtr;
	std::chrono::steady_clock::time_point lastTime;
};