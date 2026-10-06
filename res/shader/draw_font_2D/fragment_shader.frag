#version 450

layout (set = 0, binding = 0) uniform sampler2D tex_sampler;

layout (location = 0) in vec2 in_tex_coord;
layout (location = 1) in vec3 in_color;
layout (location = 2) in flat int bucket_index;
layout (location = 3) in flat int slot_index;
layout (location = 4) in flat int vertex_index;

layout (location = 0) out vec4 out_color;

void main() {
    out_color = texture(tex_sampler, in_tex_coord) * vec4(in_color, 1.f);
}
