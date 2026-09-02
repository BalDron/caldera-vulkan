#pragma once

#include <vulkan/vulkan.h>

namespace veng {

struct Vertex {
    Vertex() : position(glm::vec3(0.0f)), uv(glm::vec2(0.0f)) {}
    Vertex(glm::vec3 _position, glm::vec2 _uv) : position(_position), uv(_uv) {}

    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 uv;

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription description = {};
        description.binding = 0;
        description.stride = sizeof(Vertex);
        description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return description;
    }

    static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> descriptions = {};
        
        descriptions[0].binding = 0;
        descriptions[0].location = 0;
        descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        descriptions[0].offset = offsetof(Vertex, position);

        descriptions[1].binding = 0;
        descriptions[1].location = 1;
        descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        descriptions[1].offset = offsetof(Vertex, uv);

        descriptions[2].binding = 0;
        descriptions[2].location = 2;
        descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        descriptions[2].offset = offsetof(Vertex, normal);

        return descriptions;
    }
};

}