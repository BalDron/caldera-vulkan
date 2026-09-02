#pragma once

#include <vector>
#include <memory>
#include <array>
#include <glm/glm.hpp>
#include <mesh.h>

namespace veng {

struct Frustum {
    
    std::array<glm::vec4, 6> planes;

    static Frustum FromViewProjection(const glm::mat4& vp) {
        Frustum f;
        
        f.planes[0] = glm::vec4(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]);
        f.planes[1] = glm::vec4(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]);
        f.planes[2] = glm::vec4(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]);
        f.planes[3] = glm::vec4(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]);
        f.planes[4] = glm::vec4(vp[0][2], vp[1][2], vp[2][2], vp[3][2]);
        f.planes[5] = glm::vec4(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]);

        for (auto& plane : f.planes) {
            float length = glm::length(glm::vec3(plane));
            if (length > 0.0f) plane /= length;
        }
        return f;
    }

    bool Intersects(const AABB& box) const {
        for (const auto& plane : planes) {
            
            glm::vec3 p = box.min;
            if (plane.x >= 0) p.x = box.max.x;
            if (plane.y >= 0) p.y = box.max.y;
            if (plane.z >= 0) p.z = box.max.z;

            if (glm::dot(glm::vec3(plane), p) + plane.w < 0.0f) {
                return false; 
            }
        }
        return true;
    }
};

class QuadtreeNode {
public:
    static constexpr size_t MAX_ELEMENTS_PER_NODE = 16;
    static constexpr size_t MAX_DEPTH = 6;

    AABB bounds;
    size_t depth = 0;
    std::vector<const veng::Mesh*> meshes;
    std::array<std::unique_ptr<QuadtreeNode>, 4> children;

    QuadtreeNode(const AABB& _bounds, size_t _depth = 0) 
        : bounds(_bounds), depth(_depth) {}

    bool IsLeaf() const {
        return children[0] == nullptr;
    }

    void Subdivide() {
        glm::vec3 min = bounds.min;
        glm::vec3 max = bounds.max;
        glm::vec3 mid = (min + max) * 0.5f;

        
        AABB q0{ {min.x, min.y, min.z}, {mid.x, max.y, mid.z} }; 
        AABB q1{ {mid.x, min.y, min.z}, {max.x, max.y, mid.z} }; 
        AABB q2{ {min.x, min.y, mid.z}, {mid.x, max.y, max.z} }; 
        AABB q3{ {mid.x, min.y, mid.z}, {max.x, max.y, max.z} }; 

        children[0] = std::make_unique<QuadtreeNode>(q0, depth + 1);
        children[1] = std::make_unique<QuadtreeNode>(q1, depth + 1);
        children[2] = std::make_unique<QuadtreeNode>(q2, depth + 1);
        children[3] = std::make_unique<QuadtreeNode>(q3, depth + 1);
    }

    bool Contains(const AABB& parent, const AABB& child) const {
        return child.min.x >= parent.min.x && child.max.x <= parent.max.x &&
               child.min.z >= parent.min.z && child.max.z <= parent.max.z;
    }

    void Insert(const veng::Mesh* mesh) {
        if (!IsLeaf()) {
            for (auto& child : children) {
                if (Contains(child->bounds, mesh->aabb)) {
                    child->Insert(mesh);
                    return;
                }
            }
            
            meshes.push_back(mesh);
            return;
        }

        meshes.push_back(mesh);

        
        if (meshes.size() > MAX_ELEMENTS_PER_NODE && depth < MAX_DEPTH) {
            Subdivide();
            std::vector<const veng::Mesh*> remaining;
            for (const auto* m : meshes) {
                bool inserted = false;
                for (auto& child : children) {
                    if (Contains(child->bounds, m->aabb)) {
                        child->Insert(m);
                        inserted = true;
                        break;
                    }
                }
                if (!inserted) remaining.push_back(m);
            }
            meshes = std::move(remaining);
        }
    }

    void Query(const Frustum& frustum, std::vector<const veng::Mesh*>& out_visible) const {
        
        if (!frustum.Intersects(bounds)) {
            return; 
        }

        
        for (const auto* mesh : meshes) {
            if (frustum.Intersects(mesh->aabb)) {
                out_visible.push_back(mesh);
            }
        }

        
        if (!IsLeaf()) {
            for (const auto& child : children) {
                child->Query(frustum, out_visible);
            }
        }
    }
};

class Quadtree {
public:
    std::unique_ptr<QuadtreeNode> root;

    void Build(const std::vector<veng::Mesh>& meshes) {
        if (meshes.empty()) return;

        
        AABB scene_bounds;
        for (const auto& mesh : meshes) {
            scene_bounds.Expand(mesh.aabb.min);
            scene_bounds.Expand(mesh.aabb.max);
        }

        root = std::make_unique<QuadtreeNode>(scene_bounds);
        for (const auto& mesh : meshes) {
            root->Insert(&mesh);
        }
    }

    void QueryFrustum(const Frustum& frustum, std::vector<const veng::Mesh*>& out_visible) const {
        out_visible.clear();
        if (root) {
            root->Query(frustum, out_visible);
        }
    }
};

}