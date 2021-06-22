
#include <cstdlib>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/geometric.hpp>

#include "Simulation3D.h"
#include "Utils.h"
#include "IniConfig.h"

using namespace std;
using namespace glm;

vector<GLComputeShader*> compute_shaders;

InkBox3DSimulation::InkBox3DSimulation(const InkBoxWindows& app, int width, int height, int depth)
    : window(app.Main)
    , width(width)
    , height(height)
    , depth(depth)
    , textures(width, height, depth)
    , limiter(60)
    , scrollAcc(0)
    , delta_t(0)
    , computeLocalSize(4, 4, 4)
{
    glfwGetWindowSize(window, &wwidth, &wheight);
    computeWorkGroups = uvec3(width / computeLocalSize.x, height / computeLocalSize.y, depth / computeLocalSize.z);

    vars.GridScale = 1.0f / width;
    vars.Viscosity = 0.0004;
    vars.InkViscosity = 0.0005;
    vars.BoundariesEnabled = false;
    vars.SplatRadius = width * 0.37;
    vars.InkVolume = width * 0.37;
    vars.Gravity = 8;
    vars.InkColour = vec4(1, 1, 1, 1);
    ui.SetValues(vars);
    controlPanel = ControlPanel(app.Controls, &vars, &ui, &impulseState);
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    void* ptr = glfwGetWindowUserPointer(window);
    if (ptr)
    {
        auto* sim = reinterpret_cast<InkBox3DSimulation*>(ptr);
        sim->ScrollCallback(xoffset, yoffset);
    }
}

void PrintMatrix(mat4 mat)
{
    printf("\t%.3f   %.3f   %.3f   %.3f\n", mat[0][0], mat[1][0], mat[2][0], mat[3][0]);
    printf("\t%.3f   %.3f   %.3f   %.3f\n", mat[0][1], mat[1][1], mat[2][1], mat[3][1]);
    printf("\t%.3f   %.3f   %.3f   %.3f\n", mat[0][2], mat[1][2], mat[2][2], mat[3][2]);
    printf("\t%.3f   %.3f   %.3f   %.3f\n", mat[0][3], mat[1][3], mat[2][3], mat[3][3]);
}

void PrintVec4(vec4 vec)
{
    printf("\t%.3f   %.3f   %.3f   %.3f\n", vec[0], vec[1], vec[2], vec[3]);
}

bool _InitComputeShader(const char* file, GLComputeShader& program, uvec3 local_size, string img_format)
{


    GLShader cs(file, ShaderType::Compute, local_size, img_format);

    if (!cs.Compile())
        return false;

    program.Init();
    program.Attach(cs);
    if (!program.Link())
        return false;

    program.Name = cs.FileName();
    compute_shaders.push_back(&program);
    return true;
}


bool _InitFragmentShader(const char* file, GLShader& vs, GLShaderProgram& program, string img_format)
{
    GLShader fs(file, ShaderType::Fragment, uvec3(), img_format);
    if (!fs.Compile())
        return false;

    program.Init();
    program.Attach(fs);
    program.Attach(vs);
    if (!program.Link())
        return false;

    return true;
}

