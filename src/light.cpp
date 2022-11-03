#include "light.h"
#include "config.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/geometric.hpp>
DISABLE_WARNINGS_POP()
#include <cmath>
#include <cstdlib>
#include <time.h>

// samples a segment light source
// you should fill in the vectors position and color with the sampled position and color
void sampleSegmentLight(const SegmentLight& segmentLight, glm::vec3& position, glm::vec3& color, const float t)
{
    const auto& x = segmentLight.endpoint0.x * t + segmentLight.endpoint1.x * (1 - t);
    const auto& y = segmentLight.endpoint0.y * t + segmentLight.endpoint1.y * (1 - t);
    const auto& z = segmentLight.endpoint0.z * t + segmentLight.endpoint1.z * (1 - t);

    const auto& r = segmentLight.color0.x * t + segmentLight.color1.x * (1 - t);
    const auto& g = segmentLight.color0.y * t + segmentLight.color1.y * (1 - t);
    const auto& b = segmentLight.color0.z * t + segmentLight.color1.z * (1 - t);

    position = glm::vec3 { x, y, z };
    color = glm::vec3(r, g, b);
}

// samples a parallelogram light source
// you should fill in the vectors position and color with the sampled position and color
// x and y are uniform random numbers to multiply the edge01 and edge02
void sampleParallelogramLight(const ParallelogramLight& parallelogramLight, glm::vec3& position, glm::vec3& color, const float x, const float y)
{
    position = glm::vec3(0.0);
    color = glm::vec3(0.0);
    // TODO: implement this function.
    const auto& p = parallelogramLight;
    position = p.v0 + x * p.edge01 + y * p.edge02;
    color = p.color0 * (1 - x) * (1 - y) + p.color1 * x * (1 - y) + p.color2 * y * (1 - x) + p.color3 * x * y;
}

// test the visibility at a given light sample
// returns 1.0 if sample is visible, 0.0 otherwise
float testVisibilityLightSample(
    const glm::vec3& samplePos,
    const glm::vec3& debugColor,
    const BvhInterface& bvh,
    const Features& features,
    Ray ray,
    HitInfo hitInfo)
{
    if (!features.enableHardShadow && !features.enableSoftShadow) {
        return 1;
    }
    const auto intersectionPoint = ray.origin + ray.direction * ray.t;
    if (intersectionPoint == samplePos) {
        return 1;
    }
    auto lightRayColor = debugColor;
    float ans = 1;
    Ray newRay = { intersectionPoint, samplePos - intersectionPoint, 1 };
    newRay.origin += glm::normalize(newRay.direction) * 0.001f;
    if (bvh.intersect(newRay, hitInfo, features) && newRay.t < 1 - 0.01) {
        lightRayColor = { 1, 0, 0 };
        ans = 0.0;
    }
    if (features.enableSoftShadow) {
        drawRay(newRay, lightRayColor);
    }
    if (features.enableHardShadow) {
        newRay.t = 1;
        drawRay(newRay, lightRayColor);
    }
    drawRay(newRay, lightRayColor);
    return ans;
}

// given an intersection, computes the contribution from all light sources at the intersection point
// in this method you should cycle the light sources and for each one compute their contribution
// don't forget to check for visibility (shadows!)

// Lights are stored in a single array (scene.lights) where each item can be either a PointLight, SegmentLight or ParallelogramLight.
// You can check whether a light at index i is a PointLight using std::holds_alternative:
// std::holds_alternative<PointLight>(scene.lights[i])
//
// If it is indeed a point light, you can "convert" it to the correct type using std::get:
// PointLight pointLight = std::get<PointLight>(scene.lights[i]);
//
//
// The code to iterate over the lights thus looks like this:
// for (const auto& light : scene.lights) {
//     if (std::holds_alternative<PointLight>(light)) {
//         const PointLight pointLight = std::get<PointLight>(light);
//         // Perform your calculations for a point light.
//     } else if (std::holds_alternative<SegmentLight>(light)) {
//         const SegmentLight segmentLight = std::get<SegmentLight>(light);
//         // Perform your calculations for a segment light.
//     } else if (std::holds_alternative<ParallelogramLight>(light)) {
//         const ParallelogramLight parallelogramLight = std::get<ParallelogramLight>(light);
//         // Perform your calculations for a parallelogram light.
//     }
// }
//
// Regarding the soft shadows for **other** light sources **extra** feature:
// To add a new light source, define your new light struct in scene.h and modify the Scene struct (also in scene.h)
// by adding your new custom light type to the lights std::variant. For example:
// std::vector<std::variant<PointLight, SegmentLight, ParallelogramLight, MyCustomLightType>> lights;
//
// You can add the light sources programmatically by creating a custom scene (modify the Custom case in the
// loadScene function in scene.cpp). Custom lights will not be visible in rasterization view.
glm::vec3 computeLightContribution(const Scene& scene, const BvhInterface& bvh, const Features& features, Ray ray, HitInfo hitInfo)
{
    if (features.enableShading) {
        // If shading is enabled, compute the contribution from all lights.
        // Creating a nul vector which will be the result of all computation of all light sources
        glm::vec3 res = { 0.0, 0.0, 0.0 };
        srand(time(0));
        // Iterating through all the light sources
        for (const auto& light : scene.lights) {
            if (std::holds_alternative<PointLight>(light)) {
                // If the light is a PointLight, add the result of the computeShading method to the res vector
                const PointLight pointLight = std::get<PointLight>(light);
                res += computeShading(pointLight.position, pointLight.color, features, ray, hitInfo)
                    * testVisibilityLightSample(pointLight.position, pointLight.color, bvh, features, ray, hitInfo);
            } else if (std::holds_alternative<SegmentLight>(light)) {
                const SegmentLight segmentLight = std::get<SegmentLight>(light);
                if (features.enableSoftShadow) {
                    const size_t N = 42;
                    for (size_t t = 0; t < N; t++) {
                        float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                        auto trand = (float)t + r;
                        auto position = glm::vec3(0.0);
                        auto color = glm::vec3(0.0);
                        sampleSegmentLight(segmentLight, position, color, trand / (float)N);

                        res += computeShading(position, color, features, ray, hitInfo) / (float)N
                            * testVisibilityLightSample(position, color, bvh, features, ray, hitInfo);
                    }
                }
            } else if (std::holds_alternative<ParallelogramLight>(light)) {
                const ParallelogramLight parallelogramLight = std::get<ParallelogramLight>(light);
                if (features.enableSoftShadow) {
                    const size_t N = 10;
                    for (size_t i = 0; i < N; i++) {
                        for (size_t j = 0; j < N; j++) {
                            float r1 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                            float r2 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                            auto xrand = (float)i + r1;
                            auto yrand = (float)j + r2;
                            auto position = glm::vec3(0.0);
                            auto color = glm::vec3(0.0);
                            sampleParallelogramLight(parallelogramLight, position, color, xrand / (float)N, yrand / (float)N);
                            res += computeShading(position, color, features, ray, hitInfo) / (float)(N * N)
                                * testVisibilityLightSample(position, color, bvh, features, ray, hitInfo);
                        }
                    }
                }
            }
        }
        return res;
    } else {
        // If shading is disabled, return the albedo of the material.
        return hitInfo.material.kd;
    }
}
