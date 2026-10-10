#version 450

layout (set = 2, binding = 0) uniform graphic_uniform {
    mat4 view;
    mat4 projection;
} ubo;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coord;
layout (location = 3) in vec4 in_tangent;
layout (location = 4) in vec3 in_color;

layout (location = 5) in vec4 in_instance_col0;
layout (location = 6) in vec4 in_instance_col1;
layout (location = 7) in vec4 in_instance_col2;
layout (location = 8) in vec4 in_instance_col3;

layout (push_constant) uniform constants {
    layout(offset = 0) mat4 mesh_transform;
    layout(offset = 64) int bucket_index;
    layout(offset = 68) int slot_index;
} push_constant;

layout (location = 0) out vec3 out_color;
layout (location = 1) out vec2 out_tex_coord;
layout (location = 2) out vec3 frag_normal;
layout (location = 3) out vec3 frag_world_pos;
layout (location = 4) out int bucket_index;
layout (location = 5) out int slot_index;

void main() {
    mat4 model = mat4(in_instance_col0, in_instance_col1, in_instance_col2, in_instance_col3);
    vec4 world_pos = ubo.projection * ubo.view * push_constant.mesh_transform * model * vec4(in_position, 1.f);
    gl_Position = world_pos;
    frag_world_pos = world_pos.xyz;
    mat3 normal_matrix = transpose(inverse(mat3(model)));
    frag_normal = normal_matrix * in_normal;
    out_color = in_color; 
    out_tex_coord = in_tex_coord;
    bucket_index = push_constant.bucket_index;
    slot_index = push_constant.slot_index;
}

