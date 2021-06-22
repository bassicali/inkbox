
#include <iomanip>
#include <regex>
#include <filesystem>
#include <fstream>

#include "IniConfig.h"
#include "Common.h"
#include "Utils.h"

#define CONFIG_FILE_NAME "inkbox.ini"

using namespace std;
using namespace utils;
namespace fs = std::filesystem;

regex re_header("^\\[(\\w+)\\]$", regex_constants::optimize | regex_constants::ECMAScript);
regex re_setting("^(\\w+)=(.+)$", regex_constants::optimize | regex_constants::ECMAScript);

// Singleton
IniConfig& IniConfig::Get()
{
	static IniConfig config;
	return config;
}

IniConfig::IniConfig()
	: NumJacobiIterations(4)
	, ScrollSensitivity(0.08)
	, MouseOrbitSensitivity(0.008)
	, KeyOrbitSensitivity(0.06)
	, TextureComponentWidth(16)
	, UseSnormTextures(false)
{
	fs::path config_path(CONFIG_FILE_NAME);

	if (fs::exists(config_path))
	{
		ifstream fin(CONFIG_FILE_NAME, ios::in);
		if (fin.good())
		{
			Load(fin);
		}
	}

	Save();
}

void IniConfig::Save()
{
#define WRITE_SETTING(name) fout << #name << "=" << this->name << endl
#define WRITE_HEX_SETTING(name) fout << hex << uppercase << #name << "=" << this->name << nouppercase << "h" << endl << dec

	ofstream fout(CONFIG_FILE_NAME, ios::out);
	if (fout.good())
	{
		fout << "[inkbox]" << endl;
		WRITE_SETTING(NumJacobiIterations);
		WRITE_SETTING(ScrollSensitivity);
		WRITE_SETTING(MouseOrbitSensitivity);
		WRITE_SETTING(KeyOrbitSensitivity);
		WRITE_SETTING(TextureComponentWidth);
		WRITE_SETTING(UseSnormTextures);
	}
	else
	{
		LOG_ERROR("Failed to save config file");
	}
}

void IniConfig::Load(std::istream& stream)
{
#define PARSE_BOOL(key,value,name) if (StringEquals(key, #name)) { this->name = (bool)stoi(value); continue; }
#define PARSE_INT(key,value,name) if (StringEquals(key, #name)) { this->name = ParseNumericString(value); continue; }
#define PARSE_FLOAT(key,value,name) if (StringEquals(key, #name)) { this->name = stof(value); continue; }
#define PARSE_STR(key,value,name) if (StringEquals(key, #name)) { this->name = value; continue; }

	string line;
	string section;
	smatch match;

	while (!stream.eof())
	{
		getline(stream, line);

		if (StringStartsWith(line, "#"))
			continue;

		if (regex_match(line, match, re_header))
		{
			section = match[1].str();
		}
		else if (StringEquals(section, "inkbox") && regex_match(line, match, re_setting))
		{
			string key = match[1].str();
			string value = match[2].str();

			PARSE_INT(key, value, NumJacobiIterations)
			PARSE_FLOAT(key, value, ScrollSensitivity)
			PARSE_FLOAT(key, value, MouseOrbitSensitivity)
			PARSE_FLOAT(key, value, KeyOrbitSensitivity)
			PARSE_INT(key, value, TextureComponentWidth)
			PARSE_BOOL(key, value, UseSnormTextures)
		}
	}

	if (NumJacobiIterations % 2 != 0)
		NumJacobiIterations++;
	else if (NumJacobiIterations == 0)
		NumJacobiIterations = 2;
}

void IniConfig::Print()
{
	LOG_INFO("Config value:");
	LOG_INFO("\tNumJacobiIterations: %d", NumJacobiIterations);
	LOG_INFO("\tScrollSensitivity: %.2f", ScrollSensitivity);
	LOG_INFO("\tMouseOrbitSensitivity: %.2f", MouseOrbitSensitivity);
	LOG_INFO("\tKeyOrbitSensitivity: %.2f", KeyOrbitSensitivity);
	LOG_INFO("\tTextureComponentWidth: %d", TextureComponentWidth);
	LOG_INFO("\tUseSnormTextures: %d", UseSnormTextures);
}
