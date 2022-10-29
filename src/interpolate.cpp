#include "interpolate.h"
#include <glm/geometric.hpp>

glm::vec3 computeBarycentricCoord(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& p)
{
    const auto d0 = v1 - v0, d1 = v2 - v1, d2 = v0 - v2;
    const auto a0 = glm::length(glm::cross(d1, v1 - p));
    const auto a1 = glm::length(glm::cross(d2, v2 - p));
    const auto a2 = glm::length(glm::cross(d0, v0 - p));
    const auto sum = a0 + a1 + a2;
    if (sum == 0) {
        return { 1, 0, 0 };
    }
    return { a0 / sum, a1 / sum, a2 / sum };
}

glm::vec3 interpolateNormal(const glm::vec3& n0, const glm::vec3& n1, const glm::vec3& n2, const glm::vec3 barycentricCoord)
{
    return barycentricCoord.x * n0 + barycentricCoord.y * n1 + barycentricCoord.z * n2;
}

glm::vec2 interpolateTexCoord(const glm::vec2& t0, const glm::vec2& t1, const glm::vec2& t2, const glm::vec3 barycentricCoord)
{
    // TODO: implement this function.
    return glm::vec2(0.0);
}