bool InkBox3DSimulation::CreateScene()
{
    vec3 c(0.5, 0.5, 0.5);
    float verts[24] =
    {
         c.x, -c.y,  c.z,
         c.x,  c.y,  c.z,
        -c.x,  c.y,  c.z,
        -c.x, -c.y,  c.z,

         c.x, -c.y, -c.z,
         c.x,  c.y, -c.z,
        -c.x,  c.y, -c.z,
        -c.x, -c.y, -c.z,
    };

    unsigned int indices[36] =
    {
        7, 4, 5,
        5, 6, 7,

        3, 0, 1,
        1, 2, 3,

        2, 6, 7,
        7, 3, 2,

        1, 5, 4,
        4, 0, 1,

        7, 4, 0,
        0, 3, 7,

        6, 5, 1,
        1, 2, 6
    };

    cube.Init(&verts[0], 24, &indices[0], 36);
    cubeModel = mat4(1.0f);
    cubeModel = scale(cubeModel, vec3(1.0f, float(height) / width, float(depth) / width));

    cubeVertices[0] = vec3(-0.5, 0.5, 0.5);   // TTL
    cubeVertices[1] = vec3(0.5, 0.5, 0.5);    // TTR
    cubeVertices[2] = vec3(-0.5, 0.5, -0.5);  // TBL
    cubeVertices[3] = vec3(0.5, 0.5, -0.5);   // TBR
    cubeVertices[4] = vec3(-0.5, -0.5, 0.5);  // BTL
    cubeVertices[5] = vec3(0.5, -0.5, 0.5);   // BTR
    cubeVertices[6] = vec3(-0.5, -0.5, -0.5); // BBL
    cubeVertices[7] = vec3(0.5, -0.5, -0.5);  // BBR

    unsigned int border_indices[24] =
    {
        0, 1,
        1, 2,
        2, 3,
        3, 0,

        0, 4,
        1, 5,
        2, 6,
        3, 7,

        4, 5,
        5, 6,
        6, 7,
        7, 4
    };

    cubeBorder.Init(&verts[0], 24, &border_indices[0], 24);

    string img_format;
    if (!IniConfig::Get().UseSnormTextures)
        img_format = IniConfig::Get().TextureComponentWidth == 32 ? "rgba32f" : "rgba16f";
    else
        img_format = "rgba16_snorm";

    GLShader vs("3d\\tex_coords.vert", ShaderType::Vertex);
    if (!vs.Compile())
        return false;

    _InitFragmentShader("3d\\view.frag", vs, viewShader, img_format);
    _InitFragmentShader("3d\\border.frag", vs, borderShader, img_format);

    _InitComputeShader("3d\\add_impulse.comp", impulseShader, computeLocalSize, img_format);
    _InitComputeShader("3d\\advection.comp", advectionShader, computeLocalSize, img_format);
    _InitComputeShader("3d\\jacobi.comp", jacobiShader, computeLocalSize, img_format);
    _InitComputeShader("3d\\divergence.comp", divShader, computeLocalSize, string());
    _InitComputeShader("3d\\gradient.comp", gradShader, computeLocalSize, img_format);
    _InitComputeShader("3d\\subtract.comp", subtractShader, computeLocalSize, img_format);
    _InitComputeShader("3d\\boundary.comp", boundaryShader, computeLocalSize, img_format);
    _InitComputeShader("3d\\copy.comp", copyShader, computeLocalSize, img_format);
    _InitComputeShader("3d\\clear.comp", clearShader, computeLocalSize, img_format);

    _GL_WRAP1(glEnable, GL_DEPTH_TEST);
    //_GL_WRAP1(glEnable, GL_BLEND);

    glfwSetWindowUserPointer(window, this);
    glfwSetScrollCallback(window, ::ScrollCallback);

    LOG_INFO("Window size: %dx%d", wwidth, wheight);
    LOG_INFO("Cube dimensions: %dx%dx%d", width, height, depth);

    return true;
}

