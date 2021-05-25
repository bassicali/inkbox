
#pragma once

#include <fstream>


struct IniConfig
{
	IniConfig();
	void Save();
	void Load(std::istream& stream);
	void Print();

	int NumJacobiIterations;
	float ScrollSensitivity;
	float MouseOrbitSensitivity;
	float KeyOrbitSensitivity;

	int TextureComponentWidth;
	bool UseSnormTextures;

	static IniConfig& Get();
};