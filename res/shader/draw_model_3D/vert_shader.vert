#version 450

layout (set = 3, binding = 0) uniform graphic_uniform {
    mat4 view;
    mat4 projection;
} ubo;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coord;
layout (location = 3) in vec4 in_tangent;
layout (location = 4) in vec3 in_color;

layout (location = 5) in uint in_storage_offset;
layout (location = 6) in uint in_mesh_tranform_offset;
layout (location = 7) in uint in_texture_offset;

layout(std430, set = 0, binding = 0) readonly buffer Instance_Buffer {
    mat4 data[];
} instance_buffer;

layout(std430, set = 0, binding = 1) readonly buffer Mesh_Tranform_Buffer {
    mat4 data[];
} mesh_transform_buffer;

struct Texture_Data {
    int bucket_index;
    int slot_index;
    vec2 tex_scale;
};

layout(std430, set = 0, binding = 2) readonly buffer Texture_Buffer {
    Texture_Data data[];
} texture_buffer;

layout (location = 0) out vec3 out_color;
layout (location = 1) out vec2 out_tex_coord;
layout (location = 2) out vec3 frag_normal;
layout (location = 3) out vec3 frag_world_pos;
layout (location = 4) out int bucket_index;
layout (location = 5) out int slot_index;

void main() {
    mat4 model = instance_buffer.data[in_storage_offset];
    mat4 mesh_transform = mesh_transform_buffer.data[in_mesh_tranform_offset];
    Texture_Data texuture = texture_buffer.data[in_texture_offset];
    vec4 world_pos = ubo.projection * ubo.view * model * mesh_transform * vec4(in_position, 1.f);
    gl_Position = world_pos;
    frag_world_pos = world_pos.xyz;
    mat3 normal_matrix = transpose(inverse(mat3(model)));
    frag_normal = normal_matrix * in_normal;
    out_color = in_color; 
    out_tex_coord = in_tex_coord * texuture.tex_scale;
    bucket_index = texuture.bucket_index;
    slot_index = texuture.slot_index;
}