void InkBox3DSimulation::WindowLoop()
{
    double last_time = 0.f;

    float aspect_ratio = float(wwidth) / wheight;
    projection = perspective(radians(45.0f), aspect_ratio, 0.1f, 100.0f);

    camera.LookAt(vec3(0, 0, 0));
    invProjView = inverse(projection * camera.ViewMatrix());

    while (!glfwWindowShouldClose(window))
    {
        glfwMakeContextCurrent(window);
        double now = glfwGetTime();
        delta_t = last_time == 0 ? 0.016667 : now - last_time;
        last_time = now;

        glfwPollEvents();
        ProcessInputs();
        TickDropletsMode();

        ComputeFields();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer now!

        viewShader.Use();
        viewShader.SetMatrix4x4("model", cubeModel);
        viewShader.SetMatrix4x4("view", camera.ViewMatrix());
        viewShader.SetMatrix4x4("proj", projection);

        viewShader.SetVec3("cube_pos", vec3(0.f, 0.f, 0.f));
        viewShader.SetVec3("camera_wpos", camera.Position());
        viewShader.SetVec3("camera_dir", camera.Direction());
        viewShader.SetVec3("box_size", vec3(width, height, depth));
        viewShader.SetVec4("bg_colour", vec4(0.2f, 0.3f, 0.3f, 1.0f));
        viewShader.SetImage("field", textures.Ink.Front(), 0, GL_READ_ONLY);

        _GL_WRAP1(glBindVertexArray, cube.VAO);
        _GL_WRAP4(glDrawElements, GL_TRIANGLES, cube.NumVertices, GL_UNSIGNED_INT, nullptr);

        // Draw a border around the cube
        borderShader.Use();
        borderShader.SetMatrix4x4("model", cubeModel);
        borderShader.SetMatrix4x4("view", camera.ViewMatrix());
        borderShader.SetMatrix4x4("proj", projection);
        _GL_WRAP1(glBindVertexArray, cubeBorder.VAO);
        _GL_WRAP4(glDrawElements, GL_LINES, cubeBorder.NumVertices, GL_UNSIGNED_INT, nullptr);

        glfwSwapBuffers(window);
        limiter.Regulate();

        // Render control panel window
        bool update, clear;
        glfwMakeContextCurrent(controlPanel.WindowPtr());
        controlPanel.Render(update, clear);

        if (update)
        {
            ui.UpdateVars(vars);
            LOG_INFO("Ink colour: %d, %d, %d", int(vars.InkColour.r * 255), int(vars.InkColour.g * 255), int(vars.InkColour.b * 255));
        }
        else if (clear)
            ClearFields();

        glfwSwapBuffers(controlPanel.WindowPtr());
    }
}

