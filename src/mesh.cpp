#include <vector>
#include <mesh.h>
#include <vertex.h>

namespace veng {

SceneBuffers MergeMeshes(std::vector<veng::Mesh>& meshes) {
    SceneBuffers scene_buffers;

    size_t total_vertices = 0;
    size_t total_indices  = 0;
    for (const auto& mesh : meshes) {
        total_vertices += mesh.vertices.size();
        total_indices  += mesh.indices.size();
    }
    scene_buffers.vertices.reserve(total_vertices);
    scene_buffers.indices.reserve(total_indices);

    for (uint32_t i = 0; i < meshes.size(); ++i) {
        auto& mesh = meshes[i];
        if (mesh.vertices.empty() || mesh.indices.empty()) {
            continue;
        }

        mesh.first_index   = static_cast<uint32_t>(scene_buffers.indices.size());
        mesh.index_count   = static_cast<uint32_t>(mesh.indices.size());
        mesh.vertex_offset = static_cast<int32_t>(scene_buffers.vertices.size());
        mesh.instance_id   = i;

        scene_buffers.vertices.insert(
            scene_buffers.vertices.end(),
            mesh.vertices.begin(),
            mesh.vertices.end()
        );
        scene_buffers.indices.insert(
            scene_buffers.indices.end(),
            mesh.indices.begin(),
            mesh.indices.end()
        );

        mesh.vertices.clear();
        mesh.vertices.shrink_to_fit();
        mesh.indices.clear();
        mesh.indices.shrink_to_fit();

        DrawIndexedIndirectCommand cmd{};
        cmd.indexCount    = mesh.index_count;
        cmd.instanceCount = 1;
        cmd.firstIndex    = mesh.first_index;
        cmd.vertexOffset  = mesh.vertex_offset;
        cmd.firstInstance = mesh.instance_id; 
        scene_buffers.draw_commands.push_back(cmd);

        GpuAABB gpu_box{};
        gpu_box.min_point = glm::vec4(mesh.aabb.min, 1.0f);
        gpu_box.max_point = glm::vec4(mesh.aabb.max, 1.0f);
        scene_buffers.aabbs.push_back(gpu_box);
    }

    return scene_buffers;
}

}