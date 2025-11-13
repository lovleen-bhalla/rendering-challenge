#ifndef ANDROIDGLINVESTIGATIONS_RENDERER_H
#define ANDROIDGLINVESTIGATIONS_RENDERER_H

#include <EGL/egl.h>
#include <memory>

#include "Model.h"
#include "Shader.h"
#include "PointCloudFile.h"
#include "Gizmo.h"
#include "glm/glm.hpp"

struct android_app;

class Renderer {
public:
    /*!
     * @param pApp the android_app this Renderer belongs to, needed to configure GL
     */
    inline explicit Renderer(android_app *pApp) :
            app_(pApp),
            display_(EGL_NO_DISPLAY),
            surface_(EGL_NO_SURFACE),
            context_(EGL_NO_CONTEXT),
            width_(0),
            height_(0),
            shaderNeedsNewProjectionMatrix_(true),
            shader_program_(0),
            vao_(0),
            vbo_(0) {
        initRenderer();
    }

    virtual ~Renderer();

    /*!
     * Handles input from the android_app.
     *
     * Note: this will clear the input queue
     */
    void handleInput();

    /*!
     * Renders all the models in the renderer
     */
    void render();

private:
    /*!
     * Performs necessary OpenGL initialization. Customize this if you want to change your EGL
     * context or application-wide settings.
     */
    void initRenderer();

    /*!
     * @brief we have to check every frame to see if the framebuffer has changed in size. If it has,
     * update the viewport accordingly
     */
    void updateRenderArea();

    android_app *app_;
    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;
    EGLint width_;
    EGLint height_;

    bool shaderNeedsNewProjectionMatrix_;

    // Example: Simple triangle rendering
    GLuint shader_program_;
    GLuint vao_;
    GLuint vbo_;

    PointCloudFile *pcf;
    Gizmo *gizmo;
    glm::mat4 model;
    float yaw;
    float pitch;

    glm::vec3 camUp;
    glm::vec3 camPos;
    glm::vec3 camTarget;
    glm::vec3 direction;
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 mvp;

    bool firstTouch = true;
    float lastX = 0.0f;
    float lastY = 0.0f;
    bool updateView = false;
};

#endif //ANDROIDGLINVESTIGATIONS_RENDERER_H