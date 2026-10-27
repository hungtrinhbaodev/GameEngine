#version 450

layout (location = 0) in vec2 in_position;
layout (location = 1) in uint in_storage_index;
layout (location = 2) in uint in_char_storage_index;
layout (location = 3) in uint in_vertex_offset;
layout (location = 4) in uint in_config_index;

struct Instance_Data {
    vec2  tex_size;
    vec2  translation;
    vec3  color;
    float rotation;
    vec2  scale;
    int   bucket_index;
    int   slot_index;
    vec3  stroke_color;
    float stroke_size;
};

layout(std430, set = 0, binding = 0) readonly buffer Instance_Buffer {
    Instance_Data data[];
} instance_buffer;

struct Char_Instance_Data {
    vec2  char_translation;
    uint  rect_offset;
};

layout(std430, set = 0, binding = 1) readonly buffer Char_Instance_Buffer {
    Char_Instance_Data data[];
} char_instance_buffer;

struct Char_Rect_Instance_Data {
    vec4 rect;
};

layout(std430, set = 0, binding = 2) readonly buffer Rect_Instance_Buffer {
    Char_Rect_Instance_Data data[];
} rect_instance_buffer;

struct Config_2D_Data {
    vec2 screen_size;
};

layout(std430, set = 0, binding = 3) readonly buffer Config_2D_Data_Buffer {
    Config_2D_Data data[];
} config_2D_buffer;

layout (location = 0) out vec2 frag_tex_coord;
layout (location = 1) out vec3 out_color;
layout (location = 2) out int bucket_index;
layout (location = 3) out int slot_index;
layout (location = 4) out vec3 out_stroke_color;
layout (location = 5) out float out_stroke_size;

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

vec2 map_tex_coord(uint vertex_index, Char_Rect_Instance_Data data) {
    switch(vertex_index) {
        case 0: {
            return vec2(data.rect.x, data.rect.y);
        }
        case 1: {
            return vec2(data.rect.x, data.rect.y + data.rect.w);
        }
        case 2: {
            return vec2(data.rect.x + data.rect.z, data.rect.y + data.rect.w);
        }
        case 3: {
            return vec2(data.rect.x + data.rect.z, data.rect.y);
        }
        default: {
            return vec2(0.f, 0.f);
        }
    }
}

vec2 map_char_position(uint vertex_index, vec2 char_translation, vec2 rect_size) {
    switch(vertex_index) {
        case 0: {
            return vec2(char_translation.x, char_translation.y);
        }
        case 1: {
            return vec2(char_translation.x, char_translation.y + rect_size.y);
        }
        case 2: {
            return vec2(char_translation.x + rect_size.x, char_translation.y + rect_size.y);
        }
        case 3: {
            return vec2(char_translation.x + rect_size.x, char_translation.y);
        }
        default: {
            return vec2(0.f, 0.f);
        }
    }
}

void main() {
    Instance_Data data = instance_buffer.data[in_storage_index];
    Char_Instance_Data char_data = char_instance_buffer.data[in_char_storage_index];
    Char_Rect_Instance_Data rect_data = rect_instance_buffer.data[char_data.rect_offset];
    Config_2D_Data config = config_2D_buffer.data[in_config_index];
    uint vertex_index = gl_VertexIndex - in_vertex_offset;
    vec2 rect_size = vec2(rect_data.rect.z, rect_data.rect.w) * data.tex_size;
    vec2 translate_anchor = map_char_position(vertex_index, char_data.char_translation, rect_size) * data.scale;
    vec2 rectangle_position = data.translation + rotate(data.rotation, translate_anchor);
    gl_Position = vec4(flipped_position(norm_position(rectangle_position, config.screen_size)), 0.f, 1.f);
    vec2 mapped_tex_coord = map_tex_coord(vertex_index, rect_data);
    frag_tex_coord = vec2(mapped_tex_coord.x, 1.f - mapped_tex_coord.y);
    bucket_index = data.bucket_index;
    slot_index = data.slot_index;
    out_color = data.color;
    out_stroke_color = data.stroke_color;
    out_stroke_size = data.stroke_size;
}