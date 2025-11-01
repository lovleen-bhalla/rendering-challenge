//
// Created by bhalla on 10/22/25.
//
#include <cstdint>

#ifndef RENDERINGCHALLENGE_POINTCLOUDDATA_H
#define RENDERINGCHALLENGE_POINTCLOUDDATA_H

struct Point {
    float x;
    float y;
    float z;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t padding;
};

struct BoundingBox {
    float min_x;
    float min_y;
    float min_z;
    float max_x;
    float max_y;
    float max_z;

    bool contains(float x, float y, float z) const {
        return x >= min_x && x <= max_x &&
               y >= min_y && y <= max_y &&
               z >= min_z && z <= max_z;
    }

    void getCenter(float &cx, float &cy, float &cz) const {
        cx = (min_x + max_x) * 0.5f;
        cy = (min_y + max_y) * 0.5f;
        cz = (min_z + max_z) * 0.5f;
    }
};

struct FileHeader {
    char magic[8];            // Magic number: "PCLOUD1\0"
    uint32_t version;         // Version: 1
    BoundingBox bounds;       // Global bounding box: min_x, min_y, min_z, max_x, max_y, max_z
    uint64_t total_points;    // Total points
    uint32_t chunk_count;     // Chunk count
    uint32_t chunk_size;      // Chunk size

    uint64_t size() {
        return total_points;
    }
};

struct ChunkMetadata {
    uint32_t point_count;
    uint64_t file_offset;
    BoundingBox bbox;

    uint64_t size() {
        return point_count;
    }
};

#endif //RENDERINGCHALLENGE_POINTCLOUDDATA_H
