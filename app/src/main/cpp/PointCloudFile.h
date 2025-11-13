//
// Created by bhalla on 11/11/25.
//

#ifndef RENDERINGCHALLENGE_POINTCLOUDFILE_H
#define RENDERINGCHALLENGE_POINTCLOUDFILE_H

#include <string>
#include <android/asset_manager.h>
#include "PointCloudData.h"
#include "PointCloudFile.h"
#include "Octree.h"
#include "glm/glm.hpp"

class PointCloudFile {
public:
    PointCloudFile(AAssetManager *assetManager, std::string filename);

    void GetChunk(int id, uint64_t &start, uint64_t &end);

    void *getData();

    BoundingBox getBounds();

    uint64_t pointCount();

    uint32_t chunkCount();

    void render(glm::mat4 mvp, bool viewUpdated);

    virtual ~PointCloudFile();

private:
    void *data;
    FileHeader *header;
    ChunkMetadata *chunks;
    OctreeNode *o;

    std::vector<float> vertices;
    GLuint shaderProgram;
    GLuint vao;
    GLuint vbo;
    int pointsToRender;
};

#endif //RENDERINGCHALLENGE_POINTCLOUDFILE_H
