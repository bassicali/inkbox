
#include <iostream>
#include <windows.h>

#include "Simulation2D.h"
#include "Simulation3D.h"
#include "IniConfig.h"

#ifndef NDEBUG
#include "Tests.h"
#endif

using namespace std;

#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 400

#define UI_WINDOW_WIDTH 750
#define UI_WINDOW_HEIGHT 395

#define _3D_FIELD_SIDE 128

once_flag shutdown_flag;

void _AppShutdown()
{
    IniConfig::Get().Save();
}

void _AtExitHandler()
{
    call_once(shutdown_flag, _AppShutdown);
}

BOOL WINAPI _ConsoleCtrlHandler(DWORD type)
{
    if (type == CTRL_CLOSE_EVENT)
    {
        call_once(shutdown_flag, _AppShutdown);
        return TRUE;
    }

    return FALSE;
}

int main(int argc, char* argv[])
{
    vector<string> args;
    for (int i = 1; i < argc; i++)
        args.push_back(string(argv[i]));

#ifndef NDEBUG
    if (args.size() >= 1 && args[0].compare("run-tests") == 0)
    {
        TestManager::Get().RunTests();
        return 0;
    }
#endif

    if (SetConsoleCtrlHandler(_ConsoleCtrlHandler, TRUE) == 0)
    {
        LOG_WARN("Couldn't register console ctrl handler");
    }

    atexit(_AtExitHandler);

    bool is_3d = false;
    bool run_tests = false;
    int cube_w = _3D_FIELD_SIDE;
    int cube_h = _3D_FIELD_SIDE;
    int cube_d = _3D_FIELD_SIDE;

    if (args.size() >= 1 && args[0].compare("3d") == 0)
    {
        is_3d = true;

        if (args.size() >= 4)
        {
            cube_w = atoi(args[1].c_str());
            cube_h = atoi(args[2].c_str());
            cube_d = atoi(args[3].c_str());
        }
        else if (args.size() >= 2)
        {
            cube_w = cube_h = cube_d = atoi(args[1].c_str());
        }
    }

    InkBoxWindows app;

    try
    {
        IniConfig::Get().Print();

        int ctrl_h = UI_WINDOW_HEIGHT;
        int ctrl_w = UI_WINDOW_WIDTH;
        ctrl_w -= is_3d ? 343 : 0;

        if (!app.InitGLContexts(WINDOW_WIDTH, WINDOW_HEIGHT, !is_3d, ctrl_w, ctrl_h))
        {
            return -1;
        }

        if (is_3d)
        {
            InkBox3DSimulation* sim = new InkBox3DSimulation(app, cube_w, cube_h, cube_d);
            if (!sim->CreateScene())
            {
                delete sim;
                return -1;
            }

            sim->WindowLoop();
            delete sim;
        }
        else
        {
            InkBox2DSimulation* sim = new InkBox2DSimulation(app, WINDOW_WIDTH, WINDOW_HEIGHT);

            if (!sim->CreateScene())
            {
                delete sim;
                return -1;
            }

            sim->WindowLoop();
            delete sim;
        }
    }
    catch (exception& ex)
    {
        cout << "Unhandled error: " << ex.what() << endl;
        return -1;
    }
    catch (...)
    {
        cout << "Unhandled error" << endl;
        return -1;
    }

    return 0;
}