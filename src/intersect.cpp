#include "intersect.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/geometric.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/vector_relational.hpp>
DISABLE_WARNINGS_POP()
#include <cmath>
#include <limits>
#include <iostream>


bool sameSide(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& a, const glm::vec3& b)
{
    auto cross1 = glm::cross(b - a, p1 - a);
    auto cross2 = glm::cross(b - a, p2 - a);
    if (glm::dot(cross1, cross2) >= 0)
        return true;

    return false;
}

bool pointInTriangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& n, const glm::vec3& p) {
    return sameSide(p, v0, v1, v2) && sameSide(p, v1, v0, v2) && sameSide(p, v2, v0, v1);
}

bool intersectRayWithPlane(const Plane& plane, Ray& ray)
{
    if (glm::dot(ray.direction, plane.normal) == 0.0f || glm::dot(ray.origin, plane.normal) == plane.D) {
        return false;
    }
    auto prev_t = ray.t;
    auto new_t = (plane.D - glm::dot(ray.origin, plane.normal)) / glm::dot(ray.direction, plane.normal);

    
    if (new_t < 0) {
        return false;
    } else if (new_t <= prev_t) {
        ray.t = new_t;
        return true;
    } 
    return false;
}

Plane trianglePlane(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2)
{
    Plane plane;
    if (glm::length(glm::cross(v1 - v0, v2 - v0)) == 0.0f) {
        plane.normal = { 0, 0, 0 };
        plane.D = 0;
    } else {
        auto normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
        auto D = glm::dot(v0, normal);
        plane.D = D;
        plane.normal = normal;
    }

    return plane;
}

/// Input: the three vertices of the triangle
/// Output: if intersects then modify the hit parameter ray.t and return true, otherwise return false
bool intersectRayWithTriangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, Ray& ray, HitInfo& hitInfo, const Features& features)
{
    auto prev_t = ray.t;
    auto plane = trianglePlane(v0, v1, v2);
    if (!intersectRayWithPlane(plane, ray))
        return false;
    
    auto p = ray.origin + ray.direction * ray.t;
    auto pointToCamera = glm::normalize(p - ray.origin);

    bool revert = false;
    if(glm::dot(plane.normal, pointToCamera) <= 0) {
        revert = true;
    }

    if (pointInTriangle(v0, v1, v2, plane.normal, p) ) {
        if(features.enableNormalInterp){

            // hitInfo.normal = interpolated normal
        } else {
            
            hitInfo.normal = revert ? -plane.normal : plane.normal;
        }
        
        return true;
    }
    ray.t = prev_t;
}

/// Input: a sphere with the following attributes: sphere.radius, sphere.center
/// Output: if intersects then modify the hit parameter ray.t and return true, otherwise return false
bool intersectRayWithShape(const Sphere& sphere, Ray& ray, HitInfo& hitInfo)
{
    // auto A = glm::dot(ray.direction, ray.direction);
    // auto B = 2 * (ray.direction.x * (ray.origin.x - sphere.center.x) + ray.direction.y * (ray.origin.y - sphere.center.y) + ray.direction.z * (ray.origin.z - sphere.center.z));
    // auto C = glm::pow((ray.origin.x - sphere.center.x), 2) + glm::pow((ray.origin.y - sphere.center.y), 2) + glm::pow((ray.origin.z - sphere.center.z), 2) - glm::pow(sphere.radius, 2);

    // auto delta = B * B - 4 * A * C;
    // auto prev_t = ray.t;
    // float new_t;

    // if (delta < 0.0f) {  // negative delta 
    //     //std::cerr << "negative\n";
    //     return false;
    // } else if (delta == 0.0f) {
    //     //std::cerr << "zero\n";

    //     new_t = -B / (2 * A);
    //     if (new_t > 0 && new_t <= prev_t) { // delta = 0
    //         ray.t = new_t;
    //         return true;
    //     }
    //     return false; 
    // } else { // delta greater than 0
    //     //std::cerr << "positive\n";

    //     auto t_0 = (-B - glm::pow(delta, 0.5f)) / (2 * A);
    //     auto t_1 = (-B + glm::pow(delta, 0.5f)) / (2 * A);

        
    //     if (t_0 <= 0 && t_1 <= 0) {
    //         return false;
    //     } else if (t_0 <= 0) {
    //         if (t_1 <= prev_t) {
    //             ray.t = t_1;
    //             return true;
    //         }
    //     }
    //     auto new_t = t_0;
    //     if (new_t <= prev_t) {
    //         ray.t = new_t;
    //         return true;
    //     }
    //     return false;
    // }
    return false;
}

// bool intersectRayWithFace(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, Ray& ray)
// {
//     return intersectRayWithTriangle(v0, v1, v2, ray, HitInfo()) || intersectRayWithTriangle(v2, v3, v0, ray);
// }

/// Input: an axis-aligned bounding box with the following parameters: minimum coordinates box.lower and maximum coordinates box.upper
/// Output: if intersects then modify the hit parameter ray.t and return true, otherwise return false
bool intersectRayWithShape(const AxisAlignedBox& box, Ray& ray)
{
    
    // auto t_x_min = (box.lower.x - ray.origin.x) / ray.direction.x;
    // auto t_y_min = (box.lower.y - ray.origin.y) / ray.direction.y;
    // auto t_z_min = (box.lower.z - ray.origin.z) / ray.direction.z;
    // auto t_x_max = (box.upper.x - ray.origin.x) / ray.direction.x;
    // auto t_y_max = (box.upper.y - ray.origin.y) / ray.direction.y;
    // auto t_z_max = (box.upper.z - ray.origin.z) / ray.direction.z;

    // auto t_in_x = std::min(t_x_min, t_x_max);
    // auto t_out_x = std::max(t_x_min, t_x_max);
    // auto t_in_y = std::min(t_y_min, t_y_max);
    // auto t_out_y = std::max(t_y_min, t_y_max);
    // auto t_in_z = std::min(t_z_min, t_z_max);
    // auto t_out_z = std::max(t_z_min, t_z_max);

    // auto t_in = std::max(t_in_x, std::max(t_in_y, t_in_z));
    // auto t_out = std::min(t_out_x, std::min(t_out_y, t_out_z));
    // if (t_in > t_out || t_out < 0 || t_in > ray.t) {
    //     return false;
    // }
    // ray.t = t_in;
    // if (t_in <= 0) {
    //     ray.t = t_out;
    // }
    // return true;

    // const auto v0 = box.lower, v7 = box.upper;
    // const auto v1 = glm::vec3 { v0.x, v7.y, v0.z };
    // const auto v2 = glm::vec3 { v7.x, v7.y, v0.z };
    // const auto v3 = glm::vec3 { v7.x, v0.y, v0.z };
    // const auto v4 = glm::vec3 { v0.x, v7.y, v7.z };
    // const auto v5 = glm::vec3 { v7.x, v7.y, v7.z };
    // const auto v6 = glm::vec3 { v7.x, v0.y, v7.z };

    // return intersectRayWithFace(v0, v1, v2, v3, ray) || intersectRayWithFace(v4, v5, v6, v7, ray)
    //     || intersectRayWithFace(v0, v1, v5, v4, ray) || intersectRayWithFace(v1, v2, v6, v5, ray)
    //     || intersectRayWithFace(v2, v3, v7, v6, ray) || intersectRayWithFace(v3, v0, v4, v7, ray);
    return false;
}
