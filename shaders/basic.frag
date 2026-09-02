#version 450
#include "common.glsl"

layout(location = 0) in vec3 in_view_pos;
layout(location = 1) in vec3 in_view_normal;

layout(location = 0) out vec4 out_color;

void main() {
    vec3 N = normalize(in_view_normal);

    
    if (length(in_view_normal) < 1e-3 || isnan(N.x)) {
        vec3 dX = dFdx(in_view_pos);
        vec3 dY = dFdy(in_view_pos);
        N = normalize(cross(dX, dY));
    }

    
    vec3 L = normalize(vec3(0.5, 0.8, 0.6));

    
    float diff = max(dot(N, L), 0.0);
    float back_diff = max(dot(-N, L), 0.0) * 0.3;
    float total_diff = diff + back_diff;

    vec3 base_gray = vec3(0.72, 0.72, 0.75);
    vec3 ambient = vec3(0.2);
    vec3 light_color = vec3(0.8);

    vec3 final_color = base_gray * (ambient + light_color * total_diff);
    out_color = vec4(final_color, 1.0);
}