//
// Created by bhalla on 10/22/25.
//
#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>

#ifndef RENDERINGCHALLENGE_POINTCLOUDDATA_H
#define RENDERINGCHALLENGE_POINTCLOUDDATA_H

struct Point {
    float x, y, z;
    uint8_t r, g, b;
    uint8_t padding;

    std::string string() const {
        std::stringstream ss;
        ss << "x: " << x << ", y: " << y << ", z: " << z;
        ss << ", r: " << (int) r << ", g: " << (int) g << ", b: " << (int) b;
        return ss.str();
    }
};

struct BoundingBox {
    float min_x, min_y, min_z;
    float max_x, max_y, max_z;

    [[nodiscard]] inline bool contains(float x, float y, float z) const {
        return x >= min_x && x <= max_x &&
               y >= min_y && y <= max_y &&
               z >= min_z && z <= max_z;
    }

    [[nodiscard]] inline float size() const {
        float dx = max_x - min_x;
        float dy = max_y - min_y;
        float dz = max_z - min_z;
        return dx > dy ? (dx > dz ? dx : dz) : (dy > dz ? dy : dz);
    }

    [[nodiscard]] inline float maxDimension() const {
        float dx = max_x - min_x;
        float dy = max_y - min_y;
        float dz = max_z - min_z;
        return dx > dy ? (dx > dz ? dx : dz) : (dy > dz ? dy : dz);
    }

    inline void getCenter(float &cx, float &cy, float &cz) const {
        cx = (min_x + max_x) * 0.5f;
        cy = (min_y + max_y) * 0.5f;
        cz = (min_z + max_z) * 0.5f;
    }

    inline std::string string() {

        std::stringstream ss;
        ss << "BoundingBox(min=(" << std::fixed << std::setprecision(2) << min_x << ", " << min_y
           << ", " << min_z << "), "
           << "max=(" << std::fixed << std::setprecision(2) << max_x << ", " << max_y << ", "
           << max_z << "))";
        return ss.str();
    }
};

struct FileHeader {
    char magic[8];         // "PCLOUD1\0"
    uint32_t version;      // Format version
    BoundingBox bounds;    // Overall bounds
    uint64_t total_points; // Total number of points
    uint32_t chunk_count;  // Number of chunks
    uint32_t chunk_size;   // Target points per chunk

    std::string string() const {
        std::stringstream ss;
        ss << "Magic: " << magic << std::endl;
        ss << "Version: " << version << std::endl;
        ss << "Bounds: " << bounds.min_x << ", " << bounds.min_y << ", " << bounds.min_z << " -to- "
           << bounds.max_x << ", " << bounds.max_y << ", " << bounds.max_z << std::endl;
        ss << "Total Points: " << total_points << std::endl;
        ss << "Chunk Count: " << chunk_count << std::endl;
        ss << "Chunk Size: " << chunk_size << std::endl;
        return ss.str();
    }
};

struct ChunkMetadata {
    BoundingBox bbox;
    uint32_t point_count;
    uint64_t file_offset;

    std::string string() const {
        std::stringstream ss;
        ss << "Bounds: " << bbox.min_x << ", " << bbox.min_y << ", " << bbox.min_z << " -to-  "
           << bbox.max_x << ", " << bbox.max_y << ", " << bbox.max_z << std::endl;
        ss << "Points : " << point_count << std::endl;
        ss << "File offset: " << file_offset << std::endl;
        return ss.str();
    }
};

#endif //RENDERINGCHALLENGE_POINTCLOUDDATA_H
