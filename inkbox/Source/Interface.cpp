
#include "Interface.h"

#include <string>
#include <iostream>
#include <cstdio>
#include <regex>
#include <thread>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "../resource.h"
#include "Common.h"

using namespace std;
using namespace glm;

void GLErrorCallback(int error_code, const char* description)
{
    LOG_ERROR("GLFW error %08X: %s", error_code, description);
}

InkBoxWindows::InkBoxWindows()
    : Main(nullptr)
    , Controls(nullptr)
    , ViewportSize(0,0)
{}

InkBoxWindows::~InkBoxWindows()
{
    if (Main)
        glfwDestroyWindow(Main);

    if (Controls)
        glfwDestroyWindow(Controls);
}

bool InkBoxWindows::InitGLContexts(int width, int height, int ctrl_width, int ctrl_height)
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (glfwInit() != GLFW_TRUE)
    {
        cout << "Failed to initialize GL" << endl;
        return false;
    }

    Main = glfwCreateWindow(width, height, MAIN_WINDOW_TITLE, nullptr, nullptr);
    if (Main == nullptr)
    {
        cout << "Failed to create GLFW window" << endl;
        return false;
    }

    bool create_ctrl_pnl = ctrl_width > 0 && ctrl_height > 0;
    if (create_ctrl_pnl)
    {
        glfwWindowHint(GLFW_RESIZABLE, 0);
        Controls = glfwCreateWindow(ctrl_width, ctrl_height, CONTROLS_WINDLW_TITLE, nullptr, Main);
        if (Controls == nullptr)
        {
            cout << "Failed to create GLFW window" << endl;
            return false;
        }
    }

    glfwMakeContextCurrent(Main);
    glfwSetErrorCallback(GLErrorCallback);

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
    BringWindowToTop(hwnd);

    hwnd = GetConsoleWindow();
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hico);
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hico);

    if (create_ctrl_pnl)
    {
        hwnd = glfwGetWin32Window(Controls);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hico);
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hico);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(Controls, true);
        ImGui_ImplOpenGL3_Init("#version 330 core");
    }

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
    , is3D(false)
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
    , is3D(false)
{
    glfwSetWindowUserPointer(win, this);
}

ControlPanel::ControlPanel(GLFWwindow* win, SimulationVars* vars, VarTextBoxes* texts, ImpulseState* impulse)
    : window(win)
    , simvars(vars)
    , texts(texts)
    , impulse(impulse)
    , is3D(true)
{
}

