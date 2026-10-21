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
layout (location = 5) in uvec4 in_joints;
layout (location = 6) in vec4 in_weights;

layout (location = 7) in uint in_storage_offset;
layout (location = 8) in uint in_texture_offset;
layout (location = 9) in uint in_mesh_descriptor_offset;

layout(std430, set = 0, binding = 0) readonly buffer Mat4_Buffer {
    mat4 data[];
} mat4_buffer;

struct Texture_Data {
    int bucket_index;
    int slot_index;
    vec2 tex_scale;
};

layout(std430, set = 0, binding = 1) readonly buffer Texture_Buffer {
    Texture_Data data[];
} texture_buffer;

struct Mesh_Descriptor_Data {
    uint using_skins;
    uint skins_offset;
    uint mesh_transform_offset;
};

layout(std430, set = 0, binding = 2) readonly buffer Mesh_Descriptor_Buffer {
    Mesh_Descriptor_Data data[];
} mesh_descriptor_buffer;

layout (location = 0) out vec3 out_color;
layout (location = 1) out vec2 out_tex_coord;
layout (location = 2) out vec3 frag_normal;
layout (location = 3) out vec3 frag_world_pos;
layout (location = 4) out int bucket_index;
layout (location = 5) out int slot_index;

void main() {
    mat4 model = mat4_buffer.data[in_storage_offset];
    Mesh_Descriptor_Data mesh_descriptor = mesh_descriptor_buffer.data[in_mesh_descriptor_offset];
    Texture_Data texture = texture_buffer.data[in_texture_offset];
    uint skins_offset = mesh_descriptor.skins_offset;
    uint mesh_transform_offset = mesh_descriptor.mesh_transform_offset;
    mat4 local;
    if (mesh_descriptor.using_skins != 0) {
        local = in_weights.x * mat4_buffer.data[skins_offset + in_joints.x]
                + in_weights.y * mat4_buffer.data[skins_offset + in_joints.y]
                + in_weights.z * mat4_buffer.data[skins_offset + in_joints.z]
                + in_weights.w * mat4_buffer.data[skins_offset + in_joints.w];
    }
    else {
        local = mat4_buffer.data[mesh_transform_offset];
    }
    vec4 world_pos = ubo.projection * ubo.view * model * local * vec4(in_position, 1.f);
    gl_Position = world_pos;
    frag_world_pos = world_pos.xyz;
    mat3 normal_matrix = transpose(inverse(mat3(model * local)));
    frag_normal = normal_matrix * in_normal;
    out_color = in_color; 
    out_tex_coord = in_tex_coord * texture.tex_scale;
    bucket_index = texture.bucket_index;
    slot_index = texture.slot_index;
}

