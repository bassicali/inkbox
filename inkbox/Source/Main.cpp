
#include <iostream>

#include "Simulation.h"

using namespace std;

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800

int main(int argc, char* argv[])
{
    InkBoxWindows app;
    InkBoxSimulation* sim = nullptr;

    try
    {
        if (!app.InitGLContexts(WINDOW_WIDTH, WINDOW_HEIGHT))
        {
            return -1;
        }

        sim = new InkBoxSimulation(app, WINDOW_WIDTH, WINDOW_HEIGHT);

        if (!sim->CreateScene())
        {
            delete sim;
            return -1;
        }

        sim->WindowLoop();
        delete sim;
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