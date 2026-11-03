#version 450

layout (location = 0) in vec3 frag_color;
layout (location = 1) in vec2 local_pos;
layout (location = 2) in vec2 half_size;
layout (location = 3) in float border_width;
layout (location = 4) in float border_radius;
layout (location = 5) in vec3 border_color;

layout (location = 0) out vec4 out_color;

float sd_round_rect(vec2 local_pos, vec2 half_size, float border_radius) {
    vec2 point_from_origin = abs(local_pos) - half_size + border_radius;
    return length(max(point_from_origin, 0.f)) + min(max(point_from_origin.x, point_from_origin.y), 0.0) - border_radius;
}

void main() {
    out_color = vec4(frag_color, 1.0f);
}
