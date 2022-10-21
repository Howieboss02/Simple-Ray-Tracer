#pragma once
#include "common.h"
#include <array>
#include <framework/ray.h>
#include <vector>

// Forward declaration.
struct Scene;

/**
 * meshIndex - index of the mesh inside mesh vector
 * triangleIndex - index of triangle inside this mesh
 * nodeIndex - index of node inside BVH node vector
 */
struct TriangleOrNode {
    size_t meshIndex;
    size_t triangleIndex;
    size_t nodeIndex;
};

/**
 * Node struct
 * triangles - if node is leaf it stores indexes of vertices in the mesh (meshIndex and triangleIndex)
 *     and the second (y) coord for the index of the mesh inside the scene mesh vector.
 *     If node is NOT leaf it uses only the nodeIndex to store indices of the child nodes.
 */
struct Node {
    size_t depth = 0;
    std::vector<TriangleOrNode> triangles;
    AxisAlignedBox box;
    bool isLeaf()
    {
        return this->depth == 0;
    }
};

class BoundingVolumeHierarchy {
public:
    // Constructor. Receives the scene and builds the bounding volume hierarchy.
    BoundingVolumeHierarchy(Scene* pScene);

    // construction helper, returns index of last added node
    size_t constructorHelper(std::vector<TriangleOrNode>& triangles, int whichAxis);

    // Return how many levels there are in the tree that you have constructed.
    [[nodiscard]] int numLevels() const;

    // Return how many leaf nodes there are in the tree that you have constructed.
    [[nodiscard]] int numLeaves() const;

    // Visual Debug 1: Draw the bounding boxes of the nodes at the selected level.
    void debugDrawLevel(int level);

    // Visual Debug 2: Draw the triangles of the i-th leaf
    void debugDrawLeaf(int leafIdx);

    // Return true if something is hit, returns false otherwise.
    // Only find hits if they are closer than t stored in the ray and the intersection
    // is on the correct side of the origin (the new t >= 0).
    bool intersect(Ray& ray, HitInfo& hitInfo, const Features& features) const;

private:
    int m_numLevels;
    int m_numLeaves;
    Scene* m_pScene;

    std::vector<Node> nodes;
};