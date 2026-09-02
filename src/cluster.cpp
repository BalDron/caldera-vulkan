#include <cluster.h>
#include <mesh.h>
#include <algorithm>
#include <cmath>

namespace veng {

ClusterSceneBuffers BuildClusters(
    const std::vector<Vertex>& vertices,
    const std::vector<uint32_t>& indices,
    const std::vector<DrawIndexedIndirectCommand>& draw_commands
) {
    ClusterSceneBuffers result;
    constexpr uint32_t kMaxTrianglesPerCluster = 128;
    constexpr uint32_t kIndicesPerCluster = kMaxTrianglesPerCluster * 3;

    for (uint32_t mesh_id = 0; mesh_id < draw_commands.size(); ++mesh_id) {
        const auto& cmd = draw_commands[mesh_id];
        uint32_t start_cluster_idx = static_cast<uint32_t>(result.clusters.size());
        uint32_t total_mesh_indices = cmd.indexCount;

        for (uint32_t offset = 0; offset < total_mesh_indices; offset += kIndicesPerCluster) {
            uint32_t count = std::min(kIndicesPerCluster, total_mesh_indices - offset);

            glm::vec3 min_pt(std::numeric_limits<float>::max());
            glm::vec3 max_pt(std::numeric_limits<float>::lowest());
            glm::vec3 avg_normal(0.0f);
            uint32_t tri_count = count / 3;

            for (uint32_t i = 0; i < count; i += 3) {
                uint32_t i0 = indices[cmd.firstIndex + offset + i + 0] + cmd.vertexOffset;
                uint32_t i1 = indices[cmd.firstIndex + offset + i + 1] + cmd.vertexOffset;
                uint32_t i2 = indices[cmd.firstIndex + offset + i + 2] + cmd.vertexOffset;

                glm::vec3 v0 = vertices[i0].position;
                glm::vec3 v1 = vertices[i1].position;
                glm::vec3 v2 = vertices[i2].position;

                min_pt = glm::min(min_pt, glm::min(v0, glm::min(v1, v2)));
                max_pt = glm::max(max_pt, glm::max(v0, glm::max(v1, v2)));

                glm::vec3 tri_norm = glm::cross(v1 - v0, v2 - v0);
                if (glm::length(tri_norm) > 1e-6f) {
                    avg_normal += glm::normalize(tri_norm);
                }
            }

            
            glm::vec3 center = (min_pt + max_pt) * 0.5f;
            float radius = glm::length(max_pt - center) + 0.02f;

            
            glm::vec3 cone_axis = (glm::length(avg_normal) > 1e-5f) ? glm::normalize(avg_normal) : glm::vec3(0.0f, 1.0f, 0.0f);
            float min_dot = 1.0f;

            for (uint32_t i = 0; i < count; i += 3) {
                uint32_t i0 = indices[cmd.firstIndex + offset + i + 0] + cmd.vertexOffset;
                uint32_t i1 = indices[cmd.firstIndex + offset + i + 1] + cmd.vertexOffset;
                uint32_t i2 = indices[cmd.firstIndex + offset + i + 2] + cmd.vertexOffset;

                glm::vec3 tri_norm = glm::cross(vertices[i1].position - vertices[i0].position, vertices[i2].position - vertices[i0].position);
                if (glm::length(tri_norm) > 1e-6f) {
                    min_dot = std::min(min_dot, glm::dot(cone_axis, glm::normalize(tri_norm)));
                }
            }

            
            float half_angle = std::acos(std::clamp(min_dot, -1.0f, 1.0f));
            float cone_cutoff = -std::sin(half_angle);

            GpuCluster cluster{};
            cluster.sphere_bounds = glm::vec4(center, radius);
            cluster.cone_axis_angle = glm::vec4(cone_axis, cone_cutoff);
            cluster.first_index = cmd.firstIndex + offset;
            cluster.index_count = count;
            cluster.vertex_offset = cmd.vertexOffset;
            cluster.instance_id = cmd.firstInstance;

            result.clusters.push_back(cluster);
        }

        uint32_t cluster_count = static_cast<uint32_t>(result.clusters.size()) - start_cluster_idx;
        result.mesh_ranges.push_back({ start_cluster_idx, cluster_count });
    }

    return result;
}

}