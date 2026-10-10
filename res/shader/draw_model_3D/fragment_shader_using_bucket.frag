#version 450
layout (set = 0, binding = 0) uniform sampler2D tex_sampler;
layout (set = 1, binding = 0) uniform sampler2DArray bucket_tex_sampler_0;
layout (set = 1, binding = 1) uniform sampler2DArray bucket_tex_sampler_1;
layout (set = 1, binding = 2) uniform sampler2DArray bucket_tex_sampler_2;
layout (set = 1, binding = 3) uniform sampler2DArray bucket_tex_sampler_3;
layout (set = 1, binding = 4) uniform sampler2DArray bucket_tex_sampler_4;
layout (set = 1, binding = 5) uniform sampler2DArray bucket_tex_sampler_5;
layout (set = 1, binding = 6) uniform sampler2DArray bucket_tex_sampler_6;

layout (location = 0) in vec3 in_color;
layout (location = 1) in vec2 in_tex_coord;
layout (location = 2) in vec3 frag_normal;
layout (location = 3) in vec3 frag_world_pos;
layout (location = 4) in flat int bucket_index;
layout (location = 5) in flat int slot_index;

layout (location = 0) out vec4 out_color;

const vec3 LIGHT_DIR = normalize(vec3(0.5f, 1.f, 0.3f));
const vec3 LIGHT_COLOR = vec3(1.0);
const vec3 AMBIENT = vec3(0.1f);

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
        case 3: {
            return texture(bucket_tex_sampler_3, vec3(in_tex_coord, float(slot_index)));
        }
        case 4: {
            return texture(bucket_tex_sampler_4, vec3(in_tex_coord, float(slot_index)));
        }
        case 5: {
            return texture(bucket_tex_sampler_5, vec3(in_tex_coord, float(slot_index)));
        }
        case 6: {
            return texture(bucket_tex_sampler_6, vec3(in_tex_coord, float(slot_index)));
        }
        default: {
            return texture(tex_sampler, in_tex_coord);
        }
    }
}

void main() {
    vec3 normal = normalize(frag_normal);
    float brightness = max(dot(normal, LIGHT_DIR), 0.f);
    vec3 color = AMBIENT + LIGHT_COLOR * brightness;
    out_color = vec4(color, 1.f) * get_bucket_sampler(bucket_index, slot_index, in_tex_coord);
}