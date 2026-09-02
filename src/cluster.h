#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include <vertex.h>
#include <graphics.h>

namespace veng {

struct GpuCluster {
    alignas(16) glm::vec4 sphere_bounds;
    alignas(16) glm::vec4 cone_axis_angle;
    uint32_t first_index;
    uint32_t index_count;
    int32_t  vertex_offset;
    uint32_t instance_id;
};

struct MeshClusterRange {
    uint32_t first_cluster;
    uint32_t cluster_count;
};

struct ClusterSceneBuffers {
    std::vector<GpuCluster> clusters;
    std::vector<MeshClusterRange> mesh_ranges;
};

ClusterSceneBuffers BuildClusters(
    const std::vector<Vertex>& vertices,
    const std::vector<uint32_t>& indices,
    const std::vector<DrawIndexedIndirectCommand>& draw_commands
);

}