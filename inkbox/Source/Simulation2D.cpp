#include "Simulation2D.h"

#include <iostream>
#include <thread>

#include <windows.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>

#include "Shader.h"
#include "Common.h"

using namespace std;
using namespace glm;

InkBox2DSimulation::InkBox2DSimulation(const InkBoxWindows& app, int width, int height)
    : width(width)
    , height(height)
    , fbos(width, height)
    , window(app.Main)
    , limiter(60)
    , rdv(1.0f / width, 1.0f / height)
    , advection(width, height, 1.f/width)
    , poissonSolver(width, height, 1.f/width)
    , gradient(width, height, 1.f/width)
    , divergence(width, height, 1.f/width)
    , impulse(width, height, 1.f/width)
    , vorticity(width, height, 1.f/width)
    , boundaries(width, height, 1.f/width)
    , timestep(0)
    , paused(false)
{
    ui.SetValues(vars);
    controlPanel = ControlPanel(app.Controls, &vars, &ui, &impulseState, &fbos.VelocityVis, &fbos.PressureVis, &fbos.InkVis, &fbos.VorticityVis);

    glfwSetWindowUserPointer(app.Main, this);
}

void InkBox2DSimulation::Terminate()
{
    glfwTerminate();
}

bool InkBox2DSimulation::CreateScene()
{
    vec2 c(1.f-0.5f/width, 1.f-0.5f/height);
    float outer_vertices[] =
    {
         c.x, -c.y, 0.0f,   // top right
         c.x,  c.y, 0.0f,   // bottom right
        -c.x,  c.y, 0.0f,   // bottom left
        -c.x, -c.y, 0.0f,   // top left 
    };

    unsigned int top[] = { 3, 0 };
    borderT.Init(&outer_vertices[0], 12, top, 2);

    unsigned int bottom[] = { 1, 2 };
    borderB.Init(&outer_vertices[0], 12, bottom, 2);

    unsigned int left[] = { 2, 3 };
    borderL.Init(&outer_vertices[0], 12, left, 2);

    unsigned int right[] = { 0, 1 };
    borderR.Init(&outer_vertices[0], 12, right, 2);


    c = vec2(1.f-1.5f/width, 1.f-1.5f/height);
    float inner_vertices[] =
    {
         c.x, -c.y, 0.0f,   // top right
         c.x,  c.y, 0.0f,   // bottom right
        -c.x,  c.y, 0.0f,   // bottom left
        -c.x, -c.y, 0.0f,   // top left 
    };

    unsigned int quad_indices[] =
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

void InkBox2DSimulation::DrawQuad()
{
    _GL_WRAP1(glBindVertexArray, quad.VAO);
    _GL_WRAP4(glDrawElements, GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

void InkBox2DSimulation::WindowLoop()
{
    double last_time = 0.f;
    bool curr_paused = false;
    SimulationField curr_view = vars.DisplayField;

    while (!glfwWindowShouldClose(window))
    {
        glfwMakeContextCurrent(window);
        double now = glfwGetTime();
        timestep = last_time == 0 ? 0.016667 : now - last_time;
        last_time = now;

        glfwPollEvents();
        ProcessInputs();

        if (!paused)
        {
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
            vectorVisShader.Use();
            vectorVisShader.SetVec4("bias", vec4(0, 0, 0, 0));
            vectorVisShader.SetVec4("scale", vec4(1, 1, 1, 1));
            vectorVisShader.SetTexture("field", fbos.Ink, 0);
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

            fbos.Get(vars.DisplayField).BindTexture(0);
            DrawQuad();
            glfwSwapBuffers(window);

            curr_paused = false;
        }
        else if (!curr_paused || curr_view != vars.DisplayField)
        {
            _GL_WRAP2(glBindFramebuffer, GL_FRAMEBUFFER, 0);
            copyShader.Use();
            copyShader.SetInt("field", 0);

            fbos.Get(vars.DisplayField).BindTexture(0);
            DrawQuad();
            glfwSwapBuffers(window);

            curr_paused = true;
            curr_view = vars.DisplayField;
        }

        // Sleep for a little bit if needed
        limiter.Regulate();

        // Render control panel window
        bool update, clear;
        glfwMakeContextCurrent(controlPanel.WindowPtr());
        controlPanel.Render(update, clear);

        if (update)
            ui.UpdateVars(vars);

        if (clear)
        {
            fbos.Velocity.Clear();
            fbos.Vorticity.Clear();
            fbos.Pressure.Clear();
            fbos.Ink.Clear();
        }

        glfwSwapBuffers(controlPanel.WindowPtr());
    }
}

void InkBox2DSimulation::SetDimensions(int w, int h)
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

bool InkBox2DSimulation::CreateShaderOps()
{
#define ADD_SHADER(obj,file) { GLShader fs(file, ShaderType::Fragment); if (!_AddFragShader(obj, vs, fs)) return false; obj.Use(); obj.SetVec2("stride", rdv); }

    GLShader vs("2d\\tex_coords.vert", ShaderType::Vertex);
    if (!vs.Compile())
        return false;

    ADD_SHADER(impulseShader,       "2d\\add_impulse.frag")
    ADD_SHADER(radialImpulseShader, "2d\\add_radial_impulse.frag")
    ADD_SHADER(advectionShader,     "2d\\advection.frag")
    ADD_SHADER(jacobiShader,        "2d\\jacobi.frag")
    ADD_SHADER(divShader,           "2d\\divergence.frag")
    ADD_SHADER(gradShader,          "2d\\gradient.frag")
    ADD_SHADER(subtractShader,      "2d\\subtract.frag")
    ADD_SHADER(boundaryShader,      "2d\\boundary.frag")
    ADD_SHADER(vorticityShader,     "2d\\vorticity.frag")
    ADD_SHADER(addVorticityShader,  "2d\\add_vorticity.frag")
    ADD_SHADER(vectorVisShader,     "2d\\vector_vis.frag")
    ADD_SHADER(scalarVisShader,     "2d\\scalar_vis.frag")
    ADD_SHADER(copyShader,          "2d\\copy.frag")

    impulse.SetShader(&impulseShader);
    impulse.SetQuad(&quad);
    impulse.SetUniformsFunc([&](GLShaderProgram& sh) -> void {
        sh.SetFloat("delta_t", timestep);
    });

    
    radialImpulse.SetShader(&radialImpulseShader);
    radialImpulse.SetQuad(&quad);
    radialImpulse.SetUniformsFunc([&](GLShaderProgram& sh) -> void {
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
        sh.SetTexture("velocity", fbos.Velocity, 0);
    });

    addVorticity.SetShader(&addVorticityShader);
    addVorticity.SetQuad(&quad);
    addVorticity.SetUniformsFunc([&](GLShaderProgram& sh) -> void {
        sh.SetFloat("gs", vars.GridScale);
        sh.SetTexture("velocity", fbos.Velocity, 0);
        sh.SetTexture("vorticity", fbos.Vorticity, 1);
        sh.SetFloat("delta_t", 1);
        sh.SetFloat("scale", vars.Vorticity);
    });

    poissonSolver.SetShader(&jacobiShader);
    poissonSolver.SetQuad(&quad);

    gradient.SetShader(&gradShader);
    gradient.SetQuad(&quad);
    gradient.SetUniformsFunc([&](GLShaderProgram& sh) -> void {
        sh.SetFloat("gs", vars.GridScale);
        sh.SetTexture("field", fbos.Pressure, 0);
    });

    divergence.SetShader(&divShader);
    divergence.SetQuad(&quad);
    divergence.SetUniformsFunc([&](GLShaderProgram& sh) -> void {
        sh.SetFloat("gs", vars.GridScale);
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

void InkBox2DSimulation::ProcessInputs()
{
    static int pkey = 0;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    {
        pkey = 1;
    }
    else if (pkey == 1 && glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE)
    {
        pkey = 0;
        paused = !paused;

        if (paused)
            glfwSetWindowTitle(window, MAIN_WINDOW_TITLE "(PAUSED)");
        else
            glfwSetWindowTitle(window, MAIN_WINDOW_TITLE);
    }

    double x = 0, y = 0;
    glfwGetCursorPos(window, &x, &y);
    impulseState.Update(x, height - y, glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS, glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);

    int w = 0, h = 0;
    int ww = 0, wh = 0;
    glfwGetFramebufferSize(window, &w, &h);
    glfwGetWindowSize(window, &ww, &wh);
    if (w != width || height != h)
    {
        // For now enforce a square viewport
        width = h;
        height = h;

        if (wh != ww)
            glfwSetWindowSize(window, wh, wh);

        rdv = vec2(1.0f / width, 1.0f / height);
        fbos.Resize(width, height, copyShader, quad);
        LOG_INFO("Resized to %dx%d", width, height);
    }
}

void InkBox2DSimulation::ComputeFields(float delta_t)
{
    if (!vars.SelfAdvect && !vars.AdvectInk && !vars.DiffuseVelocity && !vars.AddVorticity)
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
        advection.Shader().SetFloat("dissipation", vars.AdvectionDissipation);
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
    if (impulseState.IsActive())
    {
        const float MAX_RADIUS = 1.0f;
        auto diff = impulseState.Delta;

        // clamp to some range
        vec3 force(min(max(diff.x, -vars.GridScale), vars.GridScale),
                    min(max(diff.y, -vars.GridScale), vars.GridScale),
                    0);

        QuadShaderOp* op = &impulse;
        if (impulseState.Radial)
            op = &radialImpulse;

        op->Use();
        op->SetOutput(&fbos.Velocity.Back());
        op->Shader().SetVec2("position", vec2(impulseState.CurrentPos.x, impulseState.CurrentPos.y) * rdv);
        op->Shader().SetFloat("radius", vars.SplatRadius);
        op->Shader().SetTexture("velocity", fbos.Velocity, 0);

        if (!impulseState.Radial)
            op->Shader().SetVec3("force", force);

        op->Compute();
        fbos.Velocity.Swap();

        if (impulseState.InkActive)
        {
            vec3 colour(vars.InkColour.x, vars.InkColour.y, vars.InkColour.z);
            impulse.Use();
            impulse.SetOutput(&fbos.Ink.Back());
            impulse.Shader().SetVec2("position", vec2(impulseState.CurrentPos.x, impulseState.CurrentPos.y) * rdv);
            impulse.Shader().SetVec3("force", colour);
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

void InkBox2DSimulation::ComputeBoundaryValues(SwapFBO& swap, float scale)
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

void InkBox2DSimulation::SolvePoissonSystem(SwapFBO& swap, FBO& initial_value, float alpha, float beta)
{
    CopyFBO(fbos.Temp, initial_value);
    poissonSolver.Use();
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

void InkBox2DSimulation::SolvePoissonSystem(SwapFBO& swap, float alpha, float beta)
{
    SolvePoissonSystem(swap, swap.Front(), alpha, beta);
}

vec2 InkBox2DSimulation::RandPos()
{
    int x = rand() % width;
    int y = rand() % height;
    return vec2(x, y);
}

void InkBox2DSimulation::TickDropletsMode()
{
    static float acc = 0;
    static float next_drop = 0;
    static ivec2 freq_range(200, 1500);

    acc += (timestep * 1000);
    if (acc >= next_drop)
    {
        acc = 0;
        next_drop = (rand() % (freq_range.y - freq_range.x)) + freq_range.x;

        vec2 rand_pos0, rand_pos1;
        rand_pos0 = RandPos();
        rand_pos1 = RandPos();
        impulseState.LastPos = vec3(rand_pos0.x, rand_pos0.y, 0);
        impulseState.CurrentPos = vec3(rand_pos1.x, rand_pos1.y, 0);
        impulseState.Delta = impulseState.CurrentPos - impulseState.LastPos;
        impulseState.ForceActive = true;
        impulseState.InkActive = true;
        impulseState.Radial = true;
    }
    else
    {
        impulseState.ForceActive = false;
        impulseState.InkActive = false;
        impulseState.Radial = false;
    }
}

void InkBox2DSimulation::CopyFBO(FBO& dest, FBO& src)
{
    dest.Bind();
    copyShader.Use();
    copyShader.SetInt("field", 0);
    src.BindTexture(0);
    DrawQuad();
}

///////////////////////////////
///     SimulationFields    ///
///////////////////////////////

SimulationFields::SimulationFields(int width, int height, int depth)
    : Velocity(width, height, depth)
    , Pressure(width, height, depth)
    , Vorticity(width, height, depth)
    , Ink(width, height, depth)
    , VelocityVis(width, height, depth)
    , PressureVis(width, height, depth)
    , InkVis(width, height, depth)
    , VorticityVis(width, height, depth)
    , Temp(width, height, depth)
{
}

FBO& SimulationFields::Get(SimulationField field)
{
    if (field == SimulationField::Velocity)
        return VelocityVis;
    else if (field == SimulationField::Ink)
        return InkVis;
    else if (field == SimulationField::Vorticity)
        return VorticityVis;
    else
        return PressureVis;
}

void SimulationFields::Resize(int w, int h, GLShaderProgram& shader, VertexList& quad)
{
    Velocity.Resize(w, h, shader, quad);
    Pressure.Resize(w, h, shader, quad);
    Vorticity.Resize(w, h, shader, quad);
    Ink.Resize(w, h, shader, quad);
    VelocityVis.Resize(w, h, shader, quad);
    PressureVis.Resize(w, h, shader, quad);
    InkVis.Resize(w, h, shader, quad);
    VorticityVis.Resize(w, h, shader, quad);
    Temp.Resize(w, h, shader, quad);
}
