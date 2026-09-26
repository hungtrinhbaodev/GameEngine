#version 450

layout (set = 0, binding = 0) uniform graphic_uniform {
    mat4 view;
    mat4 projection;
} ubo;

layout (location = 0) in vec2 in_position;
layout (location = 1) in vec3 in_color;

layout (location = 2) in vec2 in_tex_size;
layout (location = 3) in vec2 in_translation;
layout (location = 4) in vec2 in_scale;
layout (location = 5) in vec2 in_anchor_point;
layout (location = 6) in vec2 in_tex_coord0;
layout (location = 7) in vec2 in_tex_coord1;
layout (location = 8) in vec2 in_tex_coord2;
layout (location = 9) in vec2 in_tex_coord3;
layout (location = 10) in float in_rotation;
layout (location = 11) in float in_z_depth;

layout (push_constant) uniform constants {
    layout(offset = 0) vec2 screen_size;
    layout(offset = 8) uint vertex_offset;
} push_constant;

layout (location = 0) out vec2 frag_tex_coord;

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

void main() {
    vec2 tex_coord[4] = vec2[4](
        in_tex_coord0,
        in_tex_coord1,
        in_tex_coord2,
        in_tex_coord3
    );
    uint vertex_index = (gl_VertexIndex - push_constant.vertex_offset);
    vec2 translate_anchor = (in_position - in_anchor_point) * in_tex_size * in_scale;
    vec2 rectangle_position = in_translation + rotate(in_rotation, translate_anchor);
    gl_Position = vec4(flipped_position(norm_position(rectangle_position)), in_z_depth, 1.f);
    frag_tex_coord = vec2(tex_coord[vertex_index].x, 1.f - tex_coord[vertex_index].y);
}