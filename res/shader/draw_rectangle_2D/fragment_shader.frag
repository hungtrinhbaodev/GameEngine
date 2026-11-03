#version 450

layout (location = 0) in vec3 frag_color;
layout (location = 1) in vec2 local_pos;
layout (location = 2) in vec2 half_size;
layout (location = 3) in flat float border_width;
layout (location = 4) in flat float border_radius;
layout (location = 5) in flat vec3 border_color;
layout (location = 6) in flat float opacity;

layout (location = 0) out vec4 out_color;

float sd_round_rect(vec2 local_pos, vec2 half_size, float border_radius) {
    vec2 point_from_center = abs(local_pos) - half_size + border_radius;
    return length(max(point_from_center, 0.f)) + min(max(point_from_center.x, point_from_center.y), 0.f) - border_radius;
}

void main() {
    float d = -sd_round_rect(local_pos, half_size, border_radius);
    float smooth_width = fwidth(d);
    float  alpha = smoothstep(-smooth_width, smooth_width, d);
    if (alpha <= 0.f) discard;
    if (border_width <= 0) {
        out_color = vec4(frag_color, alpha * opacity);
    }
    else {
        float border_alpha = smoothstep(border_width -smooth_width , border_width + smooth_width, d);
        out_color = vec4(mix(border_color, frag_color, border_alpha), alpha * opacity);
    }
}
