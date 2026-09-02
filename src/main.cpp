#include <iostream>
#include <precomp.h>
#include <GLFW/glfw3.h>
#include <glfw_initialization.h>
#include <glfw_monitor.h>
#include <glfw_window.h>
#include <graphics.h>
#include <map_load.h>
#include <glm/gtc/matrix_transform.hpp>
#include <mesh.h>
#include <quad_tree_culling.h>
#include <fstream>
#include <cluster.h>


std::string GetPathToScene(const std::string& config_path) {
    std::ifstream f(config_path);
    if (!f.is_open()) {
        throw std::runtime_error("failed to open file");
    }

    std::string line;
    if (std::getline(f, line)) {
        return line;
    }
    return "";
}


std::int32_t main(std::int32_t argc, gsl::zstring* argv) {
    const veng::GlfwInitialization _glfw;

    veng::Window window("Vulkan Engine", {1920, 1080});

    glm::ivec2 window_size = window.GetWindowSize();
    window.TryMoveToMonitor(0);

    veng::Graphics graphics(&window);
    
    std::vector<veng::Mesh> meshes = {};
    std::string scene_path_std = GetPathToScene("config.txt");
    gsl::czstring scene_path_gsl = scene_path_std.c_str();
    bool res = veng::LoadUSDAMesh(scene_path_gsl, meshes);
    std::cout << scene_path_gsl << std::endl;

    const int grid_dim_x = 2;
    const int grid_dim_y = 3;
    const float spacing_x = 4500.0f;
    const float spacing_y = 4500.0f;

    std::vector<veng::Mesh> base_meshes = std::move(meshes);
    meshes.clear();
    meshes.reserve(base_meshes.size() * grid_dim_x * grid_dim_y);


    for (int gx = 0; gx < grid_dim_x; ++gx) {
        for (int gy = 0; gy < grid_dim_y; ++gy) {
            glm::vec3 offset(gx * spacing_x, gy * spacing_y, 0.0f);

            for (const auto& base_mesh : base_meshes) {
                veng::Mesh duplicate = base_mesh;

                
                for (auto& vertex : duplicate.vertices) {
                    vertex.position += offset;
                }

                
                duplicate.aabb.min += offset;
                duplicate.aabb.max += offset;

                meshes.push_back(std::move(duplicate));
            }
        }
    }

    veng::SceneBuffers merged = veng::MergeMeshes(meshes);
    veng::ClusterSceneBuffers cluster_data = veng::BuildClusters(merged.vertices, merged.indices, merged.draw_commands);
    if (meshes.empty() || merged.vertices.empty() || cluster_data.clusters.empty()) {
        std::cerr << "Error: Loaded scene has no geometry/clusters to process." << std::endl;
        return EXIT_FAILURE;
    }

    veng::BufferHandle global_vb = graphics.CreateVertexBuffer(merged.vertices);
    veng::BufferHandle global_ib = graphics.CreateIndexBuffer(merged.indices);

    veng::BufferHandle aabb_ssbo = graphics.CreateStorageBuffer(sizeof(veng::GpuAABB) * merged.aabbs.size(), merged.aabbs.data());
    veng::BufferHandle in_cmds_ssbo = graphics.CreateIndirectBuffer(merged.draw_commands);
    veng::BufferHandle out_cmds_ssbo = graphics.CreateStorageBuffer(sizeof(veng::DrawIndexedIndirectCommand) * merged.draw_commands.size(), nullptr);
    veng::BufferHandle visible_inst_count_buf = graphics.CreateStorageBuffer(sizeof(uint32_t), nullptr);
    veng::BufferHandle visible_inst_ids_buf = graphics.CreateStorageBuffer(sizeof(uint32_t) * merged.draw_commands.size(), nullptr);

    veng::BufferHandle all_clusters_ssbo = graphics.CreateStorageBuffer(sizeof(veng::GpuCluster) * cluster_data.clusters.size(), cluster_data.clusters.data());
    veng::BufferHandle mesh_ranges_ssbo = graphics.CreateStorageBuffer(sizeof(veng::MeshClusterRange) * cluster_data.mesh_ranges.size(), cluster_data.mesh_ranges.data());

    veng::BufferHandle candidate_indices_buf = graphics.CreateStorageBuffer(sizeof(uint32_t) * cluster_data.clusters.size(), nullptr);
    veng::BufferHandle candidate_counter_buf = graphics.CreateStorageBuffer(sizeof(uint32_t) * 4, nullptr); 
    veng::BufferHandle out_cluster_draw_cmds = graphics.CreateStorageBuffer(sizeof(veng::DrawIndexedIndirectCommand) * cluster_data.clusters.size(), nullptr);
    veng::BufferHandle final_cluster_count_buf = graphics.CreateStorageBuffer(sizeof(uint32_t), nullptr);


    VkDescriptorSet cull_set = graphics.CreateCullDescriptorSet(
        aabb_ssbo,
        in_cmds_ssbo,
        out_cmds_ssbo,
        visible_inst_count_buf,
        visible_inst_ids_buf
    );
    VkDescriptorSet expand_set = graphics.CreateExpandDescriptorSet(
        visible_inst_count_buf,
        visible_inst_ids_buf,
        mesh_ranges_ssbo,
        candidate_indices_buf,
        candidate_counter_buf
    );

    VkDescriptorSet cluster_cull_set = graphics.CreateClusterCullDescriptorSet(
        all_clusters_ssbo,
        candidate_indices_buf,
        candidate_counter_buf,
        out_cluster_draw_cmds,
        final_cluster_count_buf
    );

    veng::Quadtree quadtree;
    quadtree.Build(meshes);

    std::vector<const veng::Mesh*> visible_meshes;
    visible_meshes.reserve(meshes.size());

    glm::vec3 view_vec = {20.0f, 20.0f, 20.0f};

    glm::vec3 camera_pos   = glm::vec3(0.0f, 20.0f, 50.0f);
    glm::vec3 camera_front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 camera_up    = glm::vec3(0.0f, 1.0f, 0.0f);

    float yaw   = -90.0f; 
    float pitch = 0.0f;

    

    std::vector<glm::vec3> positions_to_visit = {};
    positions_to_visit.push_back(glm::vec3(2545.0f, 1415.0f, 4163.0f));
    positions_to_visit.push_back(glm::vec3(2457.0f, 188.0f, 3362.0f));
    positions_to_visit.push_back(glm::vec3(2845.0f, 367.0f, 1523.0f));
    positions_to_visit.push_back(glm::vec3(2535.0f, 423.0f, -1514.0f));
    

    size_t current_target_idx = 0;
    float total_rotated_angle = 0.0f;
    bool has_teleported = false;
    bool has_finished_moving = false;
    bool has_finished_rotating = false;

    float start_delay = 5.0f;

    std::fstream framerate_file;
    framerate_file.open("vulkan_framerate.txt", std::ios::out);

    float last_frame_time = static_cast<float>(glfwGetTime());
    float timestamp = 0.0f;

    while (!window.ShouldClose()) {
        glfwPollEvents();
        std::cout << camera_pos.x << " " << camera_pos.y << " " << camera_pos.z << std::endl;
        
        float current_frame_time = static_cast<float>(glfwGetTime());
        float delta_time = current_frame_time - last_frame_time;
        
        timestamp += delta_time;
        last_frame_time = current_frame_time;
        float camera_move_speed   = 250.0f * delta_time;
        float camera_rotate_speed = 150.0f * delta_time;

        if (timestamp >= start_delay && !positions_to_visit.empty()) {
            if (!has_teleported) {
                camera_pos = positions_to_visit[0];
                current_target_idx = 1;
                has_teleported = true;
            }

            if (delta_time > 0.0f) {
                float fps = 1.0f / delta_time;
                framerate_file << timestamp - start_delay << " " << fps << std::endl;
            }
            if (current_target_idx < positions_to_visit.size() && !has_finished_moving) {
                glm::vec3 target = positions_to_visit[current_target_idx];
                glm::vec3 to_target = target - camera_pos;
                float distance = glm::length(to_target);

                if (distance <= camera_move_speed) {
                    camera_pos = target;
                    current_target_idx++;
                    if (current_target_idx == positions_to_visit.size()) {
                        has_finished_moving = true;
                    }
                } else {
                    camera_pos += (to_target / distance) * camera_move_speed;
                }
            } else if (!has_finished_rotating) {
                float remaining_rotation = 360.0f - total_rotated_angle;
                float step = std::min(camera_rotate_speed, remaining_rotation);

                yaw += step;
                total_rotated_angle += step;

                if (total_rotated_angle >= 360.0f) {
                    has_finished_rotating = true;
                    framerate_file.close();
                }
            }
        }

        if (glfwGetKey(window.GetHandle(), GLFW_KEY_LEFT) == GLFW_PRESS) {
            yaw -= camera_rotate_speed;
        }
        if (glfwGetKey(window.GetHandle(), GLFW_KEY_RIGHT) == GLFW_PRESS) {
            yaw += camera_rotate_speed;
        }
        if (glfwGetKey(window.GetHandle(), GLFW_KEY_UP) == GLFW_PRESS) {
            pitch += camera_rotate_speed;
        }
        if (glfwGetKey(window.GetHandle(), GLFW_KEY_DOWN) == GLFW_PRESS) {
            pitch -= camera_rotate_speed;
        }
        pitch = std::clamp(pitch, -89.0f, 89.0f);
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        camera_front = glm::normalize(front);

        glm::vec3 camera_right = glm::normalize(glm::cross(camera_front, camera_up));

        if (glfwGetKey(window.GetHandle(), GLFW_KEY_W) == GLFW_PRESS) {
            camera_pos += camera_front * camera_move_speed;
        }
        if (glfwGetKey(window.GetHandle(), GLFW_KEY_S) == GLFW_PRESS) {
            camera_pos -= camera_front * camera_move_speed;
        }
        if (glfwGetKey(window.GetHandle(), GLFW_KEY_A) == GLFW_PRESS) {
            camera_pos -= camera_right * camera_move_speed;
        }
        if (glfwGetKey(window.GetHandle(), GLFW_KEY_D) == GLFW_PRESS) {
            camera_pos += camera_right * camera_move_speed;
        }
        if (glfwGetKey(window.GetHandle(), GLFW_KEY_SPACE) == GLFW_PRESS) {
            camera_pos += camera_up * camera_move_speed;
        }
        if (glfwGetKey(window.GetHandle(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            camera_pos -= camera_up * camera_move_speed;
        }

        glm::mat4 view = glm::lookAt(camera_pos, camera_pos + camera_front, camera_up);
        glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1920.0f / 1080.0f, 0.1f, 10000.0f);
        projection[1][1] *= -1.0f;
        glm::mat4 zUpToYUp = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        graphics.SetViewProjection(view * zUpToYUp, projection);

        glm::mat4 view_proj = projection * view;
        veng::Frustum camera_frustum = veng::Frustum::FromViewProjection(view_proj);
        quadtree.QueryFrustum(camera_frustum, visible_meshes);

        if (graphics.BeginFrame()) {
            
            glm::mat4 total_vp = projection * (view * zUpToYUp);
            glm::vec3 cluster_space_cam_pos = glm::vec3(glm::inverse(zUpToYUp) * glm::vec4(camera_pos, 1.0f));            
            
            graphics.CullFrustum(
                cull_set,
                out_cmds_ssbo,
                visible_inst_count_buf,
                total_vp,
                static_cast<uint32_t>(merged.draw_commands.size())
            );
            graphics.ExpandAndCullClusters(
                expand_set,
                cluster_cull_set,
                candidate_counter_buf,
                out_cluster_draw_cmds,
                final_cluster_count_buf,
                total_vp,
                cluster_space_cam_pos,
                static_cast<uint32_t>(cluster_data.clusters.size())
            );
            graphics.BeginRenderPass();
            graphics.RenderIndirectCount(
                global_vb,
                global_ib,
                out_cluster_draw_cmds,
                final_cluster_count_buf,
                static_cast<uint32_t>(cluster_data.clusters.size())
            );
            
            graphics.EndRenderPass();
            graphics.BuildHiZPyramid();

            graphics.EndFrame();
        }
    }

    graphics.DestroyBuffer(global_vb);
    graphics.DestroyBuffer(global_ib);
    graphics.DestroyBuffer(aabb_ssbo);
    graphics.DestroyBuffer(in_cmds_ssbo);
    graphics.DestroyBuffer(out_cmds_ssbo);
    graphics.DestroyBuffer(visible_inst_count_buf);
    graphics.DestroyBuffer(visible_inst_ids_buf);
    graphics.DestroyBuffer(all_clusters_ssbo);
    graphics.DestroyBuffer(mesh_ranges_ssbo);
    graphics.DestroyBuffer(candidate_indices_buf);
    graphics.DestroyBuffer(candidate_counter_buf);
    graphics.DestroyBuffer(out_cluster_draw_cmds);
    graphics.DestroyBuffer(final_cluster_count_buf);

    return EXIT_SUCCESS;
}