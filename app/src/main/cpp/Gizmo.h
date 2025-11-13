//
// Created by bhalla on 11/11/25.
//

#ifndef RENDERINGCHALLENGE_GIZMO_H
#define RENDERINGCHALLENGE_GIZMO_H

#include <GLES3/gl3.h>
#include "PointCloudData.h"
#include "glm/fwd.hpp"

class Gizmo {
public:
    Gizmo(BoundingBox box);

    void render(glm::mat4 mvp);

private:
    float vertices[36];
    GLuint shaderProgram;
    GLuint vao = 0, vbo = 0;
};

#endif //RENDERINGCHALLENGE_GIZMO_H
