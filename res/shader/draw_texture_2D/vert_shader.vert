#version 450

layout (location = 0) in vec2 in_position;
layout (location = 1) in uint in_storage_index;
layout (location = 2) in uint in_vertex_offset;
layout (location = 3) in uint in_config_index;

struct Instance_Data{
    vec2  tex_size;
    vec2  translation;
    vec2  scale;
    vec2  anchor;
    vec4  rect;
    float rotation;
    int   bucket_index;
    int   slot_index;
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

layout (location = 0) out vec2 frag_tex_coord;
layout (location = 1) out int bucket_index;
layout (location = 2) out int slot_index;

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

vec2 map_tex_coord(uint vertex_index, vec4 rect) {
    switch(vertex_index) {
        case 0: {
            return vec2(rect.x, rect.y);
        }
        case 1: {
            return vec2(rect.x, rect.y + rect.w);
        }
        case 2: {
            return vec2(rect.x + rect.z, rect.y + rect.w);
        }
        case 3: {
            return vec2(rect.x + rect.z, rect.y);
        }
        default: {
            return vec2(0.f, 0.f);
        }
    }
}

void main() {
    Instance_Data data = instance_buffer.data[in_storage_index];
    Config_2D_Data config = config_2D_buffer.data[in_config_index];
    vec2 translate_anchor = (in_position - data.anchor) * data.tex_size * data.scale;
    vec2 rectangle_position = data.translation + rotate(data.rotation, translate_anchor);
    gl_Position = vec4(flipped_position(norm_position(rectangle_position, config.screen_size)), 0.f, 1.f);
    vec2 mapped_tex_coord = map_tex_coord(gl_VertexIndex - in_vertex_offset, data.rect);
    frag_tex_coord = vec2(mapped_tex_coord.x, 1.f - mapped_tex_coord.y);
    bucket_index = data.bucket_index;
    slot_index = data.slot_index;
}