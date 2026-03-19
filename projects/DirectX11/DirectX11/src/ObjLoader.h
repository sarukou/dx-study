#pragma once

#include "Types.h"
#include <string>

// OBJ “Ç‚İ‚İ
MeshData LoadObj(const std::wstring& path);

// tangentŒvZ
static void ComputeTangents(MeshData& meshData);