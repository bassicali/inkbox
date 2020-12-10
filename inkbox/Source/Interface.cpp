
#include "Interface.h"

#include <string>
#include <iostream>
#include <cstdio>
#include <regex>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "../resource.h"
#include "Util.h"

#define UI_WIN_W 745
#define UI_WIN_H 590

using namespace std;
using namespace glm;

void GLErrorCallback(int error_code, const char* description)
{
    LOG_ERROR("GLFW error %08X: %s", error_code, description);
}

void _BufferResizeCallback(GLFWwindow* window, int width, int height)
{
    //if (s_Instance != nullptr)
    //{
    //    s_Instance->width = width;
    //    s_Instance->height = height;
    //    s_Instance->rdv = vec2(1.0f / width, 1.0f / height);
    //}

    //_GL_WRAP4(glViewport, 0, 0, width, height);
    //LOG_INFO("glViewport: %d, %d", width, height);
}

InkBoxWindows::InkBoxWindows()
    : Main(nullptr)
    , Settings(nullptr)
    , ViewportSize(0,0)
{}

InkBoxWindows::~InkBoxWindows()
{
    if (Main)
        glfwDestroyWindow(Main);

    if (Settings)
        glfwDestroyWindow(Settings);
}

bool InkBoxWindows::InitGLContexts(int width, int height)
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (glfwInit() != GLFW_TRUE)
    {
        cout << "Failed to initialize GL" << endl;
        return false;
    }

    Main = glfwCreateWindow(width, height, " i n k b o x ", nullptr, nullptr);
    if (Main == nullptr)
    {
        cout << "Failed to create GLFW window" << endl;
        return false;
    }

    glfwWindowHint(GLFW_RESIZABLE, 0);
    Settings = glfwCreateWindow(UI_WIN_W, UI_WIN_H, " c o n t r o l s ", nullptr, Main);
    if (Settings == nullptr)
    {
        cout << "Failed to create GLFW window" << endl;
        return false;
    }

    glfwMakeContextCurrent(Main);
    glfwSetFramebufferSizeCallback(Main, _BufferResizeCallback);
    glfwSetErrorCallback(GLErrorCallback);
    glfwSetInputMode(Main, GLFW_STICKY_KEYS, true);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GL loader" << endl;
        return false;
    }

    HINSTANCE hinst = GetModuleHandle(nullptr);
    HICON hico = LoadIcon(hinst, MAKEINTRESOURCE(IDI_ICON1));

    HWND hwnd = glfwGetWin32Window(Main);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hico);
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hico);

    hwnd = glfwGetWin32Window(Settings);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hico);
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hico);

    hwnd = GetConsoleWindow();
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hico);
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hico);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    ImGui_ImplGlfw_InitForOpenGL(Settings, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    ViewportSize = vec2(width, height);
    return true;
}


///////////////////////////
///    ControlPanel     ///
///////////////////////////

ControlPanel::ControlPanel()
    : window(nullptr)
    , simvars(nullptr)
    , texts(nullptr)
    , impulse(nullptr)
    , velocity(nullptr)
    , pressure(nullptr)
    , ink(nullptr)
    , vorticity(nullptr)
{
}

ControlPanel::ControlPanel(GLFWwindow* win, SimulationVars* vars, VarTextBoxes* texts, ImpulseState* impulse, FBO* ufbo, FBO* pfbo, FBO* ifbo, FBO* vfbo)
    : window(win)
    , simvars(vars)
    , texts(texts)
    , impulse(impulse)
    , velocity(ufbo)
    , pressure(pfbo)
    , ink(ifbo)
    , vorticity(vfbo)
{
}

