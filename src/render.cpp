#include "render.h"
#include "intersect.h"
#include "light.h"
#include "screen.h"
#include "texture.h"
#include <framework/trackball.h>
#ifdef NDEBUG
#include <omp.h>
#endif

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
            center = { 1.5f / 4, 0.5f / 3 };
            shift = { p.x / 8, -p.z / 6 };
        } else { // face 3
            center = { 1.5f / 4, 2.5f / 3 };
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
    HitInfo hitInfo;
    if (bvh.intersect(ray, hitInfo, features)) {

        glm::vec3 Lo = computeLightContribution(scene, bvh, features, ray, hitInfo);

        if (features.enableRecursive) {
            Ray reflection = computeReflectionRay(ray, hitInfo);
            // Verifying if the ray intersects a specular surface and if the rayDepth is less than 5
            if (hitInfo.material.ks != glm::vec3 { 0.0, 0.0, 0.0 } && rayDepth < 5)
                // Adding the reflected light with consideration to the specularity
                Lo += hitInfo.material.ks * getFinalColor(scene, bvh, reflection, features, rayDepth + 1);
        }

        // Draw a ray of the color of the surface if it hits the surface and the shading is enabled.
        if (features.enableShading) {
            drawRay(ray, Lo);
        }
        // Draw a black ray if the shading is disabled.
        else {
            drawRay(ray, glm::vec3(0.0, 0.0, 0.0));
        }

        // Set the color of the pixel to the color of the surface if the ray hits.
        return Lo;
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
            screen.setPixel(x, y, getFinalColor(scene, bvh, cameraRay, features));
        }
    }
}