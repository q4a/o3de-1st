/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERABC_ALEMBICCOMPILER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERABC_ALEMBICCOMPILER_H
#pragma once

#include "IConvertor.h"
#include "GeomCache.h"
#include "../ResourceCompilerPC/PhysWorld.h"
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/condition_variable.h>

#include <Alembic/AbcGeom/Visibility.h> // for Alembic::AbcGeom::ObjectVisibility

#define RC_ABC_AUTOMATIC_UVMAX_DETECTION_VALUE .0f

class ICryXML;
class GeomCacheEncoder;
class IXMLSerializer;

// Used for detecting identical meshes. For two identical meshes all digests must match.
class AlembicMeshDigest
{
    friend struct std::hash<AlembicMeshDigest>;
public:
    AlembicMeshDigest(Alembic::AbcGeom::IPolyMeshSchema& meshSchema);

    bool operator==(const AlembicMeshDigest& digest) const;

private:
    bool m_bHasNormals;
    bool m_bHasTexcoords;
    bool m_bHasColors;

    Alembic::AbcGeom::ArraySampleKey m_positionDigest;
    Alembic::AbcGeom::ArraySampleKey m_positionIndexDigest;
    Alembic::AbcGeom::ArraySampleKey m_normalsDigest;
    Alembic::AbcGeom::ArraySampleKey m_texcoordDigest;
    Alembic::AbcGeom::ArraySampleKey m_colorsDigest;
};

// Unoptimized vertex used in the compiler
struct AlembicCompilerVertex
{
    Vec3 m_position;
    Vec3 m_normal;
    Vec2 m_texcoords;
    Vec4 m_rgba;
};

template<class T>
class AlembicCompilerHash;

template<>
class AlembicCompilerHash<float>
{
public:
    uint64 operator()(const float value)
    {
        uint32 bits = alias_cast<uint32>(value);
        bits = (bits == 0x80000000) ? 0 : bits; // -0 == 0

        // Magic taken from CityHash64
        const uint64 u = bits;
        const uint64 kMul = 0x9ddfea08eb382d69ULL;
        uint64 a = u * kMul;
        a ^= (a >> 47);
        a *= kMul;
        return a;
    }
};

template<>
class AlembicCompilerHash<uint64>
{
public:
    uint64 operator()(const uint64 value)
    {
        return value;
    }
};

// Helper function to combine hashes
template<class T>
inline void AlembicCompilerHashCombine(uint64& seed, const T& v)
{
    AlembicCompilerHash<T> hasher;

    // Magic taken from CityHash64
    const uint64 kMul = 0x9ddfea08eb382d69ULL;
    uint64 a = (hasher(v) ^ seed) * kMul;
    a ^= (a >> 47);
    uint64 b = (seed ^ a) * kMul;
    b ^= (b >> 47);
    seed = b * kMul;
}

template<>
class AlembicCompilerHash<AlembicCompilerVertex>
{
public:
    uint64 operator()(const AlembicCompilerVertex& vertex) const
    {
        uint64 hash = 0;

        AlembicCompilerHashCombine(hash, vertex.m_position[0]);
        AlembicCompilerHashCombine(hash, vertex.m_position[1]);
        AlembicCompilerHashCombine(hash, vertex.m_position[2]);
        AlembicCompilerHashCombine(hash, vertex.m_normal[0]);
        AlembicCompilerHashCombine(hash, vertex.m_normal[1]);
        AlembicCompilerHashCombine(hash, vertex.m_normal[2]);
        AlembicCompilerHashCombine(hash, vertex.m_texcoords[0]);
        AlembicCompilerHashCombine(hash, vertex.m_texcoords[1]);
        AlembicCompilerHashCombine(hash, vertex.m_rgba[0]);
        AlembicCompilerHashCombine(hash, vertex.m_rgba[1]);
        AlembicCompilerHashCombine(hash, vertex.m_rgba[2]);
        AlembicCompilerHashCombine(hash, vertex.m_rgba[3]);

        return hash;
    }
};

