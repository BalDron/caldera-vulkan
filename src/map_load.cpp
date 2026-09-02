#include <map_load.h>
#include <vertex.h>
#include <mesh.h>

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <limits>

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/xformable.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/base/plug/registry.h>

namespace std {
    template<> struct hash<veng::Vertex> {
        size_t operator()(const veng::Vertex& vertex) const noexcept {
            size_t h1 = hash<glm::vec3>()(vertex.position);
            size_t h2 = hash<glm::vec3>()(vertex.normal);
            size_t h3 = hash<glm::vec2>()(vertex.uv);
            return ((h1 ^ (h2 << 1)) >> 1) ^ (h3 << 1);
        }
    };
}

namespace veng {

inline bool operator==(const veng::Vertex& a, const veng::Vertex& b) {
    return a.position == b.position && a.normal == b.normal && a.uv == b.uv;
}

inline bool ShouldIgnorePrim(const pxr::UsdPrim& prim) {
    const std::string name = prim.GetName().GetString();
    if (name.find("volume") != std::string::npos) {
        return true;
    }
    return false;
    return (name.rfind("reflection_volume", 0) == 0 ||
            name.rfind("lightdrid_volume", 0) == 0 ||
            name.rfind("volume_terrain_cutout_", 0) == 0);
}

bool LoadUSDAMesh(const gsl::czstring file_path, std::vector<veng::Mesh>& out_meshes) 
{
    out_meshes.clear();

    #ifdef USD_PLUGIN_DIR 
    pxr::PlugRegistry::GetInstance().RegisterPlugins(
        "/home/drozh/Documents/Projects/52_caldera_vulkan/Project/usd_install/lib/usd"
    );
    pxr::PlugRegistry::GetInstance().RegisterPlugins(
        "/home/drozh/Documents/Projects/52_caldera_vulkan/Project/build/_deps/open-usd-build/pxr/usd"
    );
    pxr::PlugRegistry::GetInstance().RegisterPlugins(
        "/home/drozh/Documents/Projects/52_caldera_vulkan/Project/build/_deps/open-usd-build/pxr/base"
    );
    #endif

    pxr::UsdStageRefPtr stage = pxr::UsdStage::Open(file_path);
    if (!stage) {
        std::cerr << "Failed to open USD file: " << file_path << std::endl;
        return false;
    }

    
    pxr::UsdPrimRange range = stage->Traverse();
    for (auto it = range.begin(); it != range.end(); ++it) {
        const pxr::UsdPrim& prim = *it;

        
        if (ShouldIgnorePrim(prim)) {
            it.PruneChildren();
            continue;
        }
    
        if (!prim.IsA<pxr::UsdGeomMesh>()) {
            continue;
        }
        
        pxr::UsdGeomMesh usd_mesh(prim);

        pxr::VtArray<pxr::GfVec3f> points;
        pxr::VtArray<int> face_vertex_counts;
        pxr::VtArray<int> face_vertex_indices;

        usd_mesh.GetPointsAttr().Get(&points);
        usd_mesh.GetFaceVertexCountsAttr().Get(&face_vertex_counts);
        usd_mesh.GetFaceVertexIndicesAttr().Get(&face_vertex_indices);

        if (points.empty() || face_vertex_counts.empty()) {
            continue;
        }

        veng::Mesh current_mesh;
        current_mesh.name = prim.GetPath().GetString();

        
        pxr::UsdGeomXformable xformable(prim);
        pxr::GfMatrix4d localToWorld(1.0);
        bool resetsXformStack = false;
        xformable.GetLocalTransformation(&localToWorld, &resetsXformStack);
        
        const double* d_mat = localToWorld.GetArray();
        current_mesh.transform = glm::mat4(
            d_mat[0], d_mat[1], d_mat[2], d_mat[3],
            d_mat[4], d_mat[5], d_mat[6], d_mat[7],
            d_mat[8], d_mat[9], d_mat[10], d_mat[11],
            d_mat[12], d_mat[13], d_mat[14], d_mat[15]
        );

        pxr::VtArray<pxr::GfVec3f> normals;
        usd_mesh.GetNormalsAttr().Get(&normals);
        pxr::TfToken norm_interp = usd_mesh.GetNormalsInterpolation();
        auto getNormal = [&](size_t faceIndex, size_t cornerIndex, int ptIdx, size_t fviOffset, const glm::vec3& fallback_norm) -> glm::vec3 {
            if (normals.empty()) return fallback_norm;

            if (norm_interp == pxr::UsdGeomTokens->faceVarying && fviOffset < normals.size()) {
                return glm::vec3(normals[fviOffset][0], normals[fviOffset][1], normals[fviOffset][2]);
            } else if (norm_interp == pxr::UsdGeomTokens->vertex && static_cast<size_t>(ptIdx) < normals.size()) {
                return glm::vec3(normals[ptIdx][0], normals[ptIdx][1], normals[ptIdx][2]);
            } else if (norm_interp == pxr::UsdGeomTokens->uniform && faceIndex < normals.size()) {
                return glm::vec3(normals[faceIndex][0], normals[faceIndex][1], normals[faceIndex][2]);
            }
            return fallback_norm;
        };

        pxr::UsdGeomPrimvarsAPI primvars_api(usd_mesh);
        pxr::UsdGeomPrimvar uv_primvar = primvars_api.GetPrimvar(pxr::TfToken("st"));
        if (!uv_primvar.IsDefined()) {
            uv_primvar = primvars_api.GetPrimvar(pxr::TfToken("uv"));
        }

        pxr::VtArray<pxr::GfVec2f> uvs;
        pxr::VtArray<int> uv_indices;
        pxr::TfToken interpolation = pxr::UsdGeomTokens->constant;

        if (uv_primvar.IsDefined()) {
            uv_primvar.Get(&uvs);
            interpolation = uv_primvar.GetInterpolation();
            if (uv_primvar.IsIndexed()) {
                uv_primvar.GetIndices(&uv_indices);
            }
        }

        auto getUV = [&](size_t faceIndex, size_t cornerIndex, int ptIdx, size_t fviOffset) -> glm::vec2 {
            if (uvs.empty()) return glm::vec2(0.0f);

            if (interpolation == pxr::UsdGeomTokens->faceVarying) {
                int idx = uv_indices.empty() ? static_cast<int>(fviOffset) : uv_indices[fviOffset];
                return glm::vec2(uvs[idx][0], uvs[idx][1]);
            } 
            else if (interpolation == pxr::UsdGeomTokens->vertex) {
                int idx = uv_indices.empty() ? ptIdx : uv_indices[ptIdx];
                return glm::vec2(uvs[idx][0], uvs[idx][1]);
            } 
            else if (interpolation == pxr::UsdGeomTokens->uniform) {
                int idx = uv_indices.empty() ? static_cast<int>(faceIndex) : uv_indices[faceIndex];
                return glm::vec2(uvs[idx][0], uvs[idx][1]);
            }

            return glm::vec2(uvs[0][0], uvs[0][1]);
        };

        std::unordered_map<veng::Vertex, uint32_t> local_unique_vertices;
        size_t fviOffset = 0;

        glm::mat3 normal_to_world = glm::mat3(glm::transpose(glm::inverse(current_mesh.transform)));

        for (size_t faceIdx = 0; faceIdx < face_vertex_counts.size(); ++faceIdx) {
            int count = face_vertex_counts[faceIdx];
            for (int i = 1; i < count - 1; ++i) {
                int corners[3] = {0, i, i + 1};

                int pt0 = face_vertex_indices[fviOffset + corners[0]];
                int pt1 = face_vertex_indices[fviOffset + corners[1]];
                int pt2 = face_vertex_indices[fviOffset + corners[2]];

                glm::vec3 p0(points[pt0][0], points[pt0][1], points[pt0][2]);
                glm::vec3 p1(points[pt1][0], points[pt1][1], points[pt1][2]);
                glm::vec3 p2(points[pt2][0], points[pt2][1], points[pt2][2]);

                glm::vec3 geo_normal = glm::cross(p1 - p0, p2 - p0);
                geo_normal = (glm::length(geo_normal) > 1e-6f) ? glm::normalize(geo_normal) : glm::vec3(0.0f, 1.0f, 0.0f);

                for (int c = 0; c < 3; ++c) {
                    int corner = corners[c];
                    size_t curOffset = fviOffset + corner;
                    int ptIdx = face_vertex_indices[curOffset];

                    glm::vec3 raw_pos(points[ptIdx][0], points[ptIdx][1], points[ptIdx][2]);
                    glm::vec3 raw_norm = getNormal(faceIdx, corner, ptIdx, curOffset, geo_normal);

                    veng::Vertex vertex{};
                    
                    vertex.position = raw_pos;

                    float norm_len = glm::length(raw_norm);
                    vertex.normal = (norm_len > 1e-6f) ? (raw_norm / norm_len) : geo_normal;
                    
                    vertex.color = glm::vec3(1.0f);
                    vertex.uv = getUV(faceIdx, corner, ptIdx, curOffset);
                    vertex.uv.y = 1.0f - vertex.uv.y;

                    glm::vec4 world_pos4 = current_mesh.transform * glm::vec4(raw_pos, 1.0f);
                    vertex.position = glm::vec3(world_pos4);

                    glm::vec3 world_norm = normal_to_world * raw_norm;
                    norm_len = glm::length(world_norm);
                    vertex.normal = (norm_len > 1e-6f) ? (world_norm / norm_len) : glm::normalize(normal_to_world * geo_normal);

                    vertex.color = glm::vec3(1.0f);
                    vertex.uv = getUV(faceIdx, corner, ptIdx, curOffset);
                    vertex.uv.y = 1.0f - vertex.uv.y;

                    current_mesh.aabb.Expand(vertex.position);

                    if (local_unique_vertices.find(vertex) == local_unique_vertices.end()) {
                        local_unique_vertices[vertex] = static_cast<uint32_t>(current_mesh.vertices.size());
                        current_mesh.vertices.push_back(vertex);
                    }
                    current_mesh.indices.push_back(local_unique_vertices[vertex]);
                }
            }
            fviOffset += count;
        }

        if (!current_mesh.vertices.empty()) {
            out_meshes.push_back(std::move(current_mesh));
        }
    }

    return !out_meshes.empty();
}

}