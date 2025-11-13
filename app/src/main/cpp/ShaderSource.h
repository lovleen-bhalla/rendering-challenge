//
// Created by bhalla on 11/10/25.
//

#ifndef RENDERINGCHALLENGE_SHADERSOURCE_H
#define RENDERINGCHALLENGE_SHADERSOURCE_H

static const char *pointCloudVertex = R"vertex(#version 300 es
precision highp float;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 ourColor;

uniform mat4 uProjection;

void main()
{
    gl_Position = uProjection * vec4(aPos, 1.0);
    ourColor = aColor;
}
)vertex";

static const char *pointCloudFragment = R"fragment(#version 300 es
precision highp float;

in vec3 ourColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(ourColor, 1.0);
    //FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    //FragColor = vec4(0.0, 0.0, 0.388235, 1.0);
}
)fragment";

// Vertex shader for drawing axis
static const char *gizmoVertex = R"vertex(#version 300 es

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 uProjection;

out vec3 fColor;

void main() {
    fColor = aColor;
    gl_Position = uProjection * vec4(aPos, 1.0);
}
)vertex";

// Fragment shader for drawing axis
static const char *gizmoFragment = R"fragment(#version 300 es

in vec3 fColor;
out vec4 fragColor;

void main() {
    fragColor = vec4(fColor, 1.0);
}
)fragment";


#endif //RENDERINGCHALLENGE_SHADERSOURCE_H