// std::hash<> specializations for std::unordered_map
namespace std
{
    template<>
    class hash<AlembicMeshDigest>
    {
    public:
        size_t operator()(const AlembicMeshDigest& digest) const
        {
            // Just return the position digest. It's very likely that
            // if positions match everything else is matching as well
            return Alembic::AbcGeom::StdHash(digest.m_positionDigest);
        }
    };
}

class GeomCacheEncoder;

class AlembicCompiler
    : public ICompiler
{
    struct FrameData
    {
        FrameData()
            : m_errorCount(0)
            , m_pAlembicCompiler(nullptr) {}

        FrameData(const FrameData& toBeCopied)
            : m_errorCount(0)
            , m_pAlembicCompiler(toBeCopied.m_pAlembicCompiler)
        {
            m_errorCount.store(toBeCopied.m_errorCount);
        }

        uint m_jobIndex;
        uint m_frameIndex;
        AZStd::atomic_uint m_errorCount;
        Alembic::Abc::chrono_t m_frameTime;
        AlembicCompiler* m_pAlembicCompiler;
        AABB m_frameAABB;
    };

public:
    AlembicCompiler(ICryXML* pXMLParser);
    virtual ~AlembicCompiler() {}

    // ICompiler methods.
    virtual void Release();
    virtual void BeginProcessing([[maybe_unused]] const IConfig* config) {}
    virtual void EndProcessing() {}
    virtual IConvertContext* GetConvertContext() { return &m_CC; }
    virtual bool Process();

private:
    XmlNodeRef ReadConfig(const string& configPath, IXMLSerializer* pXMLSerializer);

    string GetOutputFileNameOnly() const;
    string GetOutputPath() const;

    uint32_t GetIndex(Alembic::AbcGeom::GeometryScope geomScope, const Alembic::Abc::UInt32ArraySamplePtr& normalIndices, size_t currentIndexArraysIndex, int32_t positionIndex);

    bool CheckTimeSampling(Alembic::Abc::IArchive& archive);
    void OutputTimeSamplingType(const Alembic::Abc::TimeSamplingType& timeSamplingType);

    void CheckTimeSamplingRec(const Alembic::Abc::IObject& currentObject);
    void CheckTimeSamplingRec(const Alembic::Abc::ICompoundProperty& currentProperty);

    // Setup static data for header
    bool CompileStaticData(Alembic::Abc::IArchive& archive);
    bool CompileStaticDataRec(GeomCache::Node* pParentNode, Alembic::Abc::IObject& currentObject, const QuatTNS& localTransform,
        std::vector<Alembic::AbcGeom::IXform> abcXformStack, const bool bParentRemoved, const GeomCacheFile::ETransformType parentTransform);
    bool CompileStaticMeshData(GeomCache::Node& node, Alembic::AbcGeom::IPolyMesh& mesh);
    bool CompilePhysicsGeometry(GeomCache::Node& node, Alembic::AbcGeom::IPolyMesh& mesh);
    void CheckMeshForColors(Alembic::AbcGeom::IPolyMeshSchema& meshSchema, GeomCache::Mesh& mesh) const;

    // Prints the node tree
    void PrintNodeTreeRec(GeomCache::Node& node, string padding);

    // Compile stream of frames and  send them to the cache writer
    bool CompileAnimationData(Alembic::Abc::IArchive& archive, GeomCacheEncoder& geomCacheEncoder);

    // Pushes the completed frames in order to the encoder
    void PushCompletedFrames(GeomCacheEncoder& geomCacheEncoder);

    // Update transforms
    void UpdateTransformsWithErrorHandling();
    typedef std::unordered_map<std::string, Alembic::AbcGeom::M44d> TMatrixMap;
    typedef std::unordered_map<std::string, Alembic::AbcGeom::ObjectVisibility> TVisibilityMap;
    void UpdateTransformsRec(GeomCache::Node& node, const Alembic::Abc::chrono_t frameTime,
        AABB& frameAABB, QuatTNS currentTransform, TMatrixMap& matrixMap, TVisibilityMap& visibilityMap, std::string& currentObjectPath);

    // Compiles a mesh or update its vertices in a job
    void UpdateVertexDataWithErrorHandling(GeomCache::Mesh* mesh);

    // Gets mapping from alembic face Id to material ids
    std::unordered_map<uint32, uint16> GetMeshMaterialMap(const Alembic::AbcGeom::IPolyMesh& mesh, const Alembic::Abc::chrono_t frameTime);

    // Generates a hash for each vertex (all frames are taken into account)
    bool ComputeVertexHashes(std::vector<uint64>& abcVertexHashes, const size_t currentFrame, const size_t numAbcIndices, GeomCache::Mesh& mesh,
        Alembic::Abc::TimeSampling& meshTimeSampling, size_t numMeshSamples, Alembic::AbcGeom::IPolyMeshSchema& meshSchema, const bool bHasNormals,
        const bool bHasTexcoords, const bool bHasColors, const size_t numAbcNormalIndices, const size_t numAbcTexcoordsIndices, const size_t numAbcFaces);

    // This is used for the first frame compilation of constant/homogeneous meshes and for each frame for heterogeneous meshes.
    bool CompileFullMesh(GeomCache::Mesh& mesh, const size_t currentFrame, const QuatTNS& transform);

    // Update vertex data according to the mapping table produced by CompileFullMesh at frameTime. Used for homogeneous meshes.
    bool UpdateVertexData(GeomCache::Mesh& mesh, const size_t currentFrame);

    // Calculate smoothed normals
    void CalculateSmoothNormals(std::vector<AlembicCompilerVertex>& vertices, GeomCache::Mesh& mesh,
        const Alembic::Abc::Int32ArraySample& faceCounts, const Alembic::Abc::Int32ArraySample& faceIndices,
        const Alembic::Abc::P3fArraySample& positions);

    // Computes tangent space, quantizes vertex positions and fills MeshData
    bool CompileVertices(std::vector<AlembicCompilerVertex>& vertices, GeomCache::Mesh& mesh, GeomCache::MeshData& meshData, const bool bUpdate);

    // Goes through the node tree and update the transform frame dequeues
    void AppendTransformFrameDataRec(GeomCache::Node& node, const uint bufferIndex) const;

    // Cleans up data structures
    void Cleanup();

    // The RC XML parser instance
    ICryXML* m_pXMLParser;

    // Cache root node
    GeomCache::Node m_rootNode;

    // Context
    ConvertContext m_CC;

    // Ref count
    int m_refCount;

    // Flag for 32 bit index format
    bool m_b32BitIndices;

    // Config flags
    bool m_bConvertYUpToZUp;
    bool m_bMeshPrediction;
    bool m_bUseBFrames;
    bool m_bPlaybackFromMemory;
    uint m_indexFrameDistance;
    GeomCacheFile::EBlockCompressionFormat m_blockCompressionFormat;
    double m_positionPrecision;
    float m_uvMax;

    // Time
    std::vector<Alembic::Abc::TimeSampling> m_timeSamplings;
    Alembic::Abc::chrono_t m_minTime;
    Alembic::Abc::chrono_t m_maxTime;
    std::vector<Alembic::Abc::chrono_t> m_frameTimes;

    // Stats
    AZStd::atomic_long m_numVertexSplits;
    LONG m_numExportedMeshes;
    LONG m_numSharedMeshNodes;

    // For error handling
    std::string m_currentObjectPath;

    // List of unique meshes
    std::vector<GeomCache::Mesh*> m_meshes;
    uint m_numAnimatedMeshes;

    // For detecting cloned meshes
    std::unordered_map<AlembicMeshDigest, std::shared_ptr<GeomCache::Mesh> > m_digestToMeshMap;

    // Data for each frame processing
    FrameData m_jobGroupData;

    // Error count
    AZStd::atomic_uint m_errorCount;

};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERABC_ALEMBICCOMPILER_H
