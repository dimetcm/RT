#pragma once

#include <glm/glm.hpp>

struct Sphere
{
    Sphere(const glm::vec3& inCenter, float inRadius) : center(inCenter), radius(inRadius) {}

    glm::vec3 center;
    float radius;
};