#extension GL_KHR_vulkan_glsl: enable
#extension GL_EXT_shader_draw_parameters : enable
#extension GL_ARB_shader_draw_parameters : enable

layout(set = 0, binding = 0) uniform UniformTransformations {
    mat4 view;
    mat4 projection;
} camera;