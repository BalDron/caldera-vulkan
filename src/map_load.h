#pragma once

#include <mesh.h>

namespace veng {
    bool LoadUSDAMesh(const gsl::czstring file_path, std::vector<veng::Mesh>& out_meshes);
}