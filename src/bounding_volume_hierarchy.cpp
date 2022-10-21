#include "bounding_volume_hierarchy.h"
#include "draw.h"
#include "interpolate.h"
#include "intersect.h"
#include "scene.h"
#include "texture.h"
#include <algorithm>
#include <glm/glm.hpp>

BoundingVolumeHierarchy::BoundingVolumeHierarchy(Scene* pScene)
    : m_pScene(pScene)
{
    // TODO: implement BVH construction.
    this->m_pScene = pScene;
    auto triangles = std::vector<TriangleOrNode>();
    for (size_t i = 0; i < pScene->meshes.size(); ++i) {
        auto mesh = pScene->meshes[i];
        for (size_t j = 0; j < mesh.triangles.size(); ++j) {
            triangles.push_back(TriangleOrNode { i, j,0 });
        }
    }
    constructorHelper(triangles, 0);
}

glm::vec3 getMedian(TriangleOrNode triangle, Scene& scene)
{
    auto mesh = scene.meshes[triangle.meshIndex];
    auto tr = mesh.triangles[triangle.triangleIndex]; // uvec3, indices of points of a single triangle
    auto p1 = mesh.vertices[tr.x].position;
    auto p2 = mesh.vertices[tr.y].position;
    auto p3 = mesh.vertices[tr.z].position;
    return (p1 + p2 + p3) / 3.0f;
}

AxisAlignedBox getBox(const std::vector<TriangleOrNode>& triangles, const Scene& scene)
{
    glm::vec3 lower = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
    glm::vec3 upper = { std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min() };

    for (auto triangle : triangles) {
        auto mesh = scene.meshes[triangle.meshIndex];
        auto tr = mesh.triangles[triangle.triangleIndex];
        auto v1 = mesh.vertices[tr.x].position, v2 = mesh.vertices[tr.y].position, v3 = mesh.vertices[tr.z].position;
        upper.x = std::max(std::max(upper.x, v1.x), std::max(v2.x, v3.x));
        upper.x = std::max(std::max(upper.y, v1.y), std::max(v2.y, v3.y));
        upper.x = std::max(std::max(upper.z, v1.z), std::max(v2.z, v3.z));

        lower.x = std::min(std::min(upper.x, v1.x), std::min(v2.x, v3.x));
        lower.x = std::min(std::min(upper.y, v1.y), std::min(v2.y, v3.y));
        lower.x = std::min(std::min(upper.z, v1.z), std::min(v2.z, v3.z));
    }
    return { lower, upper };
}

size_t BoundingVolumeHierarchy::constructorHelper(std::vector<TriangleOrNode>& triangles, int whichAxis)
{
    if (triangles.size() <= 20) {
        this->nodes.push_back({ true, triangles, getBox(triangles, *this->m_pScene) });
        this->m_numLeaves += 1;
        return this->nodes.size() - 1;
    }

    std::sort(triangles.begin(), triangles.end(), [this, whichAxis](TriangleOrNode triangle1, TriangleOrNode triangle2) {
        auto& tr1 = this->m_pScene->meshes[triangle1.meshIndex].triangles[triangle1.triangleIndex];
        auto& tr2 = this->m_pScene->meshes[triangle2.meshIndex].triangles[triangle2.triangleIndex];
        return tr1[whichAxis] < tr2[whichAxis];
    });

    auto median = triangles.size() / 2;
    std::vector<TriangleOrNode> left(triangles.begin(), triangles.begin() + median);
    std::vector<TriangleOrNode> right(triangles.begin() + median, triangles.end());

    auto leftIndex = this->constructorHelper(left, (whichAxis + 1) % 3);
    auto rightIndex = this->constructorHelper(right, (whichAxis + 1) % 3);
    auto children = std::vector<TriangleOrNode> { { 0, 0, leftIndex }, { 0, 0, rightIndex } };

    this->nodes.push_back({ false, children, getBox(triangles, *this->m_pScene) });
    return this->nodes.size() - 1;
}

