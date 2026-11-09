#version 450
layout(set = 0, binding = 0) uniform Tint { vec4 color; } tint;
layout(set = 0, binding = 1) uniform sampler2D image;
layout(location = 0) in vec2 frag_uv;
layout(location = 0) out vec4 out_color;
void main() {
    out_color = texture(image, frag_uv) * tint.color;
}
