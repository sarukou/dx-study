#include "ObjLoader.h"

#include <DirectXMath.h>

#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace DirectX;

namespace
{
    struct ObjIndex
    {
        int positionIndex = 0;
        int uvIndex = 0;
        int normalIndex = 0;
    };

    // OBJ ‚Ì 1—v‘f‚ð“Ç‚Þ
    ObjIndex ParseObjVertexToken(const std::string& token)
    {
        ObjIndex result = {};

        std::stringstream ss(token);
        std::string part;

        if (!std::getline(ss, part, '/')) {
            throw std::runtime_error("OBJ face parse failed: position index missing.");
        }
        result.positionIndex = std::stoi(part);

        if (!std::getline(ss, part, '/')) {
            throw std::runtime_error("OBJ face parse failed: uv index missing.");
        }
        result.uvIndex = std::stoi(part);

        if (!std::getline(ss, part, '/')) {
            throw std::runtime_error("OBJ face parse failed: normal index missing.");
        }
        result.normalIndex = std::stoi(part);

        return result;
    }
}

MeshData LoadObj(const std::wstring& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open OBJ file.");
    }

    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT2> uvs;
    std::vector<XMFLOAT3> normals;

    MeshData meshData = {};

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "v") {
            XMFLOAT3 p = {};
            ss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        }
        else if (type == "vt") {
            XMFLOAT2 uv = {};
            ss >> uv.x >> uv.y;
            uvs.push_back(uv);
        }
        else if (type == "vn") {
            XMFLOAT3 n = {};
            ss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (type == "f") {
            std::array<std::string, 3> tokens = {};
            ss >> tokens[0] >> tokens[1] >> tokens[2];

            for (int i = 0; i < 3; ++i) {
                ObjIndex objIndex = ParseObjVertexToken(tokens[i]);

                if (objIndex.positionIndex <= 0 || objIndex.positionIndex > static_cast<int>(positions.size())) {
                    throw std::runtime_error("OBJ position index out of range.");
                }
                if (objIndex.uvIndex <= 0 || objIndex.uvIndex > static_cast<int>(uvs.size())) {
                    throw std::runtime_error("OBJ uv index out of range.");
                }
                if (objIndex.normalIndex <= 0 || objIndex.normalIndex > static_cast<int>(normals.size())) {
                    throw std::runtime_error("OBJ normal index out of range.");
                }

                Vertex vertex = {};
                vertex.position = positions[objIndex.positionIndex - 1];
                vertex.uv = uvs[objIndex.uvIndex - 1];
                vertex.normal = normals[objIndex.normalIndex - 1];

                meshData.vertices.push_back(vertex);
                meshData.indices.push_back(static_cast<uint32_t>(meshData.indices.size()));
            }
        }
    }

    if (meshData.vertices.empty() || meshData.indices.empty()) {
        throw std::runtime_error("OBJ mesh is empty.");
    }

    return meshData;
}