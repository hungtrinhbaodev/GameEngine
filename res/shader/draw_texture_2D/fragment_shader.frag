#version 450

layout (set = 1, binding = 0) uniform sampler2D tex_sampler;

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

layout (location = 0) in vec2 in_tex_coord;
layout (location = 1) in flat int bucket_index;
layout (location = 2) in flat int slot_index;
layout (location = 3) in flat vec3 frag_color;
layout (location = 4) in flat float opacity;
layout (location = 5) in flat uint number_clipping;
layout (location = 6) in flat uvec3 clipping_offsets;
layout (location = 7) in flat vec2 position;
layout (location = 8) in flat vec2 scale;
layout (location = 9) in flat float rotation;
layout (location = 10) in vec2 local_pos;
layout (location = 11) in flat vec2 half_size;

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
    vec2 world_pos = position + rotate(rotation, (local_pos) * scale);
    for (int i = 0; i < number_clipping;i++) {
        Clipping_Data clipping_data = clipping_data_buffer.data[clipping_offsets[i]];
        if (clipping_data.scale.x <= 0 || clipping_data.scale.y <= 0) return true;
        vec2 clipping_pos =  rotate(-clipping_data.rotation, world_pos - clipping_data.position) / clipping_data.scale;
        if (sd_round_rect(clipping_pos, clipping_data.half_size, clipping_data.border_radius) >= 0.f) return true;
    }
    return false;
}

void main() {
    if (is_clipping()) discard;
    out_color = texture(tex_sampler, in_tex_coord) * vec4(frag_color, opacity);
}
