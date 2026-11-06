#version 450

layout (location = 0) in vec2 in_position;
layout (location = 1) in uint in_storage_index;
layout (location = 2) in uint in_config_index;

struct Instance_Data {
    vec2  translation;
    vec2  size;
    vec3  instance_color;
    float rotation;
    vec2  anchor;
    float border_radius;
    float border_width;
    vec3  border_color;
    float opacity;
    uvec3  clipping_offsets;
    uint  number_clipping;
};

layout(std430, set = 0, binding = 0) readonly buffer Instance_Buffer {
    Instance_Data data[];
} instance_buffer;

struct Config_2D_Data {
    vec2 screen_size;
};

layout(std430, set = 0, binding = 1) readonly buffer Config_2D_Data_Buffer {
    Config_2D_Data data[];
} config_2D_buffer;

layout (location = 0) out vec3 frag_color;
layout (location = 1) out vec2 local_pos;
layout (location = 2) out vec2 half_size;
layout (location = 3) out float border_width;
layout (location = 4) out float border_radius;
layout (location = 5) out vec3 border_color;
layout (location = 6) out float opacity;
layout (location = 7) out uint number_clipping;
layout (location = 8) out uvec3 clipping_offsets;
layout (location = 9) out vec2 position;
layout (location = 10) out vec2 scale;
layout (location = 11) out float rotation;


vec2 norm_position(vec2 position, vec2 screen_size) {
    return vec2(position.x / screen_size.x, position.y / screen_size.y);
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
    Instance_Data data = instance_buffer.data[in_storage_index];
    Config_2D_Data config = config_2D_buffer.data[in_config_index];
    vec2 translate_anchor = (in_position - data.anchor) * data.size;
    vec2 rectangle_position = data.translation + rotate(data.rotation, translate_anchor);
    gl_Position = vec4(flipped_position(norm_position(rectangle_position, config.screen_size)), 0.f, 1.f);
    frag_color = data.instance_color;
    local_pos = (in_position - 0.5f) * data.size;
    half_size = data.size * 0.5;
    border_radius = data.border_radius;
    border_width = data.border_width;
    border_color = data.border_color;
    opacity = data.opacity;
    number_clipping = data.number_clipping;
    clipping_offsets = data.clipping_offsets;
    position = data.translation + rotate(data.rotation, (0.5f - data.anchor) * data.size);
    rotation = data.rotation;
    scale = vec2(1, 1);
}