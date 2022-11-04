#include "render.h"
#include "intersect.h"
#include "light.h"
#include "screen.h"
#include "texture.h"
#include <framework/trackball.h>
#include <iostream>
#include <random>
#ifdef NDEBUG
#include <omp.h>
#endif
#include "cmath"

void motionBlurDebug(Ray ray, const Scene& scene, const BvhInterface& bvh, const Features& features){
    glm::vec3 Lo = {0, 0, 0};
    glm::vec3 trueOrigin = ray.origin;
    const size_t N = scene.MB_samples;
    for (size_t t = 0; t < N; t++) {
        float random = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX));
        ray.origin = trueOrigin + glm::normalize(scene.directionVector) * (float)(scene.time0 + (random)* (scene.time1 - scene.time0));
        ray.t = {std::numeric_limits<float>::max()};
        drawRay(ray,{0,1,0});
    }
}

void DOF_debug (const Scene& scene, const BvhInterface& bvh, const Features& features, Ray ray){
    glm::vec3 ConvergePoint = ray.origin + ray.direction * (float)scene.focalLength;
    glm::vec3 trueOrigin = ray.origin;
    for(int i = 0; i < scene.DOF_samples; i ++){
        float r1 = ((-1.0f) + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(1.0f - (- 1.0f))))) / 2;
        float r2 = ((-1.0f) + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(1.0f - (- 1.0f))))) / 2;
        float r3 = ((-1.0f) + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(1.0f - (- 1.0f))))) / 2;
        //std::cout<< r1 << " " << r2 << '\n';
        ray.origin = trueOrigin + glm::vec3 {r1 * scene.aperture, r2 * scene.aperture, r3 * scene.aperture};
        ray.direction = glm::normalize(ConvergePoint - ray.origin);
        drawRay(ray, {0,1,0});
    }
}

Image envImage(std::filesystem::path { DATA_DIR } / "cube_map.jpg");

glm::vec2 getEnvironmentTexelCoords(glm::vec3 p)
{
    auto absX = std::fabs(p.x);
    auto absY = std::fabs(p.y);
    auto absZ = std::fabs(p.z);

    glm::vec2 sideCoords;

    glm::vec2 center { -1, -1 }, shift { -1, -1 };
    if (absX >= absY && absX >= absZ) {
        if (p.x > 0) { // face 0
            center = { 2.5f / 4, 1.5f / 3 };
            shift = { -p.z / 8, p.y / 6 };
        } else { // face 1
            center = { 0.5f / 4, 1.5f / 3 };
            shift = { p.z / 8, p.y / 6 };
        }
    }

    if (absY >= absX && absY >= absZ) {
        if (p.y > 0) { // face 2
            center = { 1.5f / 4, 2.5f / 3 };
            shift = { p.x / 8, -p.z / 6 };
        } else { // face 3
            center = { 1.5f / 4, 0.5f / 3 };
            shift = { p.x / 8, p.z / 6 };
        }
    }

    if (absZ >= absX && absZ >= absY) {
        if (p.z > 0) { // face 4
            center = { 1.5f / 4, 1.5f / 3 };
            shift = { p.x / 8, p.y / 6 };
        } else { // face 5
            center = { 3.5f / 4, 1.5f / 3 };
            shift = { -p.x / 8, p.y / 6 };
        }
    }
    auto ans = center + shift;
    if (!(0 <= ans.x && ans.x <= 1 && 0 <= ans.y && ans.y <= 1)) return {0, 0};
    return ans;
}

glm::vec3 getFinalColor(const Scene& scene, const BvhInterface& bvh, Ray ray, const Features& features, int rayDepth)
{
    // Visual debug for motion blur.
    if(features.extra.enableMotionBlur){
        motionBlurDebug(ray, scene, bvh, features);
    }

    // Visual debug for depth of field.
    if(features.extra.enableDepthOfField){
        DOF_debug(scene, bvh, features, ray);
    }

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

        if(features.extra.enableDepthOfField){
            DOF_debug(scene, bvh, features, ray);
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

        if (features.extra.enableEnvironmentMapping) {
            AxisAlignedBox envBox = { { -64, -64, -64 }, { 64, 64, 64 } };
            if (intersectRayWithShape(envBox, ray)) {
                auto texelCoords = getEnvironmentTexelCoords((ray.origin + ray.direction * ray.t) / 64.0f);
                return acquireTexel(envImage, texelCoords, features);
            }
        }
        return glm::vec3(0.0f);
    }
}