// Return the depth of the tree that you constructed. This is used to tell the
// slider in the UI how many steps it should display for Visual Debug 1.
int BoundingVolumeHierarchy::numLevels() const
{
    return this->m_numLevels;
}

// Return the number of leaf nodes in the tree that you constructed. This is used to tell the
// slider in the UI how many steps it should display for Visual Debug 2.
int BoundingVolumeHierarchy::numLeaves() const
{
    return this->m_numLeaves;
}

// Use this function to visualize your BVH. This is useful for debugging. Use the functions in
// draw.h to draw the various shapes. We have extended the AABB draw functions to support wireframe
// mode, arbitrary colors and transparency.
void BoundingVolumeHierarchy::debugDrawLevel(int level)
{
    // Draw the AABB as a transparent green box.
    // AxisAlignedBox aabb{ glm::vec3(-0.05f), glm::vec3(0.05f, 1.05f, 1.05f) };
    // drawShape(aabb, DrawMode::Filled, glm::vec3(0.0f, 1.0f, 0.0f), 0.2f);

    // Draw the AABB as a (white) wireframe box.
    AxisAlignedBox aabb { glm::vec3(0.0f), glm::vec3(0.0f, 1.05f, 1.05f) };
    // drawAABB(aabb, DrawMode::Wireframe);
    drawAABB(aabb, DrawMode::Filled, glm::vec3(0.05f, 1.0f, 0.05f), 0.1f);
}

// Use this function to visualize your leaf nodes. This is useful for debugging. The function
// receives the leaf node to be draw (think of the ith leaf node). Draw the AABB of the leaf node and all contained triangles.
// You can draw the triangles with different colors. NoteL leafIdx is not the index in the node vector, it is the
// i-th leaf node in the vector.
void BoundingVolumeHierarchy::debugDrawLeaf(int leafIdx)
{
    // Draw the AABB as a transparent green box.
    // AxisAlignedBox aabb{ glm::vec3(-0.05f), glm::vec3(0.05f, 1.05f, 1.05f) };
    // drawShape(aabb, DrawMode::Filled, glm::vec3(0.0f, 1.0f, 0.0f), 0.2f);

    // Draw the AABB as a (white) wireframe box.
    AxisAlignedBox aabb { glm::vec3(0.0f), glm::vec3(0.0f, 1.05f, 1.05f) };
    // drawAABB(aabb, DrawMode::Wireframe);
    drawAABB(aabb, DrawMode::Filled, glm::vec3(0.05f, 1.0f, 0.05f), 0.1f);

    // once you find the leaf node, you can use the function drawTriangle (from draw.h) to draw the contained primitives
}

// Return true if something is hit, returns false otherwise. Only find hits if they are closer than t stored
// in the ray and if the intersection is on the correct side of the origin (the new t >= 0). Replace the code
// by a bounding volume hierarchy acceleration structure as described in the assignment. You can change any
// file you like, including bounding_volume_hierarchy.h.
bool BoundingVolumeHierarchy::intersect(Ray& ray, HitInfo& hitInfo, const Features& features) const
{
    // If BVH is not enabled, use the naive implementation.
    if (!features.enableAccelStructure) {
        bool hit = false;
        // Intersect with all triangles of all meshes.
        for (const auto& mesh : m_pScene->meshes) {
            for (const auto& tri : mesh.triangles) {
                const auto v0 = mesh.vertices[tri[0]];
                const auto v1 = mesh.vertices[tri[1]];
                const auto v2 = mesh.vertices[tri[2]];
                if (intersectRayWithTriangle(v0.position, v1.position, v2.position, ray, hitInfo)) {
                    hitInfo.material = mesh.material;
                    hit = true;
                }
            }
        }
        // Intersect with spheres.
        for (const auto& sphere : m_pScene->spheres)
            hit |= intersectRayWithShape(sphere, ray, hitInfo);
        return hit;
    } else {
        // TODO: implement here the bounding volume hierarchy traversal.
        // Please note that you should use `features.enableNormalInterp` and `features.enableTextureMapping`
        // to isolate the code that is only needed for the normal interpolation and texture mapping features.
        return false;
    }
}