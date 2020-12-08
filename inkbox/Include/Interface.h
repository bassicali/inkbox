
#pragma once

#include <glm/vec2.hpp>

#define TEXTBOX_LEN 16
#define TEXTBUFF_LEN (TEXTBOX_LEN * sizeof(char))

struct GLFWwindow;

struct InkBoxWindows
{
	InkBoxWindows();
	~InkBoxWindows();

	bool InitGLContexts(int width, int height);

	GLFWwindow* Main;
	GLFWwindow* Settings;

	glm::vec2 ViewportSize;
};

enum class SplatMode
{
	Continuous,
	ClickAndRelease
};

enum class SimulationField
{
	Ink,
	Velocity,
	Pressure,
	Vorticity
};

struct CursorState
{
	CursorState();

	void Update(float x, float y, bool buttonDown);
	bool IsActive() const;
	void Reset();

	SplatMode Mode;
	bool ButtonReleased;
	bool ButtonDown;
	glm::vec2 LastPos;
	glm::vec2 CurrentPos;

	glm::vec2 Diff;
};

struct SimulationVars
{
	SimulationVars();

	bool SelfAdvect;
	bool AdvectInk;
	bool DiffuseVelocity;
	bool DiffuseInk;
	bool ExternalForces;
	bool PressureEnabled;
	bool AddVorticity;
	bool BoundariesEnabled;
	float GridScale;
	float SplatRadius;
	float InkVolume;
	float Viscosity;
	float InkViscosity;
	float Vorticity;
	float AdvectionDissipation;
	float InkAdvectionDissipation;
	SimulationField DisplayField;
};

struct VarTextBoxes
{
	VarTextBoxes();
	void SetValues(float gridscale, float viscosity, float ink_viscosity, float vorticity, float splat_radius, float adv_dissipation, float ink_adv_dissipation, float ink_volume);
	void UpdateVars(SimulationVars& vars);
	char GridScale[TEXTBOX_LEN];
	char AdvDissipation[TEXTBOX_LEN];
	char InkAdvDissipation[TEXTBOX_LEN];
	char SplatRadius[TEXTBOX_LEN];
	char Viscosity[TEXTBOX_LEN];
	char InkViscosity[TEXTBOX_LEN];
	char Vorticity[TEXTBOX_LEN];
	char InkVolume[TEXTBOX_LEN];
};