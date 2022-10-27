#include "texture.h"
#include <cmath>
#include <glm/geometric.hpp>
#include <shading.h>

const glm::vec3 computeShading(const glm::vec3& lightPosition, const glm::vec3& lightColor, const Features& features, Ray ray, HitInfo hitInfo)
{
    // TODO: implement the Phong shading model.
    return hitInfo.material.kd;
}


const Ray computeReflectionRay (Ray ray, HitInfo hitInfo)
{
    Ray reflectionRay {};
    /*
     Creating an offset since the origin of the reflected ray
     is under the surface of the object and that affects the result
     */
    float offset = glm::pow(10, -5);

    // Calculating the direction of the reflected ray using the formula for the reflection vector
    reflectionRay.direction = glm::normalize(ray.direction) - 2.0f * glm::dot(glm::normalize(ray.direction), glm::normalize(hitInfo.normal)) * glm::normalize(hitInfo.normal);
    // Vertex where the ray hits the plane transposed outside the surface by an offset.
    reflectionRay.origin = ray.origin + ray.direction * ray.t + offset * reflectionRay.direction;
    // Setting the variable t to max.
    reflectionRay.t = std::numeric_limits<float>::max();

    return reflectionRay;

}