void ControlPanel::Render()
{
#define TEXTBOX(text,var) ImGui::SetNextItemWidth(80); ImGui::InputText((text), (var), 16);

    static ImVec2 uv_min = ImVec2(0.0f, 1.0f);                 // Top-left
    static ImVec2 uv_max = ImVec2(1.0f, 0.0f);                 // Lower-right
    static ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
    static ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
    //static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    static ImVec4 clear_color = ImVec4(0.59f, 0.49f, 0.78f, 1.00f);
    static bool update = 0;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Settings");

    ImGui::Text("Constants");
    ImGui::Checkbox("Self Advection", &simvars->SelfAdvect);
    ImGui::Checkbox("Ink Advection", &simvars->AdvectInk);
    ImGui::Checkbox("Vorticity", &simvars->AddVorticity);
    ImGui::Checkbox("Diffusion", &simvars->DiffuseVelocity);
    ImGui::Checkbox("Ink Diffusion", &simvars->DiffuseInk);
    ImGui::Checkbox("External Forces", &simvars->ExternalForces);
    ImGui::Checkbox("Boundary Conditions", &simvars->BoundariesEnabled);

    // ImVec4 and glm::vec4 have the same layout
    ImGui::ColorEdit4("Ink Colour", (float*)&simvars->InkColour, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    ImGui::SameLine();
    ImGui::Text("Ink Colour");

    ImGui::Separator();
    ImGui::Text("Variables");
    TEXTBOX("Grid Scale", texts->GridScale);
    TEXTBOX("Viscosity", texts->Viscosity);
    TEXTBOX("Ink Viscosity", texts->InkViscosity);
    TEXTBOX("Vorticity##3", texts->Vorticity);
    TEXTBOX("Adv. Dissipation", texts->AdvDissipation);
    TEXTBOX("Ink Dissipation", texts->InkAdvDissipation);
    TEXTBOX("Force Radius", texts->SplatRadius);
    TEXTBOX("Ink Volume", texts->InkVolume);
    ImGui::Checkbox("Droplets", &simvars->DropletsMode);

    ImGui::Separator();
    if (ImGui::Button("Update"))
        update = true;
    if (update)
    {
        texts->UpdateVars(*simvars);
        update = false;
    }

    ImGui::Separator();
    ImGui::Text("Display Field");
    int* radio_field = (int*)(&simvars->DisplayField);
    ImGui::RadioButton("Ink", radio_field, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Velocity", radio_field, 1);
    ImGui::SameLine();
    ImGui::RadioButton("Pressure", radio_field, 2);
    ImGui::SameLine();
    ImGui::RadioButton("Vorticity##1", radio_field, 3);

    ImGui::Separator();
    ImGui::Text("Force: (%.0f,%.0f) | Pos0: (%.0f,%.0f) | Pos1: (%.0f,%.0f)", impulse->Delta.x, impulse->Delta.y, impulse->LastPos.x, impulse->LastPos.y, impulse->CurrentPos.x, impulse->CurrentPos.y);

    ImGui::Separator();
    ImGui::Text("Frame Rate: %.3f ms (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    float ratio = (float)velocity->Width() / velocity->Height();
    ImVec2 dims(150, 150 / ratio);

    ImGui::Begin("Ink");
    ImGui::Image((ImTextureID)(intptr_t)ink->TextureId(), dims, uv_min, uv_max, tint_col, border_col);
    ImGui::End();

    ImGui::Begin("Velocity");
    ImGui::Image((ImTextureID)(intptr_t)velocity->TextureId(), dims, uv_min, uv_max, tint_col, border_col);
    ImGui::End();

    ImGui::Begin("Pressure");
    ImGui::Image((ImTextureID)(intptr_t)pressure->TextureId(), dims, uv_min, uv_max, tint_col, border_col);
    ImGui::End();

    ImGui::Begin("Vorticity##2");
    ImGui::Image((ImTextureID)(intptr_t)vorticity->TextureId(), dims, uv_min, uv_max, tint_col, border_col);
    ImGui::End();

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}




///////////////////////////
///     ImpulseState     ///
///////////////////////////

ImpulseState::ImpulseState()
    : Active(false)
    , LastPos()
    , CurrentPos()
    , Delta()
{
}

void ImpulseState::Update(float x, float y, bool currButtonDown)
{
    if (!Active && currButtonDown)
    {
        CurrentPos.x = x;
        CurrentPos.y = y;
        Active = true;
    }
    else if (Active && currButtonDown)
    {
        auto temp = CurrentPos;
        CurrentPos.x = x;
        CurrentPos.y = y;
        LastPos = temp;
    }
    else if (Active && !currButtonDown)
    {
        LastPos.x = LastPos.y = 0.f;
        Reset();
    }

    Delta = CurrentPos - LastPos;
}

void ImpulseState::Reset()
{
    static vec2 zero(0, 0);
    Delta = zero;
    CurrentPos = zero;
    LastPos = zero;
    Active = false;
}

///////////////////////////
///    VarTextBoxes     ///
///////////////////////////

VarTextBoxes::VarTextBoxes()
{
    memset(GridScale, 0, TEXTBUFF_LEN);
    memset(Viscosity, 0, TEXTBUFF_LEN);
    memset(InkViscosity, 0, TEXTBUFF_LEN);
    memset(SplatRadius, 0, TEXTBUFF_LEN);
    memset(AdvDissipation, 0, TEXTBUFF_LEN);
    memset(InkAdvDissipation, 0, TEXTBUFF_LEN);
    memset(Vorticity, 0, TEXTBUFF_LEN);
}

void FormatFloatText(char cstr[TEXTBOX_LEN], float value)
{
    static regex pattern("^\\d*\\.\\d+?(0*)$", regex_constants::optimize | regex_constants::ECMAScript);

    sprintf_s((char*)cstr, TEXTBOX_LEN, "%f", value);
    string s(cstr);
    smatch match;
    if (regex_match(s, match, pattern))
    {
        auto group = match[1].str();
        if (group.length() > 0)
        {
            string trimmed = s.substr(0, s.length() - group.length());
            sprintf_s((char*)cstr, TEXTBOX_LEN, "%s", trimmed.c_str());
        }
    }
}

void VarTextBoxes::SetValues(float gridscale, float viscosity, float ink_viscosity, float vorticity, float splat_radius, float adv_dissipation, float ink_adv_dissipation, float ink_volume)
{
    FormatFloatText(GridScale, gridscale);
    FormatFloatText(Viscosity, viscosity);
    FormatFloatText(InkViscosity, ink_viscosity);
    FormatFloatText(Vorticity, vorticity);
    FormatFloatText(SplatRadius, splat_radius);
    FormatFloatText(AdvDissipation, adv_dissipation);
    FormatFloatText(InkAdvDissipation, ink_adv_dissipation);
    FormatFloatText(InkVolume, ink_volume);
}

void VarTextBoxes::UpdateVars(SimulationVars& vars)
{
    //static regex re_colour("rgb\\((\\d+),(\\d+),(\\d+)\\)", regex_constants::icase | regex_constants::optimize | regex_constants::ECMAScript);
    //static regex re_colour_hex("#?([a-f0-9]{2})([a-f0-9]{2})([a-f0-9]{2})", regex_constants::icase | regex_constants::optimize | regex_constants::ECMAScript);

    //smatch match;
    //string str(InkColour);
    //if (regex_match(str, match, re_colour))
    //{
    //    auto r = stof(match[1].str());
    //    auto g = stof(match[2].str());
    //    auto b = stof(match[3].str());

    //    vars.InkColour = vec4(r, g, b, 1);
    //}
    //else if (regex_match(str, match, re_colour_hex))
    //{
    //    float r = stoi(match[1].str(), nullptr, 16);
    //    float g = stoi(match[2].str(), nullptr, 16);
    //    float b = stoi(match[3].str(), nullptr, 16);

    //    vars.InkColour = vec4(r, g, b, 1) / 255.0f;
    //}

    vars.Viscosity = stof(Viscosity);
    vars.InkViscosity = stof(InkViscosity);
    vars.Vorticity = stof(Vorticity);
    vars.AdvectionDissipation = stof(AdvDissipation);
    vars.InkAdvectionDissipation = stof(InkAdvDissipation);
    vars.SplatRadius = stof(SplatRadius);
    vars.InkVolume = stof(InkVolume);
    vars.GridScale = stof(GridScale);
}

///////////////////////////
///   SimulationVars    ///
///////////////////////////

SimulationVars::SimulationVars()
    : GridScale(0.3)
    , Viscosity(0.001)
    , InkViscosity(0.00001)
    , Vorticity(0.01)
    , SplatRadius(0.005)
    , AdvectionDissipation(0.97)
    , InkAdvectionDissipation(0.97)
    , InkVolume(0.007)
    , SelfAdvect(true)
    , AdvectInk(true)
    , DiffuseVelocity(true)
    , DiffuseInk(true)
    , PressureEnabled(true)
    , AddVorticity(true)
    , ExternalForces(true)
    , BoundariesEnabled(true)
    , DropletsMode(false)
    , InkColour(0.54, 0.2, 0.78, 1.0)
    , DisplayField(SimulationField::Ink)
{
}
