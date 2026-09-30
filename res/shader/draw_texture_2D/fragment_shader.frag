#version 450

layout (set = 1, binding = 0) uniform sampler2D tex_sampler;
layout (set = 2, binding = 0) uniform sampler2DArray bucket_tex_samplers[7];

layout (location = 0) in vec2 in_tex_coord;
layout (location = 1) in flat int bucket_index;
layout (location = 2) in flat int slot_index;

layout (location = 0) out vec4 out_color;

void main() {
    if (bucket_index < 0) {
        out_color = texture(tex_sampler, in_tex_coord);
    }
    else {
        out_color = texture(bucket_tex_samplers[bucket_index], vec3(in_tex_coord, float(slot_index)));
    }
}
