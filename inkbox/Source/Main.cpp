
#include <iostream>

#include "Simulation2D.h"
#include "Simulation3D.h"

#ifndef NDEBUG
#include "Tests.h"
#endif

using namespace std;

#define WINDOW_WIDTH 500
#define WINDOW_HEIGHT 500

#define UI_WINDOW_WIDTH 750
#define UI_WINDOW_HEIGHT 395

#define _3D_FIELD_WIDTH 128
#define _3D_FIELD_HEIGHT 128
#define _3D_FIELD_DEPTH 128

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

    bool is_3d = false;
    bool run_tests = false;
    int cube_w = _3D_FIELD_WIDTH;
    int cube_h = _3D_FIELD_HEIGHT;
    int cube_d = _3D_FIELD_DEPTH;

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
        int ctrl_h = UI_WINDOW_HEIGHT;
        int ctrl_w = UI_WINDOW_WIDTH;
        ctrl_w -= is_3d ? 343 : 0;

        if (!app.InitGLContexts(WINDOW_WIDTH, WINDOW_HEIGHT, ctrl_w, ctrl_h))
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