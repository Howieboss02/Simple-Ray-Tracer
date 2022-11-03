#include "render.h"
#include "intersect.h"
#include "light.h"
#include "screen.h"
#include <framework/trackball.h>
#include <iostream>
#ifdef NDEBUG
#include <omp.h>
#endif
#include "iostream"
#include "cmath"

glm::vec3 getFinalColor(const Scene& scene, const BvhInterface& bvh, Ray ray, const Features& features, int rayDepth)
{
    HitInfo hitInfo;
    if (bvh.intersect(ray, hitInfo, features)) {

        glm::vec3 Lo = computeLightContribution(scene, bvh, features, ray, hitInfo);
        glm::vec3 finalColor(0, 0, 0);
        bool isTransparencyEnabled = false;

        if (features.enableRecursive) {
            Ray reflection = computeReflectionRay(ray, hitInfo);
            // Verifying if the ray intersects a specular surface and if the rayDepth is less than 5
            if (hitInfo.material.ks != glm::vec3 { 0.0, 0.0, 0.0 } && rayDepth < 5)
                // Adding the reflected light with consideration to the specularity
                Lo += hitInfo.material.ks * getFinalColor(scene, bvh, reflection, features, rayDepth + 1);
        }
        if (features.extra.enableTransparency) {
            isTransparencyEnabled = true;
            Ray transparentRay = { ray.origin + ray.direction * (0.000001f + ray.t), ray.direction, std::numeric_limits<float>::max() };
            Ray reflection = computeReflectionRay(ray, hitInfo);
            // Verifying if the ray intersects a surface with lower transparency and if the rayDepth is less than 5
            if (rayDepth < 5 && hitInfo.material.transparency < 1.0f) {
                finalColor += hitInfo.material.transparency * (Lo) + (1 - hitInfo.material.transparency) * (getFinalColor(scene, bvh, transparentRay, features, rayDepth + 1));
            } else if (rayDepth < 5 && hitInfo.material.transparency == 1.0f) {
                finalColor += Lo;
            }
        }

        // Draw a ray of the color of the surface if it hits the surface and the shading is enabled.
        if (features.enableShading || features.extra.enableTransparency) {
            drawRay(ray, Lo);
        }
        else {
            drawRay(ray, glm::vec3(0.0, 0.0, 0.0));
        }

        if (isTransparencyEnabled) {
            return finalColor;
        } else {
            return Lo;
        }

    } else {
        // Draw a red debug ray if the ray missed.

        drawRay(ray, glm::vec3(1.0f, 0.0f, 0.0f));
        // Set the color of the pixel to black if the ray misses.
        return glm::vec3(0.0f);
    }
}

glm::vec3 DOF (const Scene scene, const BvhInterface bvh, const Features features, Ray ray){
    glm::vec3 Lo = {0.0, 0.0, 0.0};
    glm::vec3 ConvergePoint = ray.origin + ray.direction * (float)scene.focalLength;
    glm::vec3 trueOrigin = ray.origin;
    srand(time(0));
    for(int i = 0; i < scene.DOF_samples; i ++){
        float r1 = (-1.0f) + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(1.0f - (- 1.0f))));
        float r2 = (-1.0f) + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(1.0f - (- 1.0f))));
        float r3 = (-1.0f) + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(1.0f - (- 1.0f))));
        //std::cout<< r1 << " " << r2 << '\n';
        ray.origin = trueOrigin + glm::vec3 {r1 * scene.aperture, r2 * scene.aperture, r3 * scene.aperture};
        ray.direction = glm::normalize(ConvergePoint - ray.origin);
        Lo += getFinalColor(scene, bvh, ray, features) / (float) scene.DOF_samples;
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
            const Ray cameraRay = camera.generateRay(normalizedPixelPos);
            if(features.extra.enableDepthOfField){
                screen.setPixel(x, y, DOF(scene, bvh, features, cameraRay));
            }else{
                screen.setPixel(x, y, getFinalColor(scene, bvh, cameraRay, features));
            }
        }
    }
}