#include "VulkanApp.h"
#include "World.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
    World world;

    world.camera.position = glm::vec3(0.0f, 0.0f, -2.0f);
    world.camera.direction = glm::vec3(0.0f, 0.0f, 1.0f);

    Sphere s1(glm::vec3(+0.0f, 0.0f, -1.0f), 0.5f);
    Sphere s2(glm::vec3(-2.0f, 0.0f, -1.0f), 0.5f);
    Sphere s3(glm::vec3(+2.0f, 0.0f, -1.0f), 0.5f);
    Sphere s4(glm::vec3(0.0f,-100.5f,-1.0f), 100.0f);

    world.spheres.push_back(s1);
    world.spheres.push_back(s2);
    world.spheres.push_back(s3);
    world.spheres.push_back(s4);

    return StartApp<VulkanAppBase>(world, hInstance, CommandLineArgs(__argc, __argv));
}