void InkBox3DSimulation::ComputeFields()
{
    if (vars.AdvectInk)
    {
        advectionShader.Use();
        advectionShader.SetFloat("delta_t", delta_t);
        advectionShader.SetFloat("dissipation", 0.99f);
        advectionShader.SetFloat("gs", vars.GridScale);
        advectionShader.SetFloat("gravity", 0);
        advectionShader.SetImage("quantity_r", textures.Ink.Front(), 0, GL_READ_ONLY);
        advectionShader.SetImage("quantity_w", textures.Ink.Back(), 1, GL_WRITE_ONLY);
        advectionShader.SetImage("velocity", textures.Velocity.Front(), 2, GL_READ_ONLY);
        advectionShader.Execute(computeWorkGroups);
        textures.Ink.Swap();
    }

    if (vars.SelfAdvect)
    {
        advectionShader.Use();
        advectionShader.SetFloat("delta_t", delta_t);
        advectionShader.SetFloat("dissipation", 0.98f);
        advectionShader.SetFloat("gs", vars.GridScale);
        advectionShader.SetFloat("gravity", vars.Gravity);
        advectionShader.SetImage("quantity_r", textures.Velocity.Front(), 0, GL_READ_ONLY);
        advectionShader.SetImage("quantity_w", textures.Velocity.Back(), 1, GL_WRITE_ONLY);
        advectionShader.SetImage("velocity", textures.Velocity.Front(), 2, GL_READ_ONLY);
        advectionShader.Execute(computeWorkGroups);
        textures.Velocity.Swap();
    }

    if (impulseState.ForceActive)
    {
        LOG_INFO("Splat: (%.0f, %.0f, %.0f)\tForce: (%.2f, %.2f, %.2f)", impulseState.CurrentPos.x, impulseState.CurrentPos.y, impulseState.CurrentPos.z, impulseState.Delta.x, impulseState.Delta.y, impulseState.Delta.z);
        impulseShader.Use();
        impulseShader.SetVec3("position", impulseState.CurrentPos);
        impulseShader.SetFloat("radius", vars.SplatRadius);
        impulseShader.SetVec4("force", vec4(impulseState.Delta.x, impulseState.Delta.y, impulseState.Delta.z, 0));
        impulseShader.SetImage("field_r", textures.Velocity.Front(), 0, GL_READ_ONLY);
        impulseShader.SetImage("field_w", textures.Velocity.Back(), 1, GL_WRITE_ONLY);
        impulseShader.Execute(computeWorkGroups);
        textures.Velocity.Swap();
        impulseState.ForceActive = false;
    }

    if (impulseState.InkActive)
    {
        vec4 colour;
        if (vars.RainbowMode)
            colour = impulseState.TickRainbowMode(delta_t);
        else
            colour = vars.InkColour;

        impulseShader.Use();
        impulseShader.SetVec3("position", impulseState.CurrentPos);
        impulseShader.SetFloat("radius", vars.InkVolume);
        impulseShader.SetVec4("force", colour);
        impulseShader.SetImage("field_r", textures.Ink.Front(), 0, GL_READ_ONLY);
        impulseShader.SetImage("field_w", textures.Ink.Back(), 1, GL_WRITE_ONLY);
        impulseShader.Execute(computeWorkGroups);
        textures.Ink.Swap();
        impulseState.InkActive = false;
    }

    if (vars.DiffuseVelocity)
    {
        float alpha = (vars.GridScale * vars.GridScale) / (vars.Viscosity * delta_t);
        float beta = alpha + 6.0f;
        SolvePoissonSystem(textures.Velocity, textures.Velocity.Front(), alpha, beta);
    }

    if (vars.DiffuseInk)
    {
        float alpha = (vars.GridScale * vars.GridScale) / (vars.InkViscosity * delta_t);
        float beta = alpha + 6.0;
        SolvePoissonSystem(textures.Ink, textures.Ink.Front(), alpha, beta);
    }

    // Projection
    divShader.Use();
    divShader.SetFloat("gs", vars.GridScale);
    divShader.SetImage("field_r", textures.Velocity.Front(), 0, GL_READ_ONLY);
    divShader.SetImage("field_w", textures.Velocity.Back(), 1, GL_WRITE_ONLY);
    divShader.Execute(computeWorkGroups);

    // Solve for P in: Laplacian(P) = div(W)
    SolvePoissonSystem(textures.Pressure, textures.Velocity.Back(), -1, 6.0f);

    // Calculate grad(P)
    gradShader.Use();
    gradShader.SetFloat("gs", vars.GridScale);
    gradShader.SetImage("field_r", textures.Pressure.Front(), 0, GL_READ_ONLY);
    gradShader.SetImage("field_w", textures.Pressure.Back(), 1, GL_WRITE_ONLY);
    gradShader.Execute(computeWorkGroups);
    // No swap, back buffer has the gradient

    // Calculate U = W - grad(P) where div(U)=0
    subtractShader.Use();
    subtractShader.SetImage("a", textures.Velocity.Front(), 0, GL_READ_ONLY);
    subtractShader.SetImage("b", textures.Pressure.Back(), 1, GL_READ_ONLY);
    subtractShader.SetImage("c", textures.Velocity.Back(), 2, GL_WRITE_ONLY);
    subtractShader.Execute(computeWorkGroups);
    textures.Velocity.Swap();

    if (vars.BoundariesEnabled)
    {
        ComputeBoundaryValues(textures.Velocity, -1);
        ComputeBoundaryValues(textures.Ink, 0);
    }
}

