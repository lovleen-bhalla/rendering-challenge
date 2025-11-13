//
// Created by bhalla on 11/10/25.
//
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "AndroidOut.h"
#include "PointCloudData.h"

#ifndef RENDERINGCHALLENGE_OCTREE_H
#define RENDERINGCHALLENGE_OCTREE_H

// We use this to rebuild the Octree from chunks stored in the file.
// The reason for rebuilding octree is frustum culling is more efficient
struct OctreeNode {
    BoundingBox bounds;
    bool isLeaf = false;
    std::unique_ptr<OctreeNode> children[8];
    bool subDivided = false;
    int id = -1;

    OctreeNode(const BoundingBox &b) : bounds(b) {}

    /// Subdivide a node into 8 equal children
    void subdivide(const BoundingBox &bbox) {
        if (subDivided) {
            return;
        }

        float mid_x = (bbox.min_x + bbox.max_x) / 2.0f;
        float mid_y = (bbox.min_y + bbox.max_y) / 2.0f;
        float mid_z = (bbox.min_z + bbox.max_z) / 2.0f;

        BoundingBox child_boxes[8] = {
                {bbox.min_x, bbox.min_y, bbox.min_z, mid_x,      mid_y,      mid_z}, // 0
                {mid_x,      bbox.min_y, bbox.min_z, bbox.max_x, mid_y,      mid_z}, // 1
                {bbox.min_x, mid_y,      bbox.min_z, mid_x,      bbox.max_y, mid_z}, // 2
                {mid_x,      mid_y,      bbox.min_z, bbox.max_x, bbox.max_y, mid_z}, // 3
                {bbox.min_x, bbox.min_y, mid_z,      mid_x,      mid_y,      bbox.max_z}, // 4
                {mid_x,      bbox.min_y, mid_z,      bbox.max_x, mid_y,      bbox.max_z}, // 5
                {bbox.min_x, mid_y,      mid_z,      mid_x,      bbox.max_y, bbox.max_z}, // 6
                {mid_x,      mid_y,      mid_z,      bbox.max_x, bbox.max_y, bbox.max_z}  // 7
        };
        for (int i = 0; i < 8; ++i) {
            children[i] = std::make_unique<OctreeNode>(child_boxes[i]);
        }
        subDivided = true;
    }

    int getOctant(float x, float y, float z) const {
        float mid_x = (bounds.min_x + bounds.max_x) / 2.0f;
        float mid_y = (bounds.min_y + bounds.max_y) / 2.0f;
        float mid_z = (bounds.min_z + bounds.max_z) / 2.0f;

        int octant = 0;
        if (x >= mid_x)
            octant |= 1;
        if (y >= mid_y)
            octant |= 2;
        if (z >= mid_z)
            octant |= 4;

        return octant;
    }

    void insert(BoundingBox child, int chunkIndex) {
        // std::cout << "insert: " << child.string() << "into: " << bounds.string() << std::endl;
        float child_length_x = std::abs(child.max_x - child.min_x);
        float child_length_y = std::abs(child.max_y - child.min_y);
        float child_length_z = std::abs(child.max_z - child.min_z);

        float parent_length_x = std::abs(bounds.max_x - bounds.min_x);
        float parent_length_y = std::abs(bounds.max_y - bounds.min_y);
        float parent_length_z = std::abs(bounds.max_z - bounds.min_z);

        if (child_length_x > parent_length_x / 2 ||
            child_length_y > parent_length_y / 2 ||
            child_length_z > parent_length_z / 2) {
            isLeaf = true;
            id = chunkIndex;
            return;
        }

        float childMidX, childMidY, childMidZ;
        child.getCenter(childMidX, childMidY, childMidZ);
        subdivide(bounds);
        int octant = getOctant(childMidX, childMidY, childMidZ);

        // std::cout << "right octant index" << octant << " : " << children[octant]->bounds.string() << std::endl;
        float length_x = std::abs(child.max_x - child.min_x);
        float length_y = std::abs(child.max_y - child.min_y);
        float length_z = std::abs(child.max_z - child.min_z);
        children[octant]->insert(child, chunkIndex);
        // If octant is same is child voila we have the node or else insert in the octant
    }

    // Add this method inside the OctreeNode struct
    std::string toString(int indent = 0) const {

        std::ostringstream oss;
        std::string indent_str(indent * 2, ' ');
        std::string child_indent_str((indent + 1) * 2, ' ');

        // Node header
        oss << indent_str << (isLeaf ? "LEAF" : "NODE")
            << " | Points: " << 0
            << " | BBox: ["
            << std::fixed << std::setprecision(2)
            << bounds.min_x << "," << bounds.min_y << "," << bounds.min_z << " -> "
            << bounds.max_x << "," << bounds.max_y << "," << bounds.max_z
            << "]\n";

        if (isLeaf) {

        } else {
            for (int i = 0; i < 8; ++i) {
                if (children[i]) {
                    oss << child_indent_str << "Child " << i << ":";
                    oss << children[i]->toString(indent + 2);
                }
            }
        }

        return oss.str();
    }

    void collectLeaves(std::vector<int> &ids) const {
        if (isLeaf && id != -1) {
            ids.push_back(id);
        } else {
            for (int i = 0; i < 8; ++i) {
                if (children[i]) {
                    children[i]->collectLeaves(ids);
                }
            }
        }
    }
};

#endif //RENDERINGCHALLENGE_OCTREE_H
