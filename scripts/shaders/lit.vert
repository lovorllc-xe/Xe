#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
layout(push_constant) uniform Push { mat4 mvp; mat4 model; } push;
layout(location = 0) out vec3 vertex_color;
layout(location = 1) out vec3 world_normal;
void main() {
    gl_Position = push.mvp * vec4(position, 1.0);
    vertex_color = color;
    world_normal = mat3(push.model) * normal;
}
