//
// Created by bhalla on 11/11/25.
//

#include <GLES3/gl3.h>
#include "PointCloudFile.h"
#include "PointCloudData.h"
#include "AndroidOut.h"
#include "ShaderSource.h"

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstring>
#include <cstdlib>
#include <iostream>

struct Plane {
    float a, b, c, d;

    inline void normalize() {
        float len = std::sqrt(a * a + b * b + c * c);
        if (len > 0.0f) {
            a /= len;
            b /= len;
            c /= len;
            d /= len;
        }
    }

    inline float distance(float x, float y, float z) const {
        return a * x + b * y + c * z + d;
    }
};

enum class FrustumResult {
    Outside,
    Intersect,
    Inside
};

inline std::array<Plane, 6> extractFrustumPlanes(const glm::mat4 &mvp) {
    std::array<Plane, 6> planes;

    // m[row][col]
    planes[0] = {mvp[0][3] + mvp[0][0], mvp[1][3] + mvp[1][0], mvp[2][3] + mvp[2][0],
                 mvp[3][3] + mvp[3][0]}; // Left
    planes[1] = {mvp[0][3] - mvp[0][0], mvp[1][3] - mvp[1][0], mvp[2][3] - mvp[2][0],
                 mvp[3][3] - mvp[3][0]}; // Right
    planes[2] = {mvp[0][3] + mvp[0][1], mvp[1][3] + mvp[1][1], mvp[2][3] + mvp[2][1],
                 mvp[3][3] + mvp[3][1]}; // Bottom
    planes[3] = {mvp[0][3] - mvp[0][1], mvp[1][3] - mvp[1][1], mvp[2][3] - mvp[2][1],
                 mvp[3][3] - mvp[3][1]}; // Top
    planes[4] = {mvp[0][3] + mvp[0][2], mvp[1][3] + mvp[1][2], mvp[2][3] + mvp[2][2],
                 mvp[3][3] + mvp[3][2]}; // Near
    planes[5] = {mvp[0][3] - mvp[0][2], mvp[1][3] - mvp[1][2], mvp[2][3] - mvp[2][2],
                 mvp[3][3] - mvp[3][2]}; // Far

    for (auto &p: planes)
        p.normalize();

    return planes;
}

FrustumResult testBoundingBoxFrustum(const BoundingBox &box, const std::array<Plane, 6> &planes) {


    bool intersects = false;

    for (const auto &p: planes) {
        // For each plane, find the most positive vertex
        float x = (p.a >= 0) ? box.max_x : box.min_x;
        float y = (p.b >= 0) ? box.max_y : box.min_y;
        float z = (p.c >= 0) ? box.max_z : box.min_z;

        // If the positive vertex is behind the plane -> completely outside
        if (p.distance(x, y, z) < 0)
            return FrustumResult::Outside;

        // If the negative vertex is behind the plane, we are intersecting
        x = (p.a >= 0) ? box.min_x : box.max_x;
        y = (p.b >= 0) ? box.min_y : box.max_y;
        z = (p.c >= 0) ? box.min_z : box.max_z;
        if (p.distance(x, y, z) < 0)
            intersects = true;
    }

    return intersects ? FrustumResult::Intersect : FrustumResult::Inside;
}

void FindVisibleNodes(OctreeNode *node,
                      std::array<Plane, 6> mvp, std::vector<OctreeNode *> &visible) {
    if (!node)
        return;

    BoundingBox box = node->bounds;

    FrustumResult result = testBoundingBoxFrustum(box, mvp);

    switch (result) {
        case FrustumResult::Inside:
//            aout << "BBox: " << node->bounds.string() << " inside clip space" << std::endl;
            visible.push_back(node);
            return;
            break;
        case FrustumResult::Intersect:
//            aout << "BBox: " << node->bounds.string() << " intersects clip space: isSubdivided: "
//                 << node->subDivided << std::endl;
            if (node->isLeaf) {
                visible.push_back(node);
            } else {
                if (node->subDivided) {
                    for (int i = 0; i < 8; ++i) {
                        FindVisibleNodes(node->children[i].get(), mvp, visible);
                    }
                }
            }

            break;
        case FrustumResult::Outside:
//            aout << "BBox: " << node->bounds.string() << " neither intersects nor inside"
            //<< std::endl;
            break;
    }
}

