#include "bounding_volume_hierarchy.h"
#include "draw.h"
#include "interpolate.h"
#include "intersect.h"
#include "scene.h"
#include "texture.h"
#include <algorithm>
#include <iostream>
#include <glm/glm.hpp>

void BoundingVolumeHierarchy::setMaxLevels(int level){
    this->m_numLeaves = level;
}

BoundingVolumeHierarchy::BoundingVolumeHierarchy(Scene* pScene)
    : m_pScene(pScene)
{
    // TODO: implement BVH construction.
    this->m_pScene = pScene;
    auto triangles = std::vector<TriangleOrNode>();
    for (size_t i = 0; i < pScene->meshes.size(); ++i) {
        auto mesh = pScene->meshes[i];
        for (size_t j = 0; j < mesh.triangles.size(); ++j) {
            triangles.push_back(TriangleOrNode { i, j });
        }
    }
    
    int maxLevel = 0;
    constructorHelper(triangles, 0, 0);
    setMaxLevels(0);
    // std::cout << this->m_numLeaves << std::endl;
    for (auto node : this->nodes) {
        maxLevel = std::max(maxLevel, node.level);

        // this->m_numLeaves = std::max(this->m_numLeaves, node.level);
        // std::cout << this->m_numLeaves << " " << node.level << ";";
    }
    setMaxLevels(maxLevel);
    std::cout << maxLevel << " " << numLevels() << std::endl;
}

glm::vec3 getMedian(TriangleOrNode triangle, Scene& scene)
{
    auto mesh = scene.meshes[triangle.meshOrNodeIndex];
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
        auto mesh = scene.meshes[triangle.meshOrNodeIndex];
        auto tr = mesh.triangles[triangle.triangleIndex];
        auto v1 = mesh.vertices[tr.x].position, v2 = mesh.vertices[tr.y].position, v3 = mesh.vertices[tr.z].position;
        upper.x = std::max(std::max(upper.x, v1.x), std::max(v2.x, v3.x));
        upper.y = std::max(std::max(upper.y, v1.y), std::max(v2.y, v3.y));
        upper.z = std::max(std::max(upper.z, v1.z), std::max(v2.z, v3.z));

        lower.x = std::min(std::min(lower.x, v1.x), std::min(v2.x, v3.x));
        lower.y = std::min(std::min(lower.y, v1.y), std::min(v2.y, v3.y));
        lower.z = std::min(std::min(lower.z, v1.z), std::min(v2.z, v3.z));
    }
    return { lower, upper };
}
// which axis can work as a depth indicator
size_t BoundingVolumeHierarchy::constructorHelper(std::vector<TriangleOrNode>& triangles, int whichAxis, int level)
{
    
    if (triangles.size() == 1 || level > 8 ) {
        this->nodes.push_back({true, level, triangles, getBox(triangles, *this->m_pScene)});
        this->m_numLeaves += 1;
        return this->nodes.size() - 1;
    }

    std::sort(triangles.begin(), triangles.end(), [this, whichAxis](TriangleOrNode triangle1, TriangleOrNode triangle2) {
        auto median1 = getMedian(triangle1, *this->m_pScene);
        auto median2 = getMedian(triangle2, *this->m_pScene);
        return median1[whichAxis] < median2[whichAxis];
    });

    auto median = triangles.size() / 2;
    std::vector<TriangleOrNode> left(triangles.begin(), triangles.begin() + median);
    std::vector<TriangleOrNode> right(triangles.begin() + median, triangles.end());

    auto leftIndex = this->constructorHelper(left, (whichAxis + 1) % 3, level + 1);
    auto rightIndex = this->constructorHelper(right, (whichAxis + 1) % 3, level + 1);

    // auto level = std::max(this->nodes[leftIndex].level, this->nodes[rightIndex].level) + 1;
    // auto level = whichAxis;
    // std::cout << level << "\n";
    auto children = std::vector<TriangleOrNode> { { leftIndex, 0 }, { rightIndex, 0 } };
    this->nodes.push_back({ false, level, children, getBox(triangles, *this->m_pScene) });
    return this->nodes.size() - 1;
}

// Return the depth of the tree that you constructed. This is used to tell the
// slider in the UI how many steps it should display for Visual Debug 1.
int BoundingVolumeHierarchy::numLevels() const
{
    int maxLevel = 0;
    for (auto node : this->nodes) maxLevel = std::max(maxLevel, node.level);
    return maxLevel;
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

    auto color = glm::vec3(1.0f, 1.0f, 1.0f);
    for (auto& node : this->nodes) {
        // std::cout << node.level << "\n";
        if (node.level == level) {
            // std::cout << node.level << " ";
            // Draw the AABB as a (white) wireframe box.
            // drawAABB(aabb, DrawMode::Wireframe);
            drawAABB(node.box, DrawMode::Wireframe, color, 1.0f);
        }
    }
    // std::cout << std::endl << numLevels() << std::endl;
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
    size_t count = 0;
    for (auto& node : this->nodes) {
        
        if (node.isLeaf == 1) {
            count++;
            if(leafIdx + 1 == count){
                drawAABB(node.box, DrawMode::Wireframe, glm::vec3(0.0f, 1.05f, 1.05f), 1.0f);
            }
        }
    }
    // Draw the AABB as a (white) wireframe box.
    // AxisAlignedBox aabb { glm::vec3(0.0f), glm::vec3(0.0f, 1.05f, 1.05f) };
    // drawAABB(aabb, DrawMode::Wireframe);
    // drawAABB(aabb, DrawMode::Filled, glm::vec3(0.05f, 1.0f, 0.05f), 0.1f);

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