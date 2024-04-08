#pragma once

#include "GeometryPrimitives.h"
#include <vector>

struct World
{
    struct
    {
        glm::vec3 position = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 direction = glm::vec3(0.0f, 0.0f, 1.0f);
    } camera;
    
    std::vector<Sphere> spheres;
};