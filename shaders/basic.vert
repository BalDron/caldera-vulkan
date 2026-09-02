#version 450
#include "common.glsl"

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec3 out_view_pos;
layout(location = 1) out vec3 out_view_normal;

void main() {
    vec4 view_pos = camera.view * vec4(in_position, 1.0);
    gl_Position = camera.projection * view_pos;

    out_view_pos = view_pos.xyz;
    mat3 normal_matrix = transpose(inverse(mat3(camera.view)));
    out_view_normal = normal_matrix * in_normal;
}