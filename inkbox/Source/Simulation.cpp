#include "Simulation.h"

#include <iostream>
#include <thread>

#include <windows.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>

#include "Shader.h"
#include "Util.h"

using namespace std;
using namespace glm;

InkBoxSimulation* InkBoxSimulation::s_Instance = nullptr;

SimulationFields::SimulationFields(int width, int height)
    : Velocity(width, height)
    , Pressure(width, height)
    , Vorticity(width, height)
    , Ink(width, height)
    , VelocityVis(width, height)
    , PressureVis(width, height)
    , InkVis(width, height)
    , VorticityVis(width, height)
    , Temp(width, height)
{
}

InkBoxSimulation::InkBoxSimulation(const InkBoxWindows& app, int width, int height)
    : width(width)
    , height(height)
    , fbos(width, height)
    , mainWindow(app.Main)
    , uiWindow(app.Settings)
    , rdv(1.0f / width, 1.0f / height)
    , advection(width, height, 1.f/width)
    , poissonSolver(width, height, 1.f/width)
    , gradient(width, height, 1.f/width)
    , divergence(width, height, 1.f/width)
    , impulse(width, height, 1.f/width)
    , vorticity(width, height, 1.f/width)
    , boundaries(width, height, 1.f/width)
    , timestep(0)
{
    if (s_Instance == nullptr)
        s_Instance = this;

    ui.SetValues(vars.GridScale, vars.Viscosity, vars.InkViscosity, vars.Vorticity, vars.SplatRadius, vars.AdvectionDissipation, vars.InkAdvectionDissipation, vars.InkVolume);
    controlPanel = ControlPanel(app.Settings, &vars, &ui, &impulseState, &fbos.VelocityVis, &fbos.PressureVis, &fbos.InkVis, &fbos.VorticityVis);
}

void InkBoxSimulation::Terminate()
{
    glfwTerminate();
}

bool InkBoxSimulation::CreateScene()
{
    vec2 c(1.f-0.5f/width, 1.f-0.5f/height);
    float outer_vertices[] =
    {
         c.x, -c.y, 0.0f,   // top right
         c.x,  c.y, 0.0f,   // bottom right
        -c.x,  c.y, 0.0f,   // bottom left
        -c.x, -c.y, 0.0f,   // top left 
    };

    int top[] = { 3,0 };
    borderT.Init(&outer_vertices[0], 12, top, 2);

    int bottom[] = { 1,2 };
    borderB.Init(&outer_vertices[0], 12, bottom, 2);

    int left[] = { 2,3 };
    borderL.Init(&outer_vertices[0], 12, left, 2);

    int right[] = { 0,1 };
    borderR.Init(&outer_vertices[0], 12, right, 2);


    c = vec2(1.f-1.5f/width, 1.f-1.5f/height);
    float inner_vertices[] =
    {
         c.x, -c.y, 0.0f,   // top right
         c.x,  c.y, 0.0f,   // bottom right
        -c.x,  c.y, 0.0f,   // bottom left
        -c.x, -c.y, 0.0f,   // top left 
    };

    int quad_indices[] =
    {
        0, 1, 3,
        1, 2, 3
    };

    quad.Init(&inner_vertices[0], 12, &quad_indices[0], 6);

    // Create and compiler shaders
    if (!CreateShaderOps())
    {
        LOG_ERROR("Shader(s) could not be intialized");
        return false;
    }

    return true;
}

