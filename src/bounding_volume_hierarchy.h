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
    size_t meshOrNodeIndex;
    size_t triangleIndex;
};

/**
 * Node struct
 * level - level of the node from bottom of its subtree. i.e. leafs have 0 level
 * triangles - if node is leaf it stores indexes of vertices in the mesh (meshIndex and triangleIndex)
 *     and the second (y) coord for the index of the mesh inside the scene mesh vector.
 *     If node is NOT leaf it uses only the nodeIndex to store indices of the child nodes.
 */
struct Node {
    bool isLeaf = false;
    int level = 0;
    std::vector<TriangleOrNode> triangles;
    AxisAlignedBox box;
};

class BoundingVolumeHierarchy {
public:
    // Constructor. Receives the scene and builds the bounding volume hierarchy.
    BoundingVolumeHierarchy(Scene* pScene);

    // construction helper, returns index of last added node
    size_t constructorHelper(std::vector<TriangleOrNode>& triangles, int whichAxis, int level);

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

    void setMaxLevels(int level);
private:
    int m_numLevels = 0;
    int m_numLeaves;
    Scene* m_pScene;

    std::vector<Node> nodes;
};