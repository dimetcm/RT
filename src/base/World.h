#pragma once

#include "GeometryPrimitives.h"
#include "Materials.h"

#include <vector>

struct Sphere
{
    SpherePrimitive shape;
    MaterialInfo material;
};

struct World
{
    struct
    {
        glm::vec3 position = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 direction = glm::vec3(0.0f, 0.0f, 1.0f);
    } camera;

    MaterialManager materialManager;

    std::vector<Sphere> spheres;
};