void InkBox3DSimulation::SolvePoissonSystem(SwapTexture& swap, Texture& initial_value, float alpha, float beta)
{
    CopyImage(textures.Temp, initial_value);
    jacobiShader.Use();
    jacobiShader.SetFloat("alpha", alpha);
    jacobiShader.SetFloat("beta", beta);
    jacobiShader.SetImage("fieldb_r", textures.Temp, 0, GL_READ_ONLY);

    for (int i = 0; i < IniConfig::Get().NumJacobiIterations; i++)
    {
        jacobiShader.SetImage("fieldx_r", swap.Front(), 1, GL_READ_ONLY);
        jacobiShader.SetImage("field_out", swap.Back(), 2, GL_WRITE_ONLY);
        jacobiShader.Execute(computeWorkGroups);
        swap.Swap();
    }
}

void InkBox3DSimulation::ComputeBoundaryValues(SwapTexture& swap, float scale)
{
    boundaryShader.Use();
    boundaryShader.SetFloat("scale", scale);
    boundaryShader.SetVec3("box_size", vec3(width, height, depth));
    boundaryShader.SetImage("field_r", swap.Front(), 0, GL_READ_ONLY);
    boundaryShader.SetImage("field_w", swap.Back(), 1, GL_WRITE_ONLY);
    swap.Swap();
}

void InkBox3DSimulation::CopyImage(Texture& dest, Texture& src)
{
    copyShader.Use();
    copyShader.SetImage("src", src, 0, GL_READ_ONLY);
    copyShader.SetImage("dest", dest, 1, GL_WRITE_ONLY);
    copyShader.Execute(computeWorkGroups);
}

void InkBox3DSimulation::ClearFields()
{
    Texture* ptrs[] =
    {
        &textures.Velocity.Front(),
        &textures.Velocity.Back(),
        &textures.Pressure.Front(),
        &textures.Pressure.Back(),
        &textures.Ink.Front(),
        &textures.Ink.Back(),
    };

    clearShader.Use();
    for (int i = 0; i < 6; i++)
    {
        clearShader.SetImage("field_w", *ptrs[i], 0, GL_WRITE_ONLY);
        clearShader.Execute(computeWorkGroups);
    }
}

void InkBox3DSimulation::TickDropletsMode()
{
    static float acc = 0;
    static int next_drop = 0;
    static ivec2 freq_range(30, 60);

    if (vars.DropletsMode)
        acc++;

    if (acc > next_drop || glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
    {
        vec3 rand_pos, rand_force;

        acc = 0;
        next_drop = (rand() % (freq_range.y - freq_range.x)) + freq_range.x;

        if ((rand() & 0x1) == 0x1)
        {
            rand_pos.x = rand() % width;
            rand_pos.y = height / 2;
            rand_pos.z = rand() % depth;

            rand_force.x = 0;
            rand_force.y = -1 * float(rand() % 1000) / 1000 * vars.ForceMultiplier;
            rand_force.z = 0;
        }
        else
        {
            rand_pos.x = width / 2;
            rand_pos.z = rand() % depth;
            rand_pos.y = rand() % height;

            rand_force.x = -1 * float(rand() % 1000) / 1000 * vars.ForceMultiplier;
            rand_force.y = 0;
            rand_force.z = 0;
        }

        impulseState.ForceActive = true;
        impulseState.InkActive = true;
        impulseState.Delta = rand_force;
        impulseState.CurrentPos = rand_pos;
    }
}

void InkBox3DSimulation::UpdatePickCoord()
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    y = wheight - y;

    float hwwidth = wwidth / 2;
    float hwheight = wheight / 2;

    vec4 scr_coord;
    scr_coord.x = (x - hwwidth) / hwwidth; // Move to range [-1,+1]
    scr_coord.y = (y - hwheight) / hwheight;
    scr_coord.z = -2;
    scr_coord.w = 1;

    vec4 pick_coord = invProjView * scr_coord;
    pick_coord /= pick_coord.w;

    //LOG_INFO("Screen click pos: (%.2f, %.2f, %.2f)", scr_coord.x, scr_coord.y, scr_coord.z);
    //LOG_INFO("Click world pos: (%.2f, %.2f, %.2f)", pick_coord.x, pick_coord.y, pick_coord.z);

    vec3 ro = camera.Position();
    vec3 delta = vec3(pick_coord.x, pick_coord.y, pick_coord.z) - ro;
    vec3 rd = normalize(delta);
    vec3 intersection;
    if (utils::LineIntersectsBox(ro, rd, cubeVertices[0], cubeVertices[1], cubeVertices[2], cubeVertices[3], cubeVertices[4], cubeVertices[5], cubeVertices[6], cubeVertices[7], intersection))
    {
        impulseState.CurrentPos = intersection;
        impulseState.CurrentPos += 0.5f;
        impulseState.CurrentPos *= vec3(width, height, depth);

        impulseState.ForceActive = true;
        impulseState.InkActive = true;
        impulseState.Delta = (intersection - ro) * vars.ForceMultiplier;

        //LOG_INFO("Mouse pick pos: (%.2f, %.2f, %.2f)", intersection.x, intersection.y, intersection.z);
    }
}

