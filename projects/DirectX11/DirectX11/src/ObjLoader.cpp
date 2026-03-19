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

    ComputeTangents(meshData);

    return meshData;
}


static XMFLOAT3 Subtract(const XMFLOAT3& a, const XMFLOAT3& b)
{
    return XMFLOAT3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static XMFLOAT2 Subtract(const XMFLOAT2& a, const XMFLOAT2& b)
{
    return XMFLOAT2(a.x - b.x, a.y - b.y);
}

static void AddTo(XMFLOAT3& dst, const XMFLOAT3& v)
{
    dst.x += v.x;
    dst.y += v.y;
    dst.z += v.z;
}

static float Dot(const XMFLOAT3& a, const XMFLOAT3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static float Length(const XMFLOAT3& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

static XMFLOAT3 NormalizeSafe(const XMFLOAT3& v)
{
    float len = Length(v);
    if (len < 1e-6f)
    {
        return XMFLOAT3(1.0f, 0.0f, 0.0f);
    }

    return XMFLOAT3(v.x / len, v.y / len, v.z / len);
}

static XMFLOAT3 Multiply(const XMFLOAT3& v, float s)
{
    return XMFLOAT3(v.x * s, v.y * s, v.z * s);
}

static XMFLOAT3 SubtractVec(const XMFLOAT3& a, const XMFLOAT3& b)
{
    return XMFLOAT3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static void ComputeTangents(MeshData& meshData)
{
    // ‰Šú‰»
    for (auto& vertex : meshData.vertices)
    {
        vertex.tangent = XMFLOAT3(0.0f, 0.0f, 0.0f);
    }

    // ŽOŠpŒ`‚²‚Æ‚É tangent ‚ðŒvŽZ‚µ‚ÄŠe’¸“_‚Ö‰ÁŽZ
    for (size_t i = 0; i + 2 < meshData.indices.size(); i += 3)
    {
        uint32_t i0 = meshData.indices[i + 0];
        uint32_t i1 = meshData.indices[i + 1];
        uint32_t i2 = meshData.indices[i + 2];

        Vertex& v0 = meshData.vertices[i0];
        Vertex& v1 = meshData.vertices[i1];
        Vertex& v2 = meshData.vertices[i2];

        XMFLOAT3 edge1 = Subtract(v1.position, v0.position);
        XMFLOAT3 edge2 = Subtract(v2.position, v0.position);

        XMFLOAT2 deltaUV1 = Subtract(v1.uv, v0.uv);
        XMFLOAT2 deltaUV2 = Subtract(v2.uv, v0.uv);

        float denom = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
        if (std::fabs(denom) < 1e-8f)
        {
            continue;
        }

        float f = 1.0f / denom;

        XMFLOAT3 tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        AddTo(v0.tangent, tangent);
        AddTo(v1.tangent, tangent);
        AddTo(v2.tangent, tangent);
    }

    // ’¸“_‚²‚Æ‚É tangent ‚ð³‹K‰»
    for (auto& vertex : meshData.vertices)
    {
        XMFLOAT3 N = NormalizeSafe(vertex.normal);
        XMFLOAT3 T = vertex.tangent;

        // Gram-Schmidt: T = T - N * dot(N, T)
        float ndott = Dot(N, T);
        XMFLOAT3 projected = Multiply(N, ndott);
        T = SubtractVec(T, projected);

        vertex.tangent = NormalizeSafe(T);
    }
}