#include "texture.h"
#include <cmath>
#include <glm/geometric.hpp>
#include <shading.h>

const glm::vec3 computeShading(const glm::vec3& lightPosition, const glm::vec3& lightColor, const Features& features, Ray ray, HitInfo hitInfo)
{
    //Computing the position of the ray on the plane
    glm::vec3 rayPosition = ray.origin + ray.direction * ray.t;
    //Computing the position light-vector and also normalizing it
    glm::vec3 lightVector = glm::normalize(lightPosition - rayPosition);
    //Normalizing the normal
    glm::vec3 normal = glm::normalize(hitInfo.normal);
    //The camera-vector is the vector of the origin of the ray
    glm::vec3 cameraVector = ray.origin;
    //Computing the reflection-vector using the normal and the light-vector
    glm::vec3 reflectionVector = 2 * glm::dot(lightVector, normal) * normal - lightVector;

    //Computing the standard lambertian shading using the formula
    glm::vec3 lambertian;
    lambertian = lightColor * hitInfo.material.kd * glm::clamp(glm::dot(normal, lightVector), 0.0f, 1.0f);

    //Computing the Phong-Specular Shading using the formula
    glm::vec3 phongSpecular;
    phongSpecular = lightColor * hitInfo.material.ks * glm::pow(glm::clamp(glm::dot(reflectionVector, glm::normalize(cameraVector)), 0.0f, 1.0f), hitInfo.material.shininess);

    //The direct illumination is the addition of the Lambertian shading model and the Phong-Specular shading model
    return lambertian + phongSpecular;
}


const Ray computeReflectionRay (Ray ray, HitInfo hitInfo)
{
    // Do NOT use glm::reflect!! write your own code.
    Ray reflectionRay {};
    // TODO: implement the reflection ray computation.
    return reflectionRay;
}