void InkBoxSimulation::DrawQuad()
{
    _GL_WRAP1(glBindVertexArray, quad.VAO);
    _GL_WRAP4(glDrawElements, GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

void InkBoxSimulation::WindowLoop()
{
    double last_time = 0.f;

    while (!glfwWindowShouldClose(mainWindow))
    {
        glfwMakeContextCurrent(mainWindow);
        double now = glfwGetTime();
        timestep = last_time == 0 ? 0.016667 : now - last_time;
        last_time = now;

        glfwPollEvents();
        ProcessInput();

        if (vars.DropletsMode)
            TickDropletsMode();

        // Update velocity, pressure, and ink fields
        ComputeFields(timestep);

        // Create visualizations for each one

        fbos.VelocityVis.Bind();
        vectorVisShader.Use();
        vectorVisShader.SetVec4("bias", vec4(0.5, 0.5, 0.5, 0.5));
        vectorVisShader.SetVec4("scale", vec4(0.5, 0.5, 0.5, 0.5));
        vectorVisShader.SetTexture("field", fbos.Velocity, 0);
        DrawQuad();

        fbos.InkVis.Bind();
        inkVisShader.Use();
        vec4 bias(0, 0, vars.InkColour.z, 0);
        inkVisShader.SetVec4("bias", bias);
        inkVisShader.SetVec4("scale", vec4(1, 1, 1, 1));
        inkVisShader.SetTexture("field", fbos.Ink, 0);
        DrawQuad();

        fbos.PressureVis.Bind();
        scalarVisShader.Use();
        scalarVisShader.SetVec4("bias", vec4(0, 0, 0, 0));
        scalarVisShader.SetVec4("scale", vec4(2, -1, -2, 1));
        scalarVisShader.SetTexture("field", fbos.Pressure, 0);
        fbos.Pressure.BindTexture(0);
        DrawQuad();

        fbos.VorticityVis.Bind();
        scalarVisShader.SetVec4("scale", vec4(1, 1, -1, -1));
        scalarVisShader.SetTexture("field", fbos.Vorticity, 0);
        DrawQuad();

        _GL_WRAP2(glBindFramebuffer, GL_FRAMEBUFFER, 0);
        copyShader.Use();
        copyShader.SetInt("field", 0);

        if (vars.DisplayField == SimulationField::Velocity)
            fbos.VelocityVis.BindTexture(0);
        else if (vars.DisplayField == SimulationField::Ink)
            fbos.InkVis.BindTexture(0);
        else if (vars.DisplayField == SimulationField::Vorticity)
            fbos.VorticityVis.BindTexture(0);
        else
            fbos.PressureVis.BindTexture(0);

        DrawQuad();
        glfwSwapBuffers(mainWindow);

        // Render ImGUI
        glfwMakeContextCurrent(controlPanel.WindowPtr());
        controlPanel.Render();

        glfwSwapBuffers(uiWindow);
    }
}

void InkBoxSimulation::ProcessInput()
{
    if (glfwGetKey(mainWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(mainWindow, true);
    }

    double x = 0, y = 0;
    glfwGetCursorPos(mainWindow, &x, &y);
    impulseState.Update(x, height - y, 
        glfwGetMouseButton(mainWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS, 
        glfwGetMouseButton(mainWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
}

void InkBoxSimulation::SetDimensions(int w, int h)
{
    width = w;
    height = h;
}

bool _AddFragShader(GLShaderProgram& program, GLShader& vShader, GLShader& fShader)
{
    if (!fShader.Compile())
        return false;

    program.Init();
    program.Attach(vShader);
    program.Attach(fShader);

    if (!program.Link())
    {
        return false;
    }

    return true;
}

bool InkBoxSimulation::CreateShaderOps()
{
#define ADD_SHADER(obj,file) { GLShader fs(file, ShaderType::Fragment); if (!_AddFragShader(obj, vs, fs)) return false; obj.Use(); obj.SetVec2("stride", rdv); }

    GLShader vs("tex_coords.v.glsl", ShaderType::Vertex);
    if (!vs.Compile())
        return false;
    

    ADD_SHADER(impulseShader,       "add_impulse.f.glsl")
    ADD_SHADER(advectionShader,     "advection.f.glsl")
    ADD_SHADER(jacobiShader,        "jacobi.f.glsl")
    ADD_SHADER(divShader,           "divergence.f.glsl")
    ADD_SHADER(gradShader,          "gradient.f.glsl")
    ADD_SHADER(subtractShader,      "subtract.f.glsl")
    ADD_SHADER(boundaryShader,      "boundary.f.glsl")
    ADD_SHADER(vorticityShader,     "vorticity.f.glsl")
    ADD_SHADER(addVorticityShader,  "add_vorticity.f.glsl")
    ADD_SHADER(vectorVisShader,     "vector_vis.f.glsl")
    ADD_SHADER(scalarVisShader,     "scalar_vis.f.glsl")
    ADD_SHADER(inkVisShader,        "ink_vis.f.glsl")
    ADD_SHADER(copyShader,          "copy.f.glsl")

    vec2 dimensions(width, height);

    impulse.SetShader(&impulseShader);
    impulse.SetQuad(&quad);
    impulse.SetUniformsFunc([&](GLShaderProgram& sh) -> void {
        sh.SetFloat("delta_t", timestep);
    });

    advection.SetShader(&advectionShader);
    advection.SetQuad(&quad);
    advection.SetUniformsFunc([&](GLShaderProgram& sh) -> void {
        sh.SetFloat("gs", vars.GridScale);
        sh.SetVec2("rdv", rdv);
        sh.SetFloat("delta_t", timestep);
        sh.SetTexture("velocity", fbos.Velocity, 0);
    });

    vorticity.SetShader(&vorticityShader);
    vorticity.SetQuad(&quad);
    vorticity.SetOutput(&fbos.Vorticity);
    vorticity.SetUniformsFunc([&](GLShaderProgram& sh) -> void {
        sh.SetFloat("gs", vars.GridScale);
        sh.SetVec2("rdv", rdv);
        sh.SetTexture("velocity", fbos.Velocity, 0);
    });

    addVorticity.SetShader(&addVorticityShader);
    addVorticity.SetQuad(&quad);
    addVorticity.SetUniformsFunc([&](GLShaderProgram& sh) -> void {
        sh.SetFloat("gs", vars.GridScale);
        sh.SetVec2("rdv", rdv);
        sh.SetTexture("velocity", fbos.Velocity, 0);
        sh.SetTexture("vorticity", fbos.Vorticity, 1);
        sh.SetFloat("delta_t", 1);
        sh.SetFloat("scale", vars.Vorticity);
    });

    poissonSolver.SetShader(&jacobiShader);
    poissonSolver.SetQuad(&quad);
    poissonSolver.SetUniformsFunc([&](GLShaderProgram& sh) -> void {
        vec2 rdv2 = -rdv * rdv;
        sh.SetVec2("rdv", rdv);
        //sh.SetVec2("alpha", rdv2);
        //sh.SetVec2("beta", vec2(4.0, 4.0));
    });

    gradient.SetShader(&gradShader);
    gradient.SetQuad(&quad);
    gradient.SetUniformsFunc([&](GLShaderProgram& sh) -> void {
        sh.SetFloat("gs", vars.GridScale);
        sh.SetVec2("rdv", rdv);
        sh.SetTexture("field", fbos.Pressure, 0);
    });

    divergence.SetShader(&divShader);
    divergence.SetQuad(&quad);
    divergence.SetUniformsFunc([&](GLShaderProgram& sh) -> void {
        sh.SetFloat("gs", vars.GridScale);
        sh.SetVec2("rdv", rdv);
        sh.SetTexture("field", fbos.Velocity, 0);
    });

    // Calculate U = W - grad(P) where div(U)=0
    subtract.SetShader(&subtractShader);
    subtract.SetQuad(&quad);
    subtract.SetUniformsFunc([&](GLShaderProgram& sh) -> void {
        sh.SetTexture("a", fbos.Velocity, 0);
        sh.SetTexture("b", fbos.Pressure.Back(), 1);
    });

    boundaries.SetShader(&boundaryShader);
    boundaries.SetLines(&borderT, &borderB, &borderL, &borderR);

    return true;

#undef ADD_SHADER
}

void InkBoxSimulation::ComputeFields(float delta_t)
{
    if (!vars.SelfAdvect && !vars.AdvectInk && !vars.DiffuseVelocity && !vars.AddVorticity && !vars.ExternalForces)
        return;

    // convention: front buffer has the correct field data after each operation is finished

    /***************************/
    /****** SELF ADVECTION *****/
    /***************************/
    if (vars.SelfAdvect)
    {
        ComputeBoundaryValues(fbos.Velocity, -1);

        advection.Use();
        advection.SetOutput(&fbos.Velocity.Back());
        advection.Shader().SetFloat("dissipation", vars.InkAdvectionDissipation);
        advection.Shader().SetTexture("quantity", fbos.Velocity, 1);
        advection.Compute();
        fbos.Velocity.Swap();
    }

    /***************************/
    /****** INK ADVECTION ******/
    /***************************/
    if (vars.AdvectInk)
    {
        ComputeBoundaryValues(fbos.Ink, 0);

        advection.Use();
        advection.SetOutput(&fbos.Ink.Back());
        advection.Shader().SetFloat("dissipation", vars.InkAdvectionDissipation);
        advection.Shader().SetTexture("quantity", fbos.Ink, 1);
        advection.Compute();
        fbos.Ink.Swap();
    }

    /**********************************/
    /******* FORCE APPLICATION ********/
    /**********************************/
    if (vars.ExternalForces && impulseState.IsActive())
    {
        const float MAX_RADIUS = 1.0f;
        auto diff = impulseState.Delta;

        // clamp to some range
        vec3 force(min(max(diff.x, -vars.GridScale), vars.GridScale),
                    min(max(diff.y, -vars.GridScale), vars.GridScale),
                    0);

        impulse.Use();
        impulse.SetOutput(&fbos.Velocity.Back());
        impulse.Shader().SetVec2("position", impulseState.CurrentPos * rdv);
        impulse.Shader().SetVec3("force", force);
        impulse.Shader().SetFloat("radius", vars.SplatRadius);
        impulse.Shader().SetTexture("velocity", fbos.Velocity, 0);
        impulse.Compute();
        fbos.Velocity.Swap();

        if (impulseState.InkActive)
        {
            force = vec3(vars.InkColour.x, vars.InkColour.y, vars.InkColour.z);

            impulse.Use();
            impulse.SetOutput(&fbos.Ink.Back());
            impulse.Shader().SetVec2("position", impulseState.CurrentPos * rdv);
            impulse.Shader().SetVec3("force", force);
            impulse.Shader().SetFloat("radius", vars.InkVolume);
            impulse.Shader().SetTexture("velocity", fbos.Ink, 0);
            impulse.Compute();
            fbos.Ink.Swap();
        }
    }

    /***************************/
    /******** VORTICITY ********/
    /***************************/
    if (vars.AddVorticity)
    {
        vorticity.Compute();

        ComputeBoundaryValues(fbos.Velocity, -1);

        addVorticity.SetOutput(&fbos.Velocity.Back());
        addVorticity.Compute();
        fbos.Velocity.Swap();
    }
    

    /***************************/
    /******** DIFFUSION ********/
    /***************************/
    if (vars.DiffuseVelocity)
    {
        float alpha = (vars.GridScale * vars.GridScale) / (vars.Viscosity * delta_t);
        float beta = alpha + 4.0f;
        SolvePoissonSystem(fbos.Velocity, alpha, beta);
    }

    if (vars.DiffuseInk)
    {
        float alpha = (vars.GridScale * vars.GridScale) / (vars.InkViscosity * delta_t);
        float beta = alpha + 4.0;
        SolvePoissonSystem(fbos.Ink, alpha, beta);
    }

    /***************************/
    /******** PROJECTION *******/
    /***************************/

    // Calculate div(W)
    divergence.SetOutput(&fbos.Velocity.Back());
    divergence.Compute();

    // Solve for P in: Laplacian(P) = div(W)
    SolvePoissonSystem(fbos.Pressure, fbos.Velocity.Back(), -vars.GridScale * vars.GridScale, 4.0f);

    // Calculate grad(P)
    gradient.SetOutput(&fbos.Pressure.Back());
    gradient.Compute();
    // No swap, back buffer has the gradient

    // Calculate U = W - grad(P) where div(U)=0
    subtract.SetOutput(&fbos.Velocity.Back());
    subtract.Compute();
    fbos.Velocity.Swap();
    
    ComputeBoundaryValues(fbos.Velocity, -1);
}

void InkBoxSimulation::ComputeBoundaryValues(SwapFBO& swap, float scale)
{
    if (!vars.BoundariesEnabled)
        return;

    CopyFBO(swap.Back(), swap.Front());
    boundaries.Use();
    boundaries.SetOutput(&swap.Back());
    boundaries.Shader().SetTexture("field", swap.Front(), 0);
    boundaries.Shader().SetFloat("scale", scale);
    boundaries.Compute();
    swap.Swap();
}

void InkBoxSimulation::SolvePoissonSystem(SwapFBO& swap, FBO& initial_value, float alpha, float beta)
{
    CopyFBO(fbos.Temp, initial_value);
    poissonSolver.Use();
    poissonSolver.Shader().SetVec2("rdv", rdv);
    poissonSolver.Shader().SetFloat("alpha", alpha);
    poissonSolver.Shader().SetFloat("beta", beta);
    poissonSolver.Shader().SetTexture("b", fbos.Temp, 1);

    for (int i = 0; i < (NUM_JACOBI_ROUNDS & (~0x1)); i++)
    {
        swap.Back().Bind();
        poissonSolver.Shader().SetTexture("x", swap.Front(), 0);
        poissonSolver.Draw();
        swap.Swap();
    }
}

void InkBoxSimulation::SolvePoissonSystem(SwapFBO& swap, float alpha, float beta)
{
    SolvePoissonSystem(swap, swap.Front(), alpha, beta);
}

vec2 InkBoxSimulation::RandPos()
{
    int x = rand() % width;
    int y = rand() % height;
    return vec2(x, y);
}

void InkBoxSimulation::TickDropletsMode()
{
    static float acc = 0;
    static float next_drop = 0;
    static ivec2 freq_range(200, 1500);

    acc += (timestep * 1000);
    if (acc >= next_drop)
    {
        acc = 0;
        next_drop = (rand() % (freq_range.y - freq_range.x)) + freq_range.x;

        impulseState.LastPos = RandPos();
        impulseState.CurrentPos = RandPos();
        impulseState.Delta = impulseState.CurrentPos - impulseState.LastPos;
        impulseState.ForceActive = true;
        impulseState.InkActive = true;
    }
}

void InkBoxSimulation::CopyFBO(FBO& dest, FBO& src)
{
    dest.Bind();
    copyShader.Use();
    copyShader.SetInt("field", 0);
    src.BindTexture(0);
    DrawQuad();
}
