#include "VulkanApp.h"
#include "World.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
    World world;

    Sphere s1(glm::vec3(0.0f, 0.0f, -1.0f), 0.5f);
    Sphere s2(glm::vec3(0.0,-100.5,-1.0), 100.0);

    world.spheres.push_back(s1);
    world.spheres.push_back(s2);

    return StartApp<VulkanAppBase>(world, hInstance, CommandLineArgs(__argc, __argv));
}