#include "bounding_volume_hierarchy.h"
#include "draw.h"
#include "interpolate.h"
#include "intersect.h"
#include "scene.h"
#include "texture.h"
#include <algorithm>
#include <deque>
#include <glm/glm.hpp>
#include <iostream>

BoundingVolumeHierarchy::BoundingVolumeHierarchy(Scene* pScene, const Features& features)
    : m_pScene(pScene)
{
    this->m_pScene = pScene;
    auto triangles = std::vector<TriangleOrNode>();
    for (size_t i = 0; i < pScene->meshes.size(); ++i) {
        const auto& mesh = pScene->meshes[i];
        for (size_t j = 0; j < mesh.triangles.size(); ++j) {
            triangles.push_back(TriangleOrNode { i, j });
        }
    }
    if (features.extra.enableBvhSahBinning) {
        sahConstructorHelper(triangles, 0, triangles.size(), 0, 0);
    } else {
        constructorHelper(triangles, 0, triangles.size(), 0, 0);
    }

    this->m_numLevels = 0;
    for (const auto& node : this->nodes) {
        this->m_numLevels = std::max(this->m_numLevels, node.level + 1);
    }
}

glm::vec3 getMedian(const TriangleOrNode& triangle, const Scene& scene)
{
    const auto& mesh = scene.meshes[triangle.meshIndex];
    const auto& tr = mesh.triangles[triangle.triangleIndex]; // uvec3, indices of points of a single triangle
    const auto& p1 = mesh.vertices[tr.x].position;
    const auto& p2 = mesh.vertices[tr.y].position;
    const auto& p3 = mesh.vertices[tr.z].position;
    return (p1 + p2 + p3) / 3.0f;
}

