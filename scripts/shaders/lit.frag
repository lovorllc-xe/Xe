#version 450
layout(location = 0) in vec3 vertex_color;
layout(location = 1) in vec3 world_normal;
layout(location = 0) out vec4 out_color;
void main() {
    vec3 light = normalize(vec3(0.4, 0.8, 0.5));
    float diffuse = max(dot(normalize(world_normal), light), 0.0);
    out_color = vec4(vertex_color * (0.25 + 0.75 * diffuse), 1.0);
}
