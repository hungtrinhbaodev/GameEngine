#version 450

layout (location = 0) in vec2 in_position;

layout (location = 1) in vec2 in_tex_size;
layout (location = 2) in vec2 in_translation;
layout (location = 3) in vec2 in_scale;
layout (location = 4) in vec2 in_anchor_point;
layout (location = 5) in vec4 in_rect;
layout (location = 6) in float in_rotation;
layout (location = 7) in int in_bucket_index;
layout (location = 8) in int in_slot_index;

layout (push_constant) uniform constants {
    layout(offset = 0) vec2 screen_size;
    layout(offset = 8) int vertex_offset;
} push_constant;

layout (location = 0) out vec2 frag_tex_coord;
layout (location = 1) out int bucket_index;
layout (location = 2) out int slot_index;

vec2 norm_position(vec2 position) {
    return vec2(position.x / push_constant.screen_size.x, position.y / push_constant.screen_size.y);
}

vec2 rotate(float rotation, vec2 position) {
    float r = radians(rotation);
    return vec2(position.x * cos(r) - position.y * sin(r), position.x * sin(r) + position.y * cos(r));
}

vec2 flipped_position(vec2 norm_position) {
    vec2 rate_position = vec2(-1.f + norm_position.x * 2, -1.f + (1.f - norm_position.y) * 2);
    return rate_position;
}

vec2 map_tex_coord(int vertex_index) {
    switch(vertex_index) {
        case 0: {
            return vec2(in_rect.x, in_rect.y);
        }
        case 1: {
            return vec2(in_rect.x, in_rect.y + in_rect.w);
        }
        case 2: {
            return vec2(in_rect.x + in_rect.z, in_rect.y + in_rect.w);
        }
        case 3: {
            return vec2(in_rect.x + in_rect.z, in_rect.y);
        }
        default: {
            return vec2(0.f, 0.f);
        }
    }
}

void main() {
    vec2 translate_anchor = (in_position - in_anchor_point) * in_tex_size * in_scale;
    vec2 rectangle_position = in_translation + rotate(in_rotation, translate_anchor);
    gl_Position = vec4(flipped_position(norm_position(rectangle_position)), 0.f, 1.f);
    vec2 mapped_tex_coord = map_tex_coord(gl_VertexIndex - push_constant.vertex_offset);
    frag_tex_coord = vec2(mapped_tex_coord.x, 1.f - mapped_tex_coord.y);
    bucket_index = in_bucket_index;
    slot_index = in_slot_index;
}