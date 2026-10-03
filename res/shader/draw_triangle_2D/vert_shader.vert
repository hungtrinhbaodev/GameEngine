#version 450

layout (location = 0) in vec2 in_position;

layout (location = 1) in vec2 in_first_point;
layout (location = 2) in vec2 in_second_point;
layout (location = 3) in vec2 in_thrid_point;
layout (location = 4) in vec3 in_color;

layout (location = 0) out vec3 frag_color;

layout (push_constant) uniform constants {
    layout (offset = 0) vec2 screen_size;
    layout (offset = 8) int vertex_offset;
} push_constant;

vec2 norm_position(vec2 position) {
    return vec2(position.x / push_constant.screen_size.x, position.y / push_constant.screen_size.y);
}

vec2 flipped_position(vec2 norm_position) {
    vec2 rate_position = vec2(-1.f + norm_position.x * 2, -1.f + (1.f - norm_position.y) * 2);
    return rate_position;
}

vec2 map_vertex_point(int index) {
    switch(index) {
        case 0: {
            return in_first_point;
        }
        case 1: {
            return in_second_point;
        }
        case 2: {
            return in_thrid_point;
        }
        default: {
            return vec2(0.f, 0.f);
        }
    }
}

void main() {
    vec2 point_position = in_position + map_vertex_point(gl_VertexIndex - push_constant.vertex_offset);
    gl_Position = vec4(flipped_position(norm_position(point_position)), 0.f, 1.f);
    frag_color = in_color;
}