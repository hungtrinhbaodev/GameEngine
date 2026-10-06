#version 450

layout (set = 0, binding = 0) uniform sampler2D tex_sampler;
layout (set = 1, binding = 0) uniform sampler2DArray bucket_tex_sampler_0;
layout (set = 1, binding = 1) uniform sampler2DArray bucket_tex_sampler_1;
layout (set = 1, binding = 2) uniform sampler2DArray bucket_tex_sampler_2;

layout (location = 0) in vec2 in_tex_coord;
layout (location = 1) in vec3 in_color;
layout (location = 2) in flat int bucket_index;
layout (location = 3) in flat int slot_index;
layout (location = 4) in flat int vertex_index;

layout (location = 0) out vec4 out_color;

vec4 get_bucket_sampler(int bucket_index, int slot_index, vec2 in_tex_coord) {
    switch(bucket_index) {
        case 0: {
            return texture(bucket_tex_sampler_0, vec3(in_tex_coord, float(slot_index)));
        }
        case 1: {
            return texture(bucket_tex_sampler_1, vec3(in_tex_coord, float(slot_index)));
        }
        case 2: {
            return texture(bucket_tex_sampler_2, vec3(in_tex_coord, float(slot_index)));
        }
    }
}

void main() {
    if (bucket_index < 0) {
        out_color = texture(tex_sampler, in_tex_coord).r * vec4(in_color, 1.f);
    }
    else {
        out_color = get_bucket_sampler(bucket_index, slot_index, in_tex_coord).r * vec4(in_color, 1.f);
    }
}
