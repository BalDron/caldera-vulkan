#pragma once

#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include <vertex.h>
#include <graphics.h>

namespace veng {

struct AABB {
    glm::vec3 min{ std::numeric_limits<float>::max() };
    glm::vec3 max{ std::numeric_limits<float>::lowest() };

    void Expand(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }
};

struct Mesh {
    std::string name;
    std::vector<veng::Vertex> vertices;
    std::vector<uint32_t> indices;
    
    glm::mat4 transform{1.0f};
    AABB aabb;

    uint32_t first_index   = 0;
    uint32_t index_count   = 0;
    int32_t  vertex_offset = 0;
    uint32_t instance_id   = 0;
};

struct GpuAABB {
    alignas(16) glm::vec4 min_point;
    alignas(16) glm::vec4 max_point;
};

struct SceneBuffers {
    std::vector<veng::Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<veng::DrawIndexedIndirectCommand> draw_commands;
    std::vector<veng::GpuAABB> aabbs;
};

SceneBuffers MergeMeshes(std::vector<veng::Mesh>& meshes);

}