void InkBox3DSimulation::ProcessInputs()
{
    lock_guard<mutex> lock(scrollMtx);

    static bool orbit_mode = true;
    bool camera_changed = false;

    if (scrollAcc != 0)
    {
        camera.Move(scrollAcc * IniConfig::Get().ScrollSensitivity);
        scrollAcc = 0;
        camera_changed = true;
    }

    static bool left_mouse_down = false;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        static double xpos, ypos;

        double x, y;
        glfwGetCursorPos(window, &x, &y);

        // Prevents snappy movements when user takes their finger off mouse
        if (!left_mouse_down)
        {
            xpos = x;
            ypos = y;
            left_mouse_down = true;
        }
        else
        {
            if (orbit_mode)
            {
                float sensitivity = IniConfig::Get().MouseOrbitSensitivity;
                camera.OrbitY(sensitivity * (xpos - x), vec3(0, 0, 0));
                camera.OrbitX(sensitivity * (ypos - y), vec3(0, 0, 0));
            }
            else
            {
                // fly mode
                camera.Pan(vec2(xpos - x, y - ypos) * 0.01f);
            }

            xpos = x;
            ypos = y;
            camera_changed = true;
        }
    }
    else
    {
        left_mouse_down = false;
    }

    if (orbit_mode)
    {
        float sensitivity = IniConfig::Get().KeyOrbitSensitivity;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            camera.Position(0, camera.Position().y, camera.Position().z);
            camera.OrbitX(sensitivity, vec3(0, 0, 0));
        }
        else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            camera.Position(0, camera.Position().y, camera.Position().z);
            camera.OrbitX(-sensitivity, vec3(0, 0, 0));
        }
        else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            camera.Position(camera.Position().x, 0, camera.Position().z);
            camera.OrbitY(-sensitivity, vec3(0, 0, 0));
        }
        else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            camera.Position(camera.Position().x, 0, camera.Position().z);
            camera.OrbitY(sensitivity, vec3(0, 0, 0));
        }

        camera_changed = true;
    }

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        camera.LookAt(vec3(0, 0, 0));
        camera_changed = true;
    }

    if (camera_changed)
        invProjView = inverse(projection * camera.ViewMatrix());

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        UpdatePickCoord();

    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
    {
        orbit_mode = true;
        LOG_INFO("Orbit mode");
    }
    else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
    {
        orbit_mode = false;
        LOG_INFO("Fly mode");
    }

#if MEASURE_CS_TIMES
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    {
        for (auto* shader : compute_shaders)
        {
            if (shader->TimingInfo().NumExecutions > 0)
            {
                LOG_INFO("Shader: %-12s\tAverage Time: %.3f", shader->Name.c_str(), shader->TimingInfo().AverageTime());
            }
        }
        LOG_INFO("");
    }
#endif
}

void InkBox3DSimulation::ScrollCallback(double xoffset, double yoffset)
{
    lock_guard<mutex> lock(scrollMtx);
    scrollAcc += yoffset;
}