PointCloudFile::PointCloudFile(AAssetManager *am, const std::string filename) {
    AAsset *pcdFile = AAssetManager_open(
            am,
            filename.c_str(),
            AASSET_MODE_BUFFER
    );

    if (!pcdFile) {
        aout << "Failed to open asset file" << std::endl;
        return;
    }

    off_t length = 0;

    fd = AAsset_openFileDescriptor(pcdFile, &start, &length);
    if (fd < 0) {
        aout << filename << " cannot get fd" << std::endl;
        AAsset_close(pcdFile);
        return;
    }
    lseek(fd, start, SEEK_SET);

    size_t fileSize = static_cast<size_t>(length);
    header = (FileHeader *) malloc(sizeof(FileHeader));
    ssize_t bytes_read = read(fd, header, sizeof(FileHeader));

    if (bytes_read == -1) {
        perror("read");
        return;
    } else if (bytes_read != sizeof(FileHeader)) {
        return;
    }
    if (std::string(header->magic, 7) != "PCLOUD1") {
        aout << "Invalid magic header: " << std::string(header->magic, 7) << "  " << header->magic
             << std::endl;
        return;
    }
    aout << "File header: " << header->string() << std::endl;

    uint32_t numChunks = header->chunk_count;
    auto chunksMetadataSize = numChunks * sizeof(ChunkMetadata);
    // mmap the asset contents


    long pageSize = sysconf(_SC_PAGESIZE);
    off_t aligned_start = (start / pageSize) * pageSize;
    auto offset = start - aligned_start;
    auto adjusted_length = offset + sizeof(FileHeader) + chunksMetadataSize;

    void *mappedPtr = mmap(
            nullptr,
            adjusted_length,
            PROT_READ,
            MAP_PRIVATE,
            fd,
            aligned_start
    );

    if (mappedPtr == MAP_FAILED) {
        aout << "mmap failing " << start << "  " << pageSize << " " << start % pageSize << start <<
             errno
             << std::endl;
        aout << "mmap failed!! " << errno << std::endl;
        perror("mmap");
        AAsset_close(pcdFile);
        close(fd);
        return;
    }

    chunks = (ChunkMetadata *) malloc(sizeof(ChunkMetadata) * numChunks);
    memcpy(chunks, (char *) mappedPtr + offset + sizeof(FileHeader), chunksMetadataSize);
    munmap(mappedPtr, adjusted_length);
    AAsset_close(pcdFile);

    // Build octree
    o = new OctreeNode(header->bounds);
    for (uint32_t i = 0; i < numChunks; i++) {
        aout << "inserting chunk: " << i << "  " << chunks[i].bbox.string() << std::endl;
        o->insert(chunks[i].bbox, i);
    }

    auto compileShader = [](GLenum type, const char *src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char buf[512];
            glGetShaderInfoLog(s, 512, nullptr, buf);
            aout << "Shader compile error: %s\n" << buf << std::endl;
        }
        return s;
    };
    GLuint vs = compileShader(GL_VERTEX_SHADER, pointCloudVertex);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, pointCloudFragment);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    GLint linkStatus;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkStatus);
    if (!linkStatus) {
        char buf[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, buf);
        aout << "Shader link error: %s\n" << buf << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void *) offsetof(Point, x));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Point),
                          (void *) offsetof(Point, r));
    glBindVertexArray(0);

}

void PointCloudFile::GetChunk(int id, uint64_t &start, uint64_t &end) {
    ChunkMetadata chunk = chunks[id];
    uint32_t pointCount = chunk.point_count;
    start = chunks->file_offset;
    end = chunk.file_offset + (sizeof(ChunkMetadata) * pointCount);
}

BoundingBox PointCloudFile::getBounds() {
    return header->bounds;
}

uint64_t PointCloudFile::pointCount() {
    return header->total_points;
}

uint32_t PointCloudFile::chunkCount() {
    return header->chunk_count;
};

void PointCloudFile::render(glm::mat4 mvp, bool viewUpdated) {
    if (viewUpdated) {
        std::vector<OctreeNode *> visibleNodes;
        auto planes = extractFrustumPlanes(mvp);
        FindVisibleNodes(o, planes, visibleNodes);
        std::vector<int> ids;
        int totalPoints = 0;
        for (auto n: visibleNodes) {
            n->collectLeaves(ids);
        }
        for (int id: ids) {
            int point_count = chunks[id].point_count;
            totalPoints = totalPoints + point_count;
        }
        pointsToRender = totalPoints;
        aout << "Rendering: " << pointsToRender << " points." << std::endl;
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Point) * totalPoints, nullptr,
                     GL_STREAM_DRAW);

        long pageSize = sysconf(_SC_PAGESIZE);
        int currOffset = 0;
        for (int id: ids) {
            ChunkMetadata chunk = chunks[id];
            auto chunkOffset = chunk.file_offset;

            auto chunkStart = chunkOffset + start;
            auto aligned_start = (chunkStart / pageSize) * pageSize;
            auto offset = chunkStart - aligned_start;
            auto dataSize = chunk.point_count * sizeof(Point);
            auto adjusted_length = offset + dataSize;

            void *mappedPtr = mmap(
                    nullptr,
                    adjusted_length,
                    PROT_READ,
                    MAP_PRIVATE,
                    fd,
                    aligned_start
            );

            if (mappedPtr == MAP_FAILED) {
                aout << "mmap failing chunk" << id << "  " << "" << pageSize << " " << errno << "  "<< start % pageSize
                     << start <<
                     errno
                     << std::endl;
                aout << "mmap failed!! " << errno << std::endl;
                perror("mmap");
                continue;
            }

            glBufferSubData(GL_ARRAY_BUFFER, currOffset, dataSize, (char *) mappedPtr + offset );
            munmap(mappedPtr, dataSize);
            currOffset = currOffset + dataSize;
        }
    }

    glUseProgram(shaderProgram);
    GLint location = glGetUniformLocation(shaderProgram, "uProjection");
    glUniformMatrix4fv(location, 1, GL_FALSE, &(mvp[0].x));
    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, pointsToRender);
    glBindVertexArray(0);
}

PointCloudFile::~PointCloudFile() {
    if (chunks) {
        free(chunks);
    }
}