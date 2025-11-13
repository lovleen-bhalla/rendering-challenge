//
// Created by bhalla on 11/11/25.
//
#include "Gizmo.h"
#include <glm/glm.hpp>
#include <GLES3/gl3.h>
#include "ShaderSource.h"
#include "AndroidOut.h"

void printMat4(const glm::mat4 &m, std::string name) {
    aout << name << "Matrix:\n";
    aout << std::fixed << std::setprecision(3); // format numbers nicely
    for (int row = 0; row < 4; ++row) {
        aout << "[ ";
        for (int col = 0; col < 4; ++col) {
            aout << std::setw(8) << m[col][row] << " "; // column-major order
        }
        aout << "]\n";
    }
}

Gizmo::Gizmo(BoundingBox box) {
    float cx, cy, cz;
    box.getCenter(cx, cy, cz);
    float values[] = {
            // X axis Red
            box.min_x, cy, cz, 100, 0, 0,
            box.max_x, cy, cz, 100, 0, 0,

            // Y axis Green
            cx, box.min_y, cz, 0, 100, 0,
            cx, box.max_y, cz, 0, 100, 0,

            // Z axis Blue
            cx, cy, box.min_z, 0, 0, 100,
            cx, cy, box.max_z, 0, 0, 100
    };

    memcpy(vertices, values, sizeof(values));
    const char *vsSrc = gizmoVertex;
    const char *fsSrc = gizmoFragment;
    auto compileShader = [](GLenum type, const char *src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char buf[512];
            glGetShaderInfoLog(s, 512, nullptr, buf);
            printf("Shader compile error: %s\n", buf);
        }
        return s;
    };

    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    GLint linkStatus;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkStatus);
    if (!linkStatus) {
        char buf[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, buf);
        printf("Shader link error: %s\n", buf);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(values), values, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) 0);


    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void *) (3 * sizeof(float)));

    glBindVertexArray(0);
}

void Gizmo::render(glm::mat4 mvp) {
    glUseProgram(shaderProgram);
    GLint location = glGetUniformLocation(shaderProgram, "uProjection");
    glUniformMatrix4fv(location, 1, GL_FALSE, &(mvp[0].x));
    glBindVertexArray(vao);
    glDrawArrays(GL_LINES, 0, 6);
    glBindVertexArray(0);
}