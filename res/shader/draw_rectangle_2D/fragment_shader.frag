#version 450

struct Clipping_Data {
    vec2 half_size;
    vec2 position;
    vec2 scale;
    float border_radius;
    float rotation;
};

layout(std430, set = 0, binding = 2) readonly buffer Clipping_Data_Buffer {
    Clipping_Data data[];
} clipping_data_buffer;


layout (location = 0) in vec3 frag_color;
layout (location = 1) in vec2 local_pos;
layout (location = 2) in vec2 half_size;
layout (location = 3) in flat float border_width;
layout (location = 4) in flat float border_radius;
layout (location = 5) in flat vec3 border_color;
layout (location = 6) in flat float opacity;
layout (location = 7) in flat uint number_clipping;
layout (location = 8) in flat uvec3 clipping_offsets;
layout (location = 9) in flat vec2 position;
layout (location = 10) in flat vec2 scale;
layout (location = 11) in flat float rotation;

layout (location = 0) out vec4 out_color;

float sd_round_rect(vec2 local_pos, vec2 half_size, float border_radius) {
    vec2 point_from_center = abs(local_pos) - half_size + border_radius;
    return length(max(point_from_center, 0.f)) + min(max(point_from_center.x, point_from_center.y), 0.f) - border_radius;
}

vec2 rotate(float rotation, vec2 position) {
    float r = radians(rotation);
    return vec2(position.x * cos(r) - position.y * sin(r), position.x * sin(r) + position.y * cos(r));
}

bool is_clipping() {
    if (number_clipping <= 0) return false;
    vec2 world_pos = position + rotate(rotation, local_pos * scale);
    for (int i = 0; i < number_clipping;i++) {
        Clipping_Data clipping_data = clipping_data_buffer.data[clipping_offsets[i]];
        vec2 clipping_pos =  rotate(-clipping_data.rotation, world_pos - clipping_data.position) / clipping_data.scale;
        if (sd_round_rect(clipping_pos, clipping_data.half_size, clipping_data.border_radius) >= 0.f) return true;
    }
    return false;
}

void main() {
    if (is_clipping()) discard;
    float d = -sd_round_rect(local_pos, half_size, border_radius);
    float smooth_width = fwidth(d);
    float  alpha = smoothstep(-smooth_width, smooth_width, d);
    if (alpha <= 0.f) discard;
    if (border_width <= 0) {
        out_color = vec4(frag_color, alpha * opacity);
    }
    else {
        float border_alpha = smoothstep(border_width -smooth_width , border_width + smooth_width, d);
        out_color = vec4(mix(border_color, frag_color, border_alpha), alpha * opacity);
    }
}
