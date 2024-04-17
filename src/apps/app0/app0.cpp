#include "VulkanApp.h"
#include "World.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
    World world;

    world.camera.position = glm::vec3(0.0f, 0.0f, -2.0f);
    world.camera.direction = glm::vec3(0.0f, 0.0f, 1.0f);

    MaterialInfo mi1 = world.materialManager.CreateMaterial(LambertianMaterialProperties(glm::vec3(1.0f, 0.0f, 0.0f)));
    MaterialInfo mi2 = world.materialManager.CreateMaterial(LambertianMaterialProperties(glm::vec3(0.0f, 1.0f, 0.0f)));
    MaterialInfo mi3 = world.materialManager.CreateMaterial(LambertianMaterialProperties(glm::vec3(0.0f, 0.0f, 1.0f)));
    MaterialInfo mi4 = world.materialManager.CreateMaterial(LambertianMaterialProperties(glm::vec3(0.7f, 0.7f, 0.7f)));

    SpherePrimitive sp1(glm::vec3(+0.0f, 0.0f, -1.0f), 0.5f);
    SpherePrimitive sp2(glm::vec3(-2.0f, 0.0f, -1.0f), 0.5f);
    SpherePrimitive sp3(glm::vec3(+2.0f, 0.0f, -1.0f), 0.5f);
    SpherePrimitive sp4(glm::vec3(+0.0f, -100.5f, -1.0f), 100.0f);

    world.spheres.push_back({sp1, mi1});
    world.spheres.push_back({sp2, mi2});
    world.spheres.push_back({sp3, mi3});
    world.spheres.push_back({sp4, mi4});

    return StartApp<VulkanAppBase>(world, hInstance, CommandLineArgs(__argc, __argv));
}