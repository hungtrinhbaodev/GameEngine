#version 450
layout (set = 0, binding = 0) uniform sampler2D tex_sampler;

layout (location = 0) in vec3 in_color;
layout (location = 1) in vec2 in_tex_coord;
layout (location = 2) in vec3 frag_normal;
layout (location = 3) in vec3 frag_world_pos;
layout (location = 4) in flat int bucket_index;
layout (location = 5) in flat int slot_index;

layout (location = 0) out vec4 out_color;

const vec3 LIGHT_DIR = normalize(vec3(0.5f, 1.f, 0.3f));
const vec3 LIGHT_COLOR = vec3(1.0);
const vec3 AMBIENT = vec3(0.1f);

void main() {
    vec3 normal = normalize(frag_normal);
    float brightness = max(dot(normal, LIGHT_DIR), 0.f);
    vec3 color = AMBIENT + LIGHT_COLOR * brightness;
    out_color = vec4(color, 1.f) * texture(tex_sampler, in_tex_coord);
}