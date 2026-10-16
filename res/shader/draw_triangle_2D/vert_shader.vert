#version 450

layout (location = 0) in vec2 in_position;
layout (location = 1) in uint in_storage_index;
layout (location = 2) in uint in_vertex_offset;
layout (location = 3) in uint in_config_index;

struct Instance_Data{
    vec2  first_point;
    vec2  second_point;
    vec3  color;
    float pad;
    vec2  third_point;
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

vec2 norm_position(vec2 position, vec2 screen_size) {
    return vec2(position.x / screen_size.x, position.y / screen_size.y);
}

vec2 flipped_position(vec2 norm_position) {
    vec2 rate_position = vec2(-1.f + norm_position.x * 2, -1.f + (1.f - norm_position.y) * 2);
    return rate_position;
}

vec2 map_vertex_point(uint index, Instance_Data data) {
    switch(index) {
        case 0: {
            return data.first_point;
        }
        case 1: {
            return data.second_point;
        }
        case 2: {
            return data.third_point;
        }
        default: {
            return vec2(0.f, 0.f);
        }
    }
}

void main() {
    Instance_Data data = instance_buffer.data[in_storage_index];
    Config_2D_Data config = config_2D_buffer.data[in_config_index];
    vec2 point_position = in_position + map_vertex_point(gl_VertexIndex - in_vertex_offset, data);
    gl_Position = vec4(flipped_position(norm_position(point_position, config.screen_size)), 0.f, 1.f);
    frag_color = data.color;
}