AxisAlignedBox getBox(
    const std::vector<TriangleOrNode>::iterator begin,
    const std::vector<TriangleOrNode>::iterator end,
    const Scene& scene)
{
    glm::vec3 lower = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
    glm::vec3 upper = { std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };
    for (auto it = begin; it != end; it += 1) {
        auto triangle = *it;
        const auto& mesh = scene.meshes[triangle.meshIndex];
        const auto& tr = mesh.triangles[triangle.triangleIndex];
        const auto &v1 = mesh.vertices[tr.x].position, v2 = mesh.vertices[tr.y].position, v3 = mesh.vertices[tr.z].position;
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
size_t BoundingVolumeHierarchy::constructorHelper(std::vector<TriangleOrNode>& triangles, size_t left, size_t right, int whichAxis, int level)
{
    const auto beginIt = triangles.begin() + left;
    const auto endIt = triangles.begin() + right;

    if (right - left <= 1 || level > 16) {
        this->nodes.push_back({ true, level, std::vector<TriangleOrNode>(beginIt, endIt), getBox(beginIt, endIt, *this->m_pScene) });
        this->m_numLeaves += 1;
        return this->nodes.size() - 1;
    }

    std::sort(triangles.begin() + left, triangles.begin() + right, [this, whichAxis](const TriangleOrNode& triangle1, const TriangleOrNode& triangle2) {
        const auto median1 = getMedian(triangle1, *this->m_pScene);
        const auto median2 = getMedian(triangle2, *this->m_pScene);
        return median1[whichAxis] < median2[whichAxis];
    });

    size_t median = (left + right + 1) / 2;
    const auto leftIndex = this->constructorHelper(triangles, left, median, (whichAxis + 1) % 3, level + 1);
    const auto rightIndex = this->constructorHelper(triangles, median, right, (whichAxis + 1) % 3, level + 1);

    const auto children = std::vector<TriangleOrNode> { { 0, 0, leftIndex }, { 0, 0, rightIndex } };
    this->nodes.push_back({ false, level, children, getBox(beginIt, endIt, *this->m_pScene) });
    return this->nodes.size() - 1;
}

float getProb(const AxisAlignedBox& inner, const AxisAlignedBox& outer)
{
    float innerA = std::abs(inner.lower.x - inner.upper.x);
    float innerB = std::abs(inner.lower.y - inner.upper.y);
    float innerC = std::abs(inner.lower.z - inner.upper.z);

    float outerA = std::abs(outer.lower.x - outer.upper.x);
    float outerB = std::abs(outer.lower.y - outer.upper.y);
    float outerC = std::abs(outer.lower.z - outer.upper.z);

    float areaInner = 2 * (innerA * innerB + innerB * innerC + innerC * innerA);
    float areaOuter = 2 * (outerA * outerB + outerB * outerC + outerC * outerA);

    return areaInner / areaInner;
}

// cost calculation function
std::pair<float, std::vector<TriangleOrNode>::iterator> calculateCostOfDivision(
    const std::vector<TriangleOrNode>::iterator begin,
    const std::vector<TriangleOrNode>::iterator end,
    const Scene& scene,
    float boundary,
    int whichAxis)
{
    auto middle = begin;
    while (getMedian(*middle, scene)[whichAxis] < boundary && middle != end)
        ++middle;
    auto outerBox = getBox(begin, end, scene);
    auto leftBox = getBox(begin, middle, scene);
    auto rightBox = getBox(middle, end, scene);
    return { getProb(leftBox, outerBox) * (middle - begin) + getProb(rightBox, outerBox) * (end - middle), middle };
}

// which axis can work as a depth indicator
size_t BoundingVolumeHierarchy::sahConstructorHelper(std::vector<TriangleOrNode>& triangles, size_t left, size_t right, int whichAxis, int level)
{
    const auto beginIt = triangles.begin() + left;
    const auto endIt = triangles.begin() + right;

    if (right - left <= 1 || level > 16) {
        this->nodes.push_back({ true, level, std::vector<TriangleOrNode>(beginIt, endIt), getBox(beginIt, endIt, *this->m_pScene) });
        this->m_numLeaves += 1;
        return this->nodes.size() - 1;
    }

    std::sort(triangles.begin() + left, triangles.begin() + right, [this, whichAxis](const TriangleOrNode& triangle1, const TriangleOrNode& triangle2) {
        const auto median1 = getMedian(triangle1, *this->m_pScene);
        const auto median2 = getMedian(triangle2, *this->m_pScene);
        return median1[whichAxis] < median2[whichAxis];
    });

    const int binsNumber = std::min(5UL, right - left - 1);
    const auto leftBoundary = getMedian(*beginIt, *this->m_pScene)[whichAxis];
    const auto rightBoundary = getMedian(*(endIt - 1), *this->m_pScene)[whichAxis];
    const size_t step = (leftBoundary - rightBoundary) / binsNumber;
    float minCost = std::numeric_limits<float>::lowest();
    size_t median = left;
    for (size_t i = 1; i < binsNumber; ++i) {
        const auto boundary = leftBoundary + step * i;
        const auto p = calculateCostOfDivision(beginIt, endIt, *this->m_pScene, boundary, whichAxis);
        const auto cost = p.first;
        if (cost < minCost) {
            minCost = cost;
            median = p.second - triangles.begin();
        }
    }

    const auto leftIndex = this->constructorHelper(triangles, left, median, (whichAxis + 1) % 3, level + 1);
    const auto rightIndex = this->constructorHelper(triangles, median, right, (whichAxis + 1) % 3, level + 1);

    const auto children = std::vector<TriangleOrNode> { { 0, 0, leftIndex }, { 0, 0, rightIndex } };
    this->nodes.push_back({ false, level, children, getBox(beginIt, endIt, *this->m_pScene) });
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

    const auto color = glm::vec3(1.0f, 1.0f, 1.0f);
    for (const auto& node : this->nodes) {
        if (node.level == level) {
            // Draw the AABB as a (white) wireframe box.
            // drawAABB(aabb, DrawMode::Wireframe);
            drawAABB(node.box, DrawMode::Wireframe, color, 1.0f);
        }
    }
}

void drawLeafTriangle(const glm::uvec3& triangle, const Mesh& mesh, const glm::vec3& color)
{
    const auto& v0 = mesh.vertices[triangle[0]];
    const auto& v1 = mesh.vertices[triangle[1]];
    const auto& v2 = mesh.vertices[triangle[2]];
    drawTriangle(v0, v1, v2, color);
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
    size_t leafCounter = 0;
    for (size_t i = 0; i <= this->nodes.size(); ++i) {
        if (this->nodes[i].isLeaf)
            ++leafCounter;
        if (this->nodes[i].isLeaf && leafCounter == leafIdx) {
            drawAABB(this->nodes[i].box, DrawMode::Wireframe, glm::vec3(0.0f, 1.05f, 1.05f), 1.0f);
            for (const auto& triangle : this->nodes[i].triangles) {
                const auto& mesh = this->m_pScene->meshes[triangle.meshIndex];
                const auto& tr = mesh.triangles[triangle.triangleIndex];
                drawLeafTriangle(tr, mesh, { 1.0f, 1.0f, 0.0f });
            }
        }
    }
    // Draw the AABB as a (white) wireframe box.
    // AxisAlignedBox aabb { glm::vec3(0.0f), glm::vec3(0.0f, 1.05f, 1.05f) };
    // drawAABB(aabb, DrawMode::Wireframe);
    // drawAABB(aabb, DrawMode::Filled, glm::vec3(0.05f, 1.0f, 0.05f), 0.1f);

    // once you find the leaf node, you can use the function drawTriangle (from draw.h) to draw the contained primitives
}

bool shoudlBeReverted(Ray& ray, HitInfo& hitInfo)
{
    auto p = ray.origin + ray.direction * ray.t;
    auto pointToCamera = glm::normalize(ray.origin - p);
    if (glm::dot(hitInfo.normal, pointToCamera) <= 0) {
        return true;
    }
    return false;
}

bool intersectWithLeafTriangle(Ray& ray, HitInfo& hitInfo, glm::uvec3 triangle, const Mesh& mesh, const Features& features)
{
    const auto& v0 = mesh.vertices[triangle[0]];
    const auto& v1 = mesh.vertices[triangle[1]];
    const auto& v2 = mesh.vertices[triangle[2]];
    if (intersectRayWithTriangle(v0.position, v1.position, v2.position, ray, hitInfo)) {
        hitInfo.material = mesh.material;
        hitInfo.normal = glm::normalize(glm::cross(v1.position - v0.position, v2.position - v0.position));

        if (features.enableNormalInterp) {
            hitInfo.barycentricCoord = computeBarycentricCoord(v0.position, v1.position, v2.position, ray.origin + ray.direction * ray.t);
            hitInfo.normal = glm::normalize(interpolateNormal(v0.normal, v1.normal, v2.normal, hitInfo.barycentricCoord));
        }
        hitInfo.normal = shoudlBeReverted(ray, hitInfo) ? -hitInfo.normal : hitInfo.normal;

        if (features.enableTextureMapping) {
            hitInfo.texCoord = interpolateTexCoord(v0.texCoord, v1.texCoord, v2.texCoord, hitInfo.barycentricCoord);
            if (hitInfo.material.kdTexture) {
                hitInfo.material.kd = acquireTexel(hitInfo.material.kdTexture.operator*(), hitInfo.texCoord, features);
            }
        }

        return true;
    }
    return false;
}

float getClosestIntersectionWithBox(const AxisAlignedBox& box, const Ray& ray)
{
    Ray newRay = { ray.origin, ray.direction, ray.t };
    intersectRayWithShape(box, newRay);
    return newRay.t;
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
                if (intersectWithLeafTriangle(ray, hitInfo, tri, mesh, features)) {
                    hit = true;
                }
            }
        }
        // Intersect with spheres.
        for (const auto& sphere : m_pScene->spheres)
            hit |= intersectRayWithShape(sphere, ray, hitInfo);
        return hit;
    } else {
        // Please note that you should use `features.enableNormalInterp` and `features.enableTextureMapping`
        // to isolate the code that is only needed for the normal interpolation and texture mapping features.
        if (this->nodes.empty())
            return false;
        float closestIntersection = std::numeric_limits<float>::max();

        std::deque<size_t> deque;
        auto lower = nodes[0].box.lower;
        auto upper = nodes[0].box.upper;

        if (getClosestIntersectionWithBox(this->nodes.back().box, ray) < closestIntersection) {
            deque.push_back(nodes.size() - 1);
        }

        glm::uvec3 intersectionTriangle = {};
        size_t meshInd = -1;
        while (!deque.empty()) {
            const auto& next = this->nodes[deque.back()];
            deque.pop_back();

            if (getClosestIntersectionWithBox(next.box, ray) > closestIntersection) {
                continue;
            }
            drawAABB(next.box, DrawMode::Wireframe, glm::vec3(1.0f, 1.00f, 1.0f), 1.0f);

            if (next.isLeaf) {
                for (const auto& triangle : next.triangles) {
                    const auto& mesh = this->m_pScene->meshes[triangle.meshIndex];
                    const auto& tr = mesh.triangles[triangle.triangleIndex];
                    if (intersectWithLeafTriangle(ray, hitInfo, tr, mesh, features)) {
                        closestIntersection = std::min(closestIntersection, ray.t);
                        intersectionTriangle = tr;
                        meshInd = triangle.meshIndex;
                    }
                }
            } else {
                for (const auto& nodeIndex : next.triangles) {
                    auto index = nodeIndex.nodeIndex;
                    const auto& child = this->nodes[index];
                    float closest = getClosestIntersectionWithBox(child.box, ray);

                    if (closest < closestIntersection) {
                        deque.push_back(index);
                    } else if (closest < std::numeric_limits<float>::max() && features.debugOptimisedNodes) {
                        // draw unvisited inteersected node
                        drawAABB(next.box, DrawMode::Wireframe, glm::vec3(1.0f, 0.00f, 0.0f), 0.1f);
                    }
                }
            }
        }
        const auto intersectionHappened = closestIntersection != std::numeric_limits<float>::max();
        if (intersectionHappened) {
            drawLeafTriangle(intersectionTriangle, this->m_pScene->meshes[meshInd], { 0.0f, 1.0f, 0.0f });
        }
        return intersectionHappened;
    }
}