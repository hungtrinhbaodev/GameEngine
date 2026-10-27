#version 450

layout (set = 1, binding = 0) uniform sampler2D tex_sampler;

layout (location = 0) in vec2 in_tex_coord;
layout (location = 1) in vec3 in_color;
layout (location = 2) in flat int bucket_index;
layout (location = 3) in flat int slot_index;
layout (location = 4) in flat vec3 stroke_color;
layout (location = 5) in flat float stroke_size;

layout (location = 0) out vec4 out_color;

void main() {
    float distance = texture(tex_sampler, in_tex_coord).r;
    float smooth_width = fwidth(distance);
    clamp(smooth_width, 0.01, 0.05); 
    float alpha = smoothstep(0.5 - smooth_width, 0.5 + smooth_width, distance);
    float stroke_alpha = smoothstep((0.5 - stroke_size) - smooth_width, (0.5 - stroke_size) + smooth_width, distance);
    out_color = mix(vec4(stroke_color, stroke_alpha) , vec4(in_color, 1.f), alpha);
}