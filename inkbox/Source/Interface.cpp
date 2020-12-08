
#include "Interface.h"

#include <string>
#include <iostream>
#include <cstdio>
#include <regex>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Util.h"

#define UI_WIN_W 745
#define UI_WIN_H 525

using namespace std;

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
    //    s_Instance->rdv = glm::vec2(1.0f / width, 1.0f / height);
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

    Main = glfwCreateWindow(width, height, " i n k . b o x ", nullptr, nullptr);
    if (Main == nullptr)
    {
        cout << "Failed to create GLFW window" << endl;
        return false;
    }

    glfwWindowHint(GLFW_RESIZABLE, 0);
    Settings = glfwCreateWindow(UI_WIN_W, UI_WIN_H, " i n k . b o x ", nullptr, Main);
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

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    ImGui_ImplGlfw_InitForOpenGL(Settings, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    ViewportSize = glm::vec2(width, height);
    return true;
}


///////////////////////////
///     CursorState     ///
///////////////////////////

CursorState::CursorState()
    : ButtonDown(false)
    , LastPos()
    , CurrentPos()
    , Diff()
    , ButtonReleased(false)
    , Mode(SplatMode::Continuous)
{
}

void CursorState::Update(float x, float y, bool currButtonDown)
{
    if (Mode == SplatMode::Continuous)
    {
        if (!ButtonDown && currButtonDown)
        {
            CurrentPos.x = x;
            CurrentPos.y = y;
            ButtonDown = true;
        }
        else if (ButtonDown && currButtonDown)
        {
            auto temp = CurrentPos;
            CurrentPos.x = x;
            CurrentPos.y = y;
            LastPos = temp;
        }
        else if (ButtonDown && !currButtonDown)
        {
            LastPos.x = LastPos.y = 0.f;
            Reset();
        }
    }
    else
    {
        if (!ButtonDown && currButtonDown)
        {
            LastPos.x = x;
            LastPos.y = y;
            ButtonDown = true;
            ButtonReleased = false;
        }
        else if (ButtonDown && !currButtonDown)
        {
            CurrentPos.x = x;
            CurrentPos.y = y;
            ButtonDown = false;
            ButtonReleased = true;
        }
    }

    Diff = CurrentPos - LastPos;
}

bool CursorState::IsActive() const
{
    if (Mode == SplatMode::Continuous && LastPos.x != 0)
        return ButtonDown;
    else if (Mode == SplatMode::ClickAndRelease)
        return ButtonReleased;

    return false;
}

void CursorState::Reset()
{
    static glm::vec2 zero(0, 0);
    Diff = zero;
    CurrentPos = zero;
    LastPos = zero;
    ButtonReleased = false;
    ButtonDown = false;
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
    , DisplayField(SimulationField::Velocity)
{
}
