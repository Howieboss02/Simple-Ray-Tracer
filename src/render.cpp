#include "render.h"
#include "intersect.h"
#include "light.h"
#include "screen.h"
#include <framework/trackball.h>
#ifdef NDEBUG
#include <omp.h>
#endif
#include "iostream"

void motionBlurDebug(Ray ray, const Scene& scene, const BvhInterface& bvh, const Features& features){
    glm::vec3 Lo = {0, 0, 0};
    glm::vec3 trueOrigin = ray.origin;
    const size_t N = scene.MB_samples;
    HitInfo hitInfo;
    for (size_t t = 0; t < N; t++) {
        float random = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX));
        ray.origin = trueOrigin + glm::normalize(scene.directionVector) * (float)(scene.time0 + (random)* (scene.time1 - scene.time0));
        ray.t = {std::numeric_limits<float>::max()};
        drawRay(ray,{0,1,0});
    }
}

glm::vec3 getFinalColor(const Scene& scene, const BvhInterface& bvh, Ray ray, const Features& features, int rayDepth)
{
    HitInfo hitInfo;
    if (bvh.intersect(ray, hitInfo, features)) {
        glm::vec3 Lo = computeLightContribution(scene, bvh, features, ray, hitInfo);
        if (features.enableRecursive) {
            Ray reflection = computeReflectionRay(ray, hitInfo);
            // Verifying if the ray intersects a specular surface and if the rayDepth is less than 5
            if(hitInfo.material.ks != glm::vec3 {0.0, 0.0, 0.0} && rayDepth < 5)
                // Adding the reflected light with consideration to the specularity
                Lo += hitInfo.material.ks * getFinalColor(scene, bvh, reflection, features, rayDepth + 1);
        }

        // Draw a ray of the color of the surface if it hits the surface and the shading is enabled.
        if(features.enableShading) {
            drawRay(ray, Lo);
        }
        // Draw a black ray if the shading is disabled.
        else{
            drawRay(ray, glm::vec3(0.0, 0.0, 0.0));
        }

        if(features.extra.enableMotionBlur){
            motionBlurDebug(ray, scene, bvh, features);
        }

        // Set the color of the pixel to the color of the surface if the ray hits.
        return Lo;
    } else {
        // Draw a red debug ray if the ray missed.

        drawRay(ray, glm::vec3(1.0f, 0.0f, 0.0f));
        // Set the color of the pixel to black if the ray misses.
        return glm::vec3(0.0f);
    }
}

glm::vec3 motionBlur(Ray ray, const Scene& scene, const BvhInterface& bvh, const Features& features){
    glm::vec3 Lo = {0, 0, 0};
    glm::vec3 trueOrigin = ray.origin;
    const int N = scene.MB_samples;
    for (int t = 0; t < N; t++) {
        float random = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX)) / 2;
        ray.origin = trueOrigin +  glm::normalize(scene.directionVector) * (float)(scene.time0  + random * (scene.time1 - scene.time0) - ((scene.time1 - scene.time0) / 2));
        ray.t = {std::numeric_limits<float>::max()};
        Lo += getFinalColor(scene, bvh, ray, features) / (float)N;
    }
    return Lo;
}

void renderRayTracing(const Scene& scene, const Trackball& camera, const BvhInterface& bvh, Screen& screen, const Features& features)
{
    glm::ivec2 windowResolution = screen.resolution();
    // Enable multi threading in Release mode
#ifdef NDEBUG
#pragma omp parallel for schedule(guided)
#endif
    for (int y = 0; y < windowResolution.y; y++) {
        for (int x = 0; x != windowResolution.x; x++) {
            // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
            const glm::vec2 normalizedPixelPos {
                float(x) / float(windowResolution.x) * 2.0f - 1.0f,
                float(y) / float(windowResolution.y) * 2.0f - 1.0f
            };
             Ray cameraRay = camera.generateRay(normalizedPixelPos);
            if(features.extra.enableMotionBlur) {
                screen.setPixel(x, y, motionBlur(cameraRay, scene, bvh, features));
            }else{
                screen.setPixel(x, y, getFinalColor(scene, bvh, cameraRay, features));
            }
        }

    }
}