void ControlPanel::Render(bool& update_vars, bool& clear_buffers)
{
#define TEXTBOX(text,var) ImGui::SetNextItemWidth(80); ImGui::InputText((text), (var), 16);

    static ImVec2 uv_min = ImVec2(0.0f, 1.0f);                 // Top-left
    static ImVec2 uv_max = ImVec2(1.0f, 0.0f);                 // Lower-right
    static ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
    static ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    update_vars = false;
    clear_buffers = false;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Settings");

    ImGui::Text("Equation Steps");
    ImGui::Checkbox("Self Advection", &simvars->SelfAdvect);
    ImGui::SameLine(200);
    ImGui::Checkbox("Ink Advection", &simvars->AdvectInk);

    ImGui::Checkbox("Diffusion", &simvars->DiffuseVelocity);
    ImGui::SameLine(200);
    ImGui::Checkbox("Ink Diffusion", &simvars->DiffuseInk);

    if (!is3D)
    {
        ImGui::Checkbox("Vorticity", &simvars->AddVorticity);
        ImGui::SameLine(200);
    }

    ImGui::Checkbox("Boundary Conditions", &simvars->BoundariesEnabled);

    ImGui::Separator();
    ImGui::Text("Variables");
    TEXTBOX("Grid Scale", texts->GridScale);
    ImGui::SameLine(200);

    if (!is3D)
    {
        TEXTBOX("Vorticity##3", texts->Vorticity);
    }
    else
    {
        TEXTBOX("Gravity##3", texts->Gravity);
    }

    TEXTBOX("Viscosity", texts->Viscosity);
    ImGui::SameLine(200);
    TEXTBOX("Ink Viscosity", texts->InkViscosity);

    TEXTBOX("Dissipation", texts->AdvDissipation);
    ImGui::SameLine(200);
    TEXTBOX("Ink Dissipation", texts->InkAdvDissipation);
    
    TEXTBOX("Force Radius", texts->SplatRadius);
    ImGui::SameLine(200);
    TEXTBOX("Ink Volume", texts->InkVolume);
    
    if (is3D)
    {
        TEXTBOX("Force Multiplier", texts->ForceMultiplier);
    }

    // ImVec4 and glm::vec4 have the same layout
    ImGui::ColorEdit4("Ink Colour", (float*)&simvars->InkColour, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    ImGui::SameLine();
    ImGui::Text("Ink Colour");

    ImGui::SameLine(200);
    ImGui::Checkbox("Droplets", &simvars->DropletsMode);
    if (ImGui::Button("Update"))
        update_vars = true; 

    ImGui::SameLine();
    if (ImGui::Button("Clear"))
        clear_buffers = true;

    if (!is3D)
    {
        ImGui::Separator();
        ImGui::Text("Viewport");
        int* radio_field = (int*)(&simvars->DisplayField);
        ImGui::RadioButton("Ink", radio_field, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Velocity", radio_field, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Pressure", radio_field, 2);
        ImGui::SameLine();
        ImGui::RadioButton("Vorticity##1", radio_field, 3);
    }

    ImGui::Separator();
    if (!is3D)
        ImGui::Text("Force: (%.0f,%.0f) | Pos0: (%.0f,%.0f) | Pos1: (%.0f,%.0f)", impulse->Delta.x, impulse->Delta.y, impulse->LastPos.x, impulse->LastPos.y, impulse->CurrentPos.x, impulse->CurrentPos.y);
    else
        ImGui::Text("Force: (%.2f,%.2f,%.2f) | Position: (%.0f,%.0f,%.0f)", impulse->Delta.x, impulse->Delta.y, impulse->Delta.z, impulse->CurrentPos.x, impulse->CurrentPos.y, impulse->CurrentPos.z);

    ImGui::Separator();
    ImGui::Text("Frame Rate: %.3f ms (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    if (!is3D)
    {
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
    }

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
    : ForceActive(false)
    , InkActive(false)
    , Radial(false)
    , LastPos()
    , CurrentPos()
    , Delta()
{
}

void ImpulseState::Update(float x, float y, bool left_down, bool right_down)
{
    bool down = left_down || right_down;

    if (!IsActive() && down)
    {
        CurrentPos.x = x;
        CurrentPos.y = y;
        ForceActive = true;
        InkActive = left_down && !right_down;
    }
    else if (IsActive() && down)
    {
        auto temp = CurrentPos;
        CurrentPos.x = x;
        CurrentPos.y = y;
        LastPos = temp;
    }
    else if (IsActive() && !down)
    {
        LastPos.x = LastPos.y = 0.f;
        Reset();
    }

    Delta = CurrentPos - LastPos;
}

void ImpulseState::Reset()
{
    static vec3 zero(0, 0, 0);
    Delta = zero;
    CurrentPos = zero;
    LastPos = zero;
    ForceActive = false;
    InkActive = false;
    Radial = false;
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
    memset(Gravity, 0, TEXTBUFF_LEN);
}

void FormatFloatText(char cstr[TEXTBOX_LEN], float value)
{
    // Trim trailing 0s from a float

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

void VarTextBoxes::SetValues(SimulationVars& vars)
{
    FormatFloatText(GridScale, vars.GridScale);
    FormatFloatText(Viscosity, vars.Viscosity);
    FormatFloatText(InkViscosity, vars.InkViscosity);
    FormatFloatText(Vorticity, vars.Vorticity);
    FormatFloatText(SplatRadius, vars.SplatRadius);
    FormatFloatText(AdvDissipation, vars.AdvectionDissipation);
    FormatFloatText(InkAdvDissipation, vars.InkAdvectionDissipation);
    FormatFloatText(InkVolume, vars.InkVolume);
    FormatFloatText(Gravity, vars.Gravity);
    FormatFloatText(ForceMultiplier, vars.ForceMultiplier);
}

void VarTextBoxes::SetValues(float gridscale, float viscosity, float ink_viscosity, float vorticity, float splat_radius, float adv_dissipation, float ink_adv_dissipation, float ink_volume, float gravity, float fmult)
{
    FormatFloatText(GridScale, gridscale);
    FormatFloatText(Viscosity, viscosity);
    FormatFloatText(InkViscosity, ink_viscosity);
    FormatFloatText(Vorticity, vorticity);
    FormatFloatText(SplatRadius, splat_radius);
    FormatFloatText(AdvDissipation, adv_dissipation);
    FormatFloatText(InkAdvDissipation, ink_adv_dissipation);
    FormatFloatText(InkVolume, ink_volume);
    FormatFloatText(Gravity, gravity);
    FormatFloatText(ForceMultiplier, fmult);
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
    vars.Gravity = stof(Gravity);
    vars.ForceMultiplier = stof(ForceMultiplier);
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
    , AdvectionDissipation(0.99)
    , InkAdvectionDissipation(0.98)
    , InkVolume(0.007)
    , Gravity(0)
    , ForceMultiplier(1.0)
    , SelfAdvect(true)
    , AdvectInk(true)
    , DiffuseVelocity(true)
    , DiffuseInk(true)
    , PressureEnabled(true)
    , AddVorticity(true)
    , BoundariesEnabled(true)
    , DropletsMode(false)
    , InkColour(0.54, 0.2, 0.78, 1.0)
    , DisplayField(SimulationField::Ink)
{
}

///////////////////////////
///     FPSLimiter      ///
///////////////////////////

FPSLimiter::FPSLimiter(int fps)
    : simFrameTime(1000.0f / fps)
    , adjustmentCtr(FRAME_DELAY_ADJUSTMENT_MOD)
    , avgFrameTime(0)
    , fpsDelay(int(simFrameTime * 0.6))
{
}

void FPSLimiter::Regulate()
{
    using namespace std::chrono;

    if (fpsDelay != 0)
        this_thread::sleep_for(milliseconds(fpsDelay));

    if (--adjustmentCtr == 0)
    {
        auto now = high_resolution_clock::now();

        if (lastTime.time_since_epoch().count() != 0)
        {
            duration<float, milli> time_taken = now - lastTime;
            avgFrameTime = time_taken.count() / FRAME_DELAY_ADJUSTMENT_MOD;

            float diff = simFrameTime - avgFrameTime;
            if (diff >= 1.0f)
                fpsDelay++;
            else if (diff <= -1.0f)
                fpsDelay = fpsDelay > 0 ? fpsDelay - 1 : 0;
        }

        lastTime = now;
        adjustmentCtr = FRAME_DELAY_ADJUSTMENT_MOD;
    }
}