glm::vec3 motionBlur(Ray ray, const Scene& scene, const BvhInterface& bvh, const Features& features){
    glm::vec3 Lo = {0, 0, 0};
    glm::vec3 trueOrigin = ray.origin;
    const int N = scene.MB_samples;
    for (int t = 0; t < N; t++) {
        float random = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX));
        ray.origin = trueOrigin +  glm::normalize(scene.directionVector) * (float)(scene.time0  + random * (scene.time1 - scene.time0) - ((scene.time1 - scene.time0) / 2));
        ray.t = {std::numeric_limits<float>::max()};
        Lo += getFinalColor(scene, bvh, ray, features) / (float)N;
    }
    return Lo;
}

glm::vec3 DOF (const Scene& scene, const BvhInterface& bvh, const Features& features, Ray ray){
    glm::vec3 Lo = {0.0, 0.0, 0.0};
    glm::vec3 ConvergePoint = ray.origin + ray.direction * (float)scene.focalLength;
    glm::vec3 trueOrigin = ray.origin;

    for(int i = 0; i < scene.DOF_samples; i ++){
        float r1 = (-1.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (1.0f - (-1.0f)))));
        float r2 = (-1.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (1.0f - (-1.0f)))));
        float r3 = (-1.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (1.0f - (-1.0f)))));
        //std::cout<< r1 << " " << r2 << '\n';
        ray.origin = trueOrigin + glm::vec3 {r1 * scene.aperture, r2 * scene.aperture, r3 * scene.aperture};
        ray.direction = glm::normalize(ConvergePoint - ray.origin);
        Lo += getFinalColor(scene, bvh, ray, features) / (float) scene.DOF_samples;
    }
    return Lo;
}

void renderRayTracing(const Scene& scene, const Trackball& camera, const BvhInterface& bvh, Screen& screen, const Features& features, const float& threshold, const int& boxSize, const int& numRays)
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

            glm::vec3 colour(0.0f);
            if(features.extra.enableMultipleRaysPerPixel){
                std::random_device rd;
                std::mt19937 mt(rd());
                std::uniform_real_distribution<float> dist(0.0, 1.0);
                for(int i = 0; i < numRays; i++){
                    for(int j = 0; j < numRays; j++){
                        float randX = static_cast<float>(dist(mt)) / RAND_MAX;
                        float randY = static_cast<float>(dist(mt)) / RAND_MAX;
                        float a = (float(i) + randX)/float(numRays) + float(x);
                        float b = (float(j) + randY)/float(numRays) + float(y);
                        const glm::vec2 normalizedPixelPos2 {
                            float(a) / float(windowResolution.x) * 2.0f - 1.0f,
                            float(b) / float(windowResolution.y) * 2.0f - 1.0f
                        };

                        const Ray cameraRay2 = camera.generateRay(normalizedPixelPos2);

                        colour += getFinalColor(scene, bvh, cameraRay2, features);
                    }
                }
                colour /= (numRays * numRays);
                screen.setPixel(x, y, colour);
            } else if (features.extra.enableMotionBlur) {
                const Ray cameraRay = camera.generateRay(normalizedPixelPos);
                screen.setPixel(x, y, motionBlur(cameraRay, scene, bvh, features));
            } else if (features.extra.enableDepthOfField) {
                const Ray cameraRay = camera.generateRay(normalizedPixelPos);
                screen.setPixel(x, y, DOF(scene, bvh, features, cameraRay));
            } else { 
                const Ray cameraRay = camera.generateRay(normalizedPixelPos);
                screen.setPixel(x, y, getFinalColor(scene, bvh, cameraRay, features));
            }
        }
    }

    if(features.extra.enableBloomEffect){
        screen.applyBloomFilter(threshold, 2 * boxSize + 1);
    }
}
