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

#include "ResourceCompilerABC_precompiled.h"
#include "GeomCacheEncoder.h"
#include "../Cry3DEngine/GeomCachePredictors.h"

#include <AzCore/std/parallel/lock.h>

#include <iterator>
#include <tuple>

namespace
{
    // Helper to add blob data to std::vector<uint8>
    template<class T>
    void PushData(std::vector<uint8>& v, const T& d)
    {
        const uint8* pRawData = reinterpret_cast<const uint8*>(&d);
        v.insert(v.end(), pRawData, pRawData + sizeof(T));
    }

    template<class T>
    void PushData(std::vector<uint8>& v, std::vector<T>& d)
    {
        const uint8* pRawData = reinterpret_cast<const uint8*>(d.data());
        v.insert(v.end(), pRawData, pRawData + (d.size() * sizeof(T)));
    }

    template<class T>
    void PushDataPadded(std::vector<uint8>& v, std::vector<T>& d)
    {
        const uint8* pRawData = reinterpret_cast<const uint8*>(d.data());
        v.insert(v.end(), pRawData, pRawData + (d.size() * sizeof(T)));

        while ((v.size() % 16) != 0)
        {
            v.push_back(0);
        }
    }
}

GeomCacheEncoder::GeomCacheEncoder(GeomCacheWriter& geomCacheWriter, GeomCache::Node& rootNode,
    const std::vector<GeomCache::Mesh*>& meshes, const bool bUseBFrames, const uint indexFrameDistance)
    : m_rootNode(rootNode)
    , m_meshes(meshes)
    , m_geomCacheWriter(geomCacheWriter)
    , m_nextFrameIndex(0)
    , m_firstInfoFrameIndex(0)
    , m_bUseBFrames(bUseBFrames)
    , m_indexFrameDistance(indexFrameDistance)
{
    if (m_indexFrameDistance > GeomCacheFile::kMaxIFrameDistance)
    {
        RCLogWarning("Index frame distance clamped to %d", GeomCacheFile::kMaxIFrameDistance);
        m_indexFrameDistance = GeomCacheFile::kMaxIFrameDistance;
    }
}

void GeomCacheEncoder::Init()
{
    m_numNodes = 0;
    CountNodesRec(m_rootNode);
}

void GeomCacheEncoder::CountNodesRec(GeomCache::Node& currentNode)
{
    ++m_numNodes;

    const uint numChildren = currentNode.m_children.size();
    for (uint i = 0; i < numChildren; ++i)
    {
        CountNodesRec(*currentNode.m_children[i]);
    }
}

void GeomCacheEncoder::AddFrame(const Alembic::Abc::chrono_t frameTime, const AABB& aabb, const bool bIsLastFrame)
{
    GeomCacheEncoderFrameInfo* pFrame = new GeomCacheEncoderFrameInfo(this, m_nextFrameIndex, frameTime, aabb, bIsLastFrame);

    if (!m_bUseBFrames || bIsLastFrame || (m_nextFrameIndex % m_indexFrameDistance) == 0)
    {
        pFrame->m_frameType = GeomCacheFile::eFrameType_IFrame;
    }
    else
    {
        pFrame->m_frameType = GeomCacheFile::eFrameType_BFrame;
    }

    ++m_nextFrameIndex;

    EncodeFrame(pFrame);
}

GeomCacheEncoderFrameInfo& GeomCacheEncoder::GetInfoFromFrameIndex(const uint index)
{
    const uint frameInfoOffset = index - m_firstInfoFrameIndex;
    return *m_frames[frameInfoOffset].get();
}

void GeomCacheEncoder::EncodeFrame(GeomCacheEncoderFrameInfo* pFrame)
{
    m_frames.push_back(std::unique_ptr<GeomCacheEncoderFrameInfo>(pFrame));

    pFrame->m_encodeCountdown += m_numNodes;

    EncodeNodesRec(m_rootNode, pFrame);
    EncodeAllMeshes(pFrame);

    FrameEncodeFinished(pFrame);
}

void GeomCacheEncoder::EncodeAllMeshes(GeomCacheEncoderFrameInfo* pFrame)
{
    const uint numMeshes = m_meshes.size();
    int numAnimatedMeshes = 0;
    for (uint i = 0; i < numMeshes; ++i)
    {
        GeomCache::Mesh* pCurrentMesh = m_meshes[i];

        if (pCurrentMesh->m_animatedStreams != 0)
        {
            ++pFrame->m_encodeCountdown;
            EncodeMesh(pCurrentMesh, pFrame);

            // Only need to track the animated meshes we process if we are not the last frame. If we
            // are processing the last frame then we can set the doneCountdown to 0 since we have by
            // this point have encoded all the frames, including the current one.
            if (!pFrame->m_bIsLastFrame)
            {
                numAnimatedMeshes++;
            }
        }
    }
    
    pFrame->m_doneCountdown = numAnimatedMeshes;
}

void GeomCacheEncoder::FrameEncodeFinished([[maybe_unused]] GeomCacheEncoderFrameInfo* pFrame)
{
    // Write ready frames
    const uint numFrames = m_frames.size();
    for (uint i = 0; i < numFrames; ++i)
    {
        GeomCacheEncoderFrameInfo& frame = *m_frames[i].get();
        if (frame.m_encodeCountdown == 0 && !frame.m_bWritten)
        {
            m_geomCacheWriter.WriteFrame(frame.m_frameIndex, frame.m_frameAABB, frame.m_frameType, m_meshes, m_rootNode);
            frame.m_bWritten = true;
        }
    }
    
    // Remove frames that are not needed anymore
    while (m_frames.size() > 0 && m_frames.front()->m_doneCountdown == 0)
    {
        m_frames.pop_front();
        ++m_firstInfoFrameIndex;
    }
}

void GeomCacheEncoder::EncodeNodesRec(GeomCache::Node& currentNode, GeomCacheEncoderFrameInfo* pFrame)
{
    const uint frameIndex = pFrame->m_frameIndex;

    // Get frame to process
    GeomCache::NodeData& rawFrame = currentNode.m_animatedNodeData.front();

    // Add encoded frame
    currentNode.m_encodedFramesCS.lock();
    currentNode.m_encodedFrames.push_back(std::vector<uint8>());
    std::vector<uint8>& output = currentNode.m_encodedFrames.back();
    currentNode.m_encodedFramesCS.unlock();

    // Encode index frame
    EncodeNodeIFrame(currentNode, rawFrame, output);

    assert(pFrame->m_encodeCountdown > 0);
    --pFrame->m_encodeCountdown;
    --pFrame->m_doneCountdown;

    // Remove processed raw frame
    currentNode.m_animatedNodeData.pop_front();

    const uint numChildren = currentNode.m_children.size();
    for (uint i = 0; i < numChildren; ++i)
    {
        EncodeNodesRec(*currentNode.m_children[i], pFrame);
    }
}

void GeomCacheEncoder::EncodeNodeIFrame(const GeomCache::Node& currentNode, const GeomCache::NodeData& nodeData, std::vector<uint8>& output)
{
    uint32 flags = nodeData.m_bVisible ? 0 : GeomCacheFile::eFrameFlags_Hidden;
    PushData(output, flags);

    if (currentNode.m_transformType == GeomCacheFile::eTransformType_Animated)
    {
        QuatTNS transform = nodeData.m_transform;

        if (!nodeData.m_bVisible)
        {
            memset(&transform, 0, sizeof(QuatTNS));
        }

        PushData(output, transform);
    }
}

void GeomCacheEncoder::EncodeMesh(GeomCache::Mesh* pMesh, GeomCacheEncoderFrameInfo* pFrame)
{
    const uint frameIndex = pFrame->m_frameIndex;
    const Alembic::Abc::chrono_t frameTime = pFrame->m_frameTime;

    // Get frame and last index frame to process
    pMesh->m_rawFramesCS.lock();

    // Get raw mesh frame for current frame index
    const uint offset = frameIndex - pMesh->m_firstRawFrameIndex;
    GeomCache::RawMeshFrame& rawMeshFrame = pMesh->m_rawFrames[offset];

    // Search backwards for last encoded index frame
    GeomCache::RawMeshFrame* pLastIFrameRawFrame = nullptr;
    GeomCacheEncoderFrameInfo* pLastIFrameInfo = nullptr;
    uint lastIFrameIndex = 0;

    for (int i = offset; i >= 0; --i)
    {
        const uint currentFrameIndex = pMesh->m_firstRawFrameIndex + i;
        GeomCacheEncoderFrameInfo& frameInfo = GetInfoFromFrameIndex(currentFrameIndex);

        GeomCache::RawMeshFrame& currentFrame = pMesh->m_rawFrames[i];
        if (currentFrame.m_bEncoded && frameInfo.m_frameType == GeomCacheFile::eFrameType_IFrame)
        {
            // Get current raw mesh frame
            pLastIFrameRawFrame = &currentFrame;

            // Get current frame info structure
            lastIFrameIndex = currentFrameIndex;
            pLastIFrameInfo = &frameInfo;

            break;
        }
    }

    pMesh->m_rawFramesCS.unlock();

    if (pLastIFrameInfo && pLastIFrameRawFrame)
    {
        const uint frameDelta = frameIndex - pLastIFrameInfo->m_frameIndex;
        const Alembic::Abc::chrono_t timeDelta = frameTime - pLastIFrameInfo->m_frameTime;

        if (pFrame->m_frameType == GeomCacheFile::eFrameType_IFrame)
        {
            // Don't need the last index frame anymore after this
            pLastIFrameRawFrame->m_bDone = true;
            GeomCache::RawMeshFrame* pPrevFrames[2] = { NULL, pLastIFrameRawFrame };
            --pLastIFrameInfo->m_doneCountdown;

            // Compress frames in between as b frames
            for (uint bFrameIndex = lastIFrameIndex + 1; bFrameIndex < frameIndex; ++bFrameIndex)
            {
                assert(m_bUseBFrames);

                pMesh->m_encodedFramesCS.lock();
                pMesh->m_encodedFrames.push_back(std::vector<uint8>());
                std::vector<uint8>& output = pMesh->m_encodedFrames.back();
                pMesh->m_encodedFramesCS.unlock();

                const uint offset2 = bFrameIndex - pMesh->m_firstRawFrameIndex;
                GeomCache::RawMeshFrame& rawMeshBFrame = pMesh->m_rawFrames[offset2];
                EncodeMeshBFrame(*pMesh, rawMeshBFrame, pPrevFrames, *pLastIFrameRawFrame, rawMeshFrame, output);
                rawMeshBFrame.m_bDone = true;

                GeomCacheEncoderFrameInfo& bFrameInfo = GetInfoFromFrameIndex(bFrameIndex);
                --bFrameInfo.m_encodeCountdown;
                --bFrameInfo.m_doneCountdown;

                pPrevFrames[0] = pPrevFrames[1];
                pPrevFrames[1] = (rawMeshBFrame.m_frameUseCount > 0) ? &rawMeshBFrame : NULL;
            }

            // Encode current frame as index frame
            pMesh->m_encodedFramesCS.lock();
            pMesh->m_encodedFrames.push_back(std::vector<uint8>());
            std::vector<uint8>& output = pMesh->m_encodedFrames.back();
            pMesh->m_encodedFramesCS.unlock();

            EncodeMeshIFrame(*pMesh, rawMeshFrame, output);
            GeomCacheEncoderFrameInfo& frameInfo = GetInfoFromFrameIndex(frameIndex);
            --frameInfo.m_encodeCountdown;

            // Remove unneeded frames. Don't remove last frame, because we might still need it for velocity vectors.
            // Don't need to care if there are frames left in the end, it will be killed with the mesh data structure.
            if (frameIndex > 1)
            {
                pMesh->m_rawFramesCS.lock();
                for (uint i = 0; (m_firstInfoFrameIndex + i) < (frameIndex - 2); ++i)
                {
                    if (pMesh->m_rawFrames.front().m_bDone)
                    {
                        pMesh->m_rawFrames.pop_front();
                        ++pMesh->m_firstRawFrameIndex;
                    }
                }
                pMesh->m_rawFramesCS.unlock();
            }
        }
    }
    else
    {
        // No previous index frame, encode as first index frame
        assert(frameIndex == 0);

        pMesh->m_encodedFramesCS.lock();
        pMesh->m_encodedFrames.push_back(std::vector<uint8>());
        std::vector<uint8>& output = pMesh->m_encodedFrames.back();
        pMesh->m_encodedFramesCS.unlock();

        EncodeMeshIFrame(*pMesh, rawMeshFrame, output);
        --GetInfoFromFrameIndex(frameIndex).m_encodeCountdown;
    }
}

void GeomCacheEncoder::EncodeMeshIFrame(GeomCache::Mesh& mesh, GeomCache::RawMeshFrame& rawMeshFrame, std::vector<uint8>& output)
{
    const bool bMeshVisible = rawMeshFrame.m_frameUseCount > 0;
    const bool bUsePrediction = mesh.m_bUsePredictor;

    STATIC_ASSERT((sizeof(GeomCacheFile::SMeshFrameHeader) % 16) == 0, "SMeshFrameHeader size must be a multiple of 16");

    GeomCacheFile::SMeshFrameHeader frameHeader;
    memset(&frameHeader, 0, sizeof(GeomCacheFile::SMeshFrameHeader));
    frameHeader.m_flags = bMeshVisible ? 0 : GeomCacheFile::eFrameFlags_Hidden;

    rawMeshFrame.m_bEncoded = true;

    const GeomCacheFile::EStreams streamMask = mesh.m_animatedStreams;

    bool bWroteStream = false;
    GeomCache::MeshData& meshData = rawMeshFrame.m_meshData;
    uint numElements = meshData.m_positions.size();

    std::vector<uint8> frameData;

    if (streamMask & GeomCacheFile::eStream_Positions)
    {
        if (bUsePrediction)
        {
            std::vector<GeomCacheFile::Position> encodedPositions(numElements);
            GeomCachePredictors::ParallelogramPredictor<GeomCacheFile::Position, true>(numElements, meshData.m_positions.data(), encodedPositions.data(), mesh.m_predictorData);

#ifdef _DEBUG
            // Make sure the encoding is reversible
            std::vector<GeomCacheFile::Position> decodedPositions(numElements);
            GeomCachePredictors::ParallelogramPredictor<GeomCacheFile::Position, false>(numElements, encodedPositions.data(), decodedPositions.data(), mesh.m_predictorData);
            assert(decodedPositions == meshData.m_positions);
#endif

            PushDataPadded(frameData, encodedPositions);
        }
        else
        {
            PushDataPadded(frameData, meshData.m_positions);
        }

        bWroteStream = true;
        numElements = meshData.m_positions.size();
    }

    if (streamMask & GeomCacheFile::eStream_Texcoords)
    {
        assert(!bWroteStream || meshData.m_texcoords.size() == numElements);

        if (bUsePrediction)
        {
            std::vector<GeomCacheFile::Texcoords> encodedTexcoords(numElements);
            GeomCachePredictors::ParallelogramPredictor<GeomCacheFile::Texcoords, true>(numElements, meshData.m_texcoords.data(), encodedTexcoords.data(), mesh.m_predictorData);

#ifdef _DEBUG
            // Make sure the encoding is reversible
            std::vector<GeomCacheFile::Texcoords> decodedTexcoords(numElements);
            GeomCachePredictors::ParallelogramPredictor<GeomCacheFile::Texcoords, false>(numElements, encodedTexcoords.data(), decodedTexcoords.data(), mesh.m_predictorData);
            assert(decodedTexcoords == meshData.m_texcoords);
#endif

            PushDataPadded(frameData, encodedTexcoords);
        }
        else
        {
            PushDataPadded(frameData, meshData.m_texcoords);
        }

        bWroteStream = true;
        numElements = meshData.m_texcoords.size();
    }

    if (streamMask & GeomCacheFile::eStream_QTangents)
    {
        assert(!bWroteStream || meshData.m_qTangents.size() == numElements);

        if (bUsePrediction)
        {
            std::vector<GeomCacheFile::QTangent> encodedQTangents(numElements);
            GeomCachePredictors::QTangentPredictor<true>(numElements, meshData.m_qTangents.data(), encodedQTangents.data(), mesh.m_predictorData);

#ifdef _DEBUG
            // Make sure the encoding is reversible
            std::vector<GeomCacheFile::QTangent> decodedQTangents(numElements);
            GeomCachePredictors::QTangentPredictor<false>(numElements, encodedQTangents.data(), decodedQTangents.data(), mesh.m_predictorData);
            assert(decodedQTangents == meshData.m_qTangents);
#endif

            PushDataPadded(frameData, encodedQTangents);
        }
        else
        {
            PushDataPadded(frameData, meshData.m_qTangents);
        }

        bWroteStream = true;
        numElements = meshData.m_qTangents.size();
    }

    if (streamMask & GeomCacheFile::eStream_Colors)
    {
        assert(!bWroteStream || meshData.m_reds.size() == numElements);
        assert(!bWroteStream || meshData.m_greens.size() == numElements);
        assert(!bWroteStream || meshData.m_blues.size() == numElements);

        if (bUsePrediction)
        {
            std::vector<GeomCacheFile::Color> encodedReds(numElements);
            GeomCachePredictors::ColorPredictor<true>(numElements, meshData.m_reds.data(), encodedReds.data(), mesh.m_predictorData);
            std::vector<GeomCacheFile::Color> encodedGreens(numElements);
            GeomCachePredictors::ColorPredictor<true>(numElements, meshData.m_greens.data(), encodedGreens.data(), mesh.m_predictorData);
            std::vector<GeomCacheFile::Color> encodedBlues(numElements);
            GeomCachePredictors::ColorPredictor<true>(numElements, meshData.m_blues.data(), encodedBlues.data(), mesh.m_predictorData);
            std::vector<GeomCacheFile::Color> encodedAlphas(numElements);
            GeomCachePredictors::ColorPredictor<true>(numElements, meshData.m_alphas.data(), encodedAlphas.data(), mesh.m_predictorData);

#ifdef _DEBUG
            // Make sure the encoding is reversible
            std::vector<GeomCacheFile::Color> decodedReds(numElements);
            GeomCachePredictors::ColorPredictor<false>(numElements, encodedReds.data(), decodedReds.data(), mesh.m_predictorData);
            std::vector<GeomCacheFile::Color> decodedGreens(numElements);
            GeomCachePredictors::ColorPredictor<false>(numElements, encodedGreens.data(), decodedGreens.data(), mesh.m_predictorData);
            std::vector<GeomCacheFile::Color> decodedBlues(numElements);
            GeomCachePredictors::ColorPredictor<false>(numElements, encodedBlues.data(), decodedBlues.data(), mesh.m_predictorData);
            std::vector<GeomCacheFile::Color> decodedAlphas(numElements);
            GeomCachePredictors::ColorPredictor<false>(numElements, encodedAlphas.data(), decodedAlphas.data(), mesh.m_predictorData);

            assert(decodedReds == meshData.m_reds);
            assert(decodedGreens == meshData.m_greens);
            assert(decodedBlues == meshData.m_blues);
            assert(decodedAlphas == meshData.m_alphas);
#endif

            PushDataPadded(frameData, encodedReds);
            PushDataPadded(frameData, encodedGreens);
            PushDataPadded(frameData, encodedBlues);
            PushDataPadded(frameData, encodedAlphas);
        }
        else
        {
            PushDataPadded(frameData, meshData.m_reds);
            PushDataPadded(frameData, meshData.m_greens);
            PushDataPadded(frameData, meshData.m_blues);
            PushDataPadded(frameData, meshData.m_alphas);
        }

        bWroteStream = true;
        numElements = meshData.m_reds.size();
    }

    // If mesh is not visible and we are not using b frame we can zero out the
    // complete index frame data resulting in almost no data stored after
    // range/entropy coding
    if (!bMeshVisible && !m_bUseBFrames)
    {
        std::fill(frameData.begin(), frameData.end(), 0);
    }

    PushData(output, frameHeader);
    PushData(output, frameData);
}

namespace
{
    template<class T>
    float Entropy(const uint numElements, const T* pData)
    {
        const uint8* pRawData = reinterpret_cast<const uint8*>(pData);

        size_t symbolCounts[256];
        std::fill(symbolCounts, symbolCounts + 256, 0);

        const size_t numBytes = numElements * sizeof(T);
        for (size_t i = 0; i < numBytes; ++i)
        {
            ++symbolCounts[pRawData[i]];
        }

        float sum = 0.0f;
        for (uint i = 0; i < 256; ++i)
        {
            if (symbolCounts[i] > 0)
            {
                const float symbolPropability = ((float)symbolCounts[i] / (float)numBytes);
                sum += symbolPropability * (log(symbolPropability) / log(2.0f));
            }
        }

        return -sum;
    }

    template<class T>
    uint8 BinarySearchPredictor(const uint numElements, std::function<const T*(uint8)> predictor)
    {
        uint8 min = std::numeric_limits<uint8>::min();
        uint8 max = std::numeric_limits<uint8>::max();

        const T* pLeft = predictor(min);
        float leftEntropy = Entropy(numElements, pLeft);
        const T* pRight = predictor(max);
        float rightEntropy = Entropy(numElements, pRight);

        while (min != max)
        {
            if (leftEntropy < rightEntropy)
            {
                max -= (max - min + 1) / 2;
                rightEntropy = Entropy(numElements, predictor(max));
            }
            else
            {
                min += (max - min + 1) / 2;
                leftEntropy = Entropy(numElements, predictor(min));
            }

            assert(max >= min);
        }

        return min;
    }

    template<class I, class T>
    void TemporalPredictorEncode(GeomCacheFile::STemporalPredictorControl& controlOut, const GeomCachePredictors::STemporalPredictorData<T>& data, const T* pIn, T* pOut)
    {
        const uint numElements = data.m_numElements;

        // Search for the best interpolate predictor value
        controlOut.m_indexFrameLerpFactor = BinarySearchPredictor<T>(numElements, [&](uint8 lerpFactor) -> const T*
                {
                    GeomCachePredictors::InterpolateDeltaEncode<I, T>(numElements, lerpFactor, data.m_pFloorFrame, data.m_pCeilFrame, pIn, pOut);
                    return pOut;
                });

        if (data.m_pPrevFrames[0] && data.m_pPrevFrames[1])
        {
            // Search for the best motion predictor value
            controlOut.m_acceleration = BinarySearchPredictor<T>(numElements, [&](uint8 acceleration) -> const T*
                    {
                        GeomCachePredictors::MotionDeltaEncode<I, T>(numElements, acceleration, data.m_pPrevFrames, pIn, pOut);
                        return pOut;
                    });
        }
        else
        {
            controlOut.m_acceleration = 0;
            controlOut.m_combineFactor = 0;
            return;
        }

        // Finally search for the best combination
        controlOut.m_acceleration = BinarySearchPredictor<T>(numElements, [&](uint8 combineFactor) -> const T*
                {
                    controlOut.m_combineFactor = combineFactor;
                    GeomCachePredictors::InterpolateMotionDeltaPredictor<I, T, true>(controlOut, data, pIn, pOut);
                    return pOut;
                });

        GeomCachePredictors::InterpolateMotionDeltaPredictor<I, T, true>(controlOut, data, pIn, pOut);

#ifdef _DEBUG
        // Make sure the encoding is reversible
        std::vector<T> decoded(numElements);
        GeomCachePredictors::InterpolateMotionDeltaPredictor<I, T, false>(controlOut, data, pOut, decoded.data());
        const T* pDecoded = decoded.data();
        assert(memcmp(pDecoded, pIn, sizeof(T) * numElements) == 0);
#endif
    }
}

void GeomCacheEncoder::EncodeMeshBFrame(GeomCache::Mesh& mesh, GeomCache::RawMeshFrame& rawMeshFrame,
    GeomCache::RawMeshFrame* pPrevFrames[2], GeomCache::RawMeshFrame& floorIndexFrame,
    GeomCache::RawMeshFrame& ceilIndexFrame, std::vector<uint8>& output)
{
    const bool bMeshVisible = rawMeshFrame.m_frameUseCount > 0;

    GeomCacheFile::SMeshFrameHeader frameHeader;
    memset(&frameHeader, 0, sizeof(GeomCacheFile::SMeshFrameHeader));
    frameHeader.m_flags = bMeshVisible ? 0 : GeomCacheFile::eFrameFlags_Hidden;

    rawMeshFrame.m_bEncoded = true;

    const GeomCacheFile::EStreams streamMask = mesh.m_animatedStreams;
    GeomCache::MeshData& meshData = rawMeshFrame.m_meshData;
    uint numElements = meshData.m_positions.size();

    std::vector<uint8> frameData;

    const bool bCanMotionPredict = pPrevFrames[0] && pPrevFrames[1];

    if (streamMask & GeomCacheFile::eStream_Positions)
    {
        GeomCachePredictors::STemporalPredictorData<GeomCacheFile::Position> predictorData;
        predictorData.m_numElements = numElements;
        predictorData.m_pFloorFrame = floorIndexFrame.m_meshData.m_positions.data();
        predictorData.m_pCeilFrame = ceilIndexFrame.m_meshData.m_positions.data();
        predictorData.m_pPrevFrames[0] = pPrevFrames[0] ? pPrevFrames[0]->m_meshData.m_positions.data() : nullptr;
        predictorData.m_pPrevFrames[1] = pPrevFrames[1] ? pPrevFrames[1]->m_meshData.m_positions.data() : nullptr;

        std::vector<GeomCacheFile::Position> predictedPositions(numElements);

        typedef Vec3_tpl<uint32> I;
        TemporalPredictorEncode<I>(frameHeader.m_positionStreamPredictorControl, predictorData,
            rawMeshFrame.m_meshData.m_positions.data(), predictedPositions.data());

        PushDataPadded(frameData, predictedPositions);
    }

    if (streamMask & GeomCacheFile::eStream_Texcoords)
    {
        GeomCachePredictors::STemporalPredictorData<GeomCacheFile::Texcoords> predictorData;
        predictorData.m_numElements = numElements;
        predictorData.m_pFloorFrame = floorIndexFrame.m_meshData.m_texcoords.data();
        predictorData.m_pCeilFrame = ceilIndexFrame.m_meshData.m_texcoords.data();
        predictorData.m_pPrevFrames[0] = pPrevFrames[0] ? pPrevFrames[0]->m_meshData.m_texcoords.data() : nullptr;
        predictorData.m_pPrevFrames[1] = pPrevFrames[1] ? pPrevFrames[1]->m_meshData.m_texcoords.data() : nullptr;

        std::vector<GeomCacheFile::Texcoords> predictedTexcoords(numElements);
        typedef Vec2_tpl<uint32> I;
        TemporalPredictorEncode<I>(frameHeader.m_texcoordStreamPredictorControl, predictorData,
            rawMeshFrame.m_meshData.m_texcoords.data(), predictedTexcoords.data());

        PushDataPadded(frameData, predictedTexcoords);
    }

    if (streamMask & GeomCacheFile::eStream_QTangents)
    {
        GeomCachePredictors::STemporalPredictorData<GeomCacheFile::QTangent> predictorData;
        predictorData.m_numElements = numElements;
        predictorData.m_pFloorFrame = floorIndexFrame.m_meshData.m_qTangents.data();
        predictorData.m_pCeilFrame = ceilIndexFrame.m_meshData.m_qTangents.data();
        predictorData.m_pPrevFrames[0] = pPrevFrames[0] ? pPrevFrames[0]->m_meshData.m_qTangents.data() : nullptr;
        predictorData.m_pPrevFrames[1] = pPrevFrames[1] ? pPrevFrames[1]->m_meshData.m_qTangents.data() : nullptr;

        std::vector<GeomCacheFile::QTangent> predictedQTangents(numElements);
        typedef Vec4_tpl<uint32> I;
        TemporalPredictorEncode<I>(frameHeader.m_qTangentStreamPredictorControl, predictorData,
            rawMeshFrame.m_meshData.m_qTangents.data(), predictedQTangents.data());

        PushDataPadded(frameData, predictedQTangents);
    }

    if (streamMask & GeomCacheFile::eStream_Colors)
    {
        {
            GeomCachePredictors::STemporalPredictorData<GeomCacheFile::Color> predictorData;
            predictorData.m_numElements = numElements;
            predictorData.m_pFloorFrame = floorIndexFrame.m_meshData.m_reds.data();
            predictorData.m_pCeilFrame = ceilIndexFrame.m_meshData.m_reds.data();
            predictorData.m_pPrevFrames[0] = pPrevFrames[0] ? pPrevFrames[0]->m_meshData.m_reds.data() : nullptr;
            predictorData.m_pPrevFrames[1] = pPrevFrames[1] ? pPrevFrames[1]->m_meshData.m_reds.data() : nullptr;

            std::vector<GeomCacheFile::Color> predictedReds(numElements);
            typedef uint16 I;
            TemporalPredictorEncode<I>(frameHeader.m_colorStreamPredictorControl[0], predictorData,
                rawMeshFrame.m_meshData.m_reds.data(), predictedReds.data());

            PushDataPadded(frameData, predictedReds);
        }

        {
            GeomCachePredictors::STemporalPredictorData<GeomCacheFile::Color> predictorData;
            predictorData.m_numElements = numElements;
            predictorData.m_pFloorFrame = floorIndexFrame.m_meshData.m_greens.data();
            predictorData.m_pCeilFrame = ceilIndexFrame.m_meshData.m_greens.data();
            predictorData.m_pPrevFrames[0] = pPrevFrames[0] ? pPrevFrames[0]->m_meshData.m_greens.data() : nullptr;
            predictorData.m_pPrevFrames[1] = pPrevFrames[1] ? pPrevFrames[1]->m_meshData.m_greens.data() : nullptr;

            std::vector<GeomCacheFile::Color> predictedGreens(numElements);
            typedef uint16 I;
            TemporalPredictorEncode<I>(frameHeader.m_colorStreamPredictorControl[1], predictorData,
                rawMeshFrame.m_meshData.m_greens.data(), predictedGreens.data());

            PushDataPadded(frameData, predictedGreens);
        }

        {
            GeomCachePredictors::STemporalPredictorData<GeomCacheFile::Color> predictorData;
            predictorData.m_numElements = numElements;
            predictorData.m_pFloorFrame = floorIndexFrame.m_meshData.m_blues.data();
            predictorData.m_pCeilFrame = ceilIndexFrame.m_meshData.m_blues.data();
            predictorData.m_pPrevFrames[0] = pPrevFrames[0] ? pPrevFrames[0]->m_meshData.m_blues.data() : nullptr;
            predictorData.m_pPrevFrames[1] = pPrevFrames[1] ? pPrevFrames[1]->m_meshData.m_blues.data() : nullptr;

            std::vector<GeomCacheFile::Color> predictedBlues(numElements);
            typedef uint16 I;
            TemporalPredictorEncode<I>(frameHeader.m_colorStreamPredictorControl[2], predictorData,
                rawMeshFrame.m_meshData.m_blues.data(), predictedBlues.data());

            PushDataPadded(frameData, predictedBlues);
        }

        {
            GeomCachePredictors::STemporalPredictorData<GeomCacheFile::Color> predictorData;
            predictorData.m_numElements = numElements;
            predictorData.m_pFloorFrame = floorIndexFrame.m_meshData.m_alphas.data();
            predictorData.m_pCeilFrame = ceilIndexFrame.m_meshData.m_alphas.data();
            predictorData.m_pPrevFrames[0] = pPrevFrames[0] ? pPrevFrames[0]->m_meshData.m_alphas.data() : nullptr;
            predictorData.m_pPrevFrames[1] = pPrevFrames[1] ? pPrevFrames[1]->m_meshData.m_alphas.data() : nullptr;

            std::vector<GeomCacheFile::Color> predictedAlphas(numElements);
            typedef uint16 I;
            TemporalPredictorEncode<I>(frameHeader.m_colorStreamPredictorControl[3], predictorData,
                rawMeshFrame.m_meshData.m_alphas.data(), predictedAlphas.data());

            PushDataPadded(frameData, predictedAlphas);
        }
    }

    // If mesh is not visible we can zero out the complete frame data
    // resulting in almost no data stored after range/entropy coding
    if (!bMeshVisible)
    {
        std::fill(frameData.begin(), frameData.end(), 0);
    }

    PushData(output, frameHeader);
    PushData(output, frameData);
}

namespace std
{
    // Needed for unordered map. It's beyond me why this isn't included in the STL.
    template<class T1, class T2>
    class hash<std::pair<T1, T2> >
    {
    public:
        size_t operator()(const std::pair<T1, T2>& pair) const
        {
            std::hash<T1> hasherT1;
            std::hash<T2> hasherT2;
            size_t hash1 = hasherT1(pair.first);
            return hasherT2(pair.second) + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2);
        }
    };
}

namespace
{
    typedef std::unordered_map<uint32, uint32> ReorderMap;

    template<class T>
    void ReorderVector(std::vector<T>& vector, const ReorderMap& reorderMap)
    {
        std::vector<T> oldVector = vector;

        const uint numElements = oldVector.size();
        for (uint i = 0; i < numElements; ++i)
        {
            assert(reorderMap.find(i) != reorderMap.end());
            const uint32 newIndex = reorderMap.find(i)->second;
            vector[newIndex] = oldVector[i];
        }
    }
}

// Optimizes the given mesh for frame compression
bool GeomCacheEncoder::OptimizeMeshForCompression(GeomCache::Mesh& mesh, const bool bUseMeshPrediction)
{
    // Reorder vertices based on first use in index arrays
    ReorderMap reorderMap;

    uint32 currentNewIndex = 0;
    for (auto iter = mesh.m_indicesMap.begin(); iter != mesh.m_indicesMap.end(); ++iter)
    {
        std::vector<uint32>& materialIndices = iter->second;

        const uint numMaterialIndices = materialIndices.size();
        for (uint i = 0; i < numMaterialIndices; ++i)
        {
            const uint32 oldIndex = materialIndices[i];
            if (reorderMap.find(oldIndex) == reorderMap.end())
            {
                reorderMap[oldIndex] = currentNewIndex++;
            }
        }
    }

    for (auto iter = mesh.m_indicesMap.begin(); iter != mesh.m_indicesMap.end(); ++iter)
    {
        std::vector<uint32>& materialIndices = iter->second;

        const uint numMaterialIndices = materialIndices.size();
        for (uint i = 0; i < numMaterialIndices; ++i)
        {
            assert(reorderMap.find(materialIndices[i]) != reorderMap.end());
            materialIndices[i] = reorderMap[materialIndices[i]];
        }
    }

    GeomCache::MeshData& staticMeshData = mesh.m_staticMeshData;

    ReorderVector<>(staticMeshData.m_positions, reorderMap);
    ReorderVector<>(staticMeshData.m_texcoords, reorderMap);
    ReorderVector<>(staticMeshData.m_qTangents, reorderMap);
    ReorderVector<>(staticMeshData.m_reds, reorderMap);
    ReorderVector<>(staticMeshData.m_greens, reorderMap);
    ReorderVector<>(staticMeshData.m_blues, reorderMap);
    ReorderVector<>(staticMeshData.m_alphas, reorderMap);
    ReorderVector<>(mesh.m_reflections, reorderMap);

    uint numIndexToGeomCacheIndex = mesh.m_abcIndexToGeomCacheIndex.size();
    for (uint i = 0; i < numIndexToGeomCacheIndex; ++i)
    {
        const uint32 oldIndex = mesh.m_abcIndexToGeomCacheIndex[i];
        assert(reorderMap.find(oldIndex) != reorderMap.end());
        const uint32 newIndex = reorderMap.find(oldIndex)->second;
        mesh.m_abcIndexToGeomCacheIndex[i] = newIndex;
    }

    if (!bUseMeshPrediction)
    {
        mesh.m_bUsePredictor = false;
        return true;
    }

    // Create map of neighbor indices for each index
    std::unordered_map<uint32, std::vector<uint32> > neighborIndexMap;
    for (auto iter = mesh.m_indicesMap.begin(); iter != mesh.m_indicesMap.end(); ++iter)
    {
        std::vector<uint32>& materialIndices = iter->second;

        const uint numMaterialIndices = materialIndices.size();
        for (uint i = 0; i < numMaterialIndices; i += 3)
        {
            const uint32 index1 = materialIndices[i];
            const uint32 index2 = materialIndices[i + 1];
            const uint32 index3 = materialIndices[i + 2];

            stl::push_back_unique(neighborIndexMap[index1], index2);
            stl::push_back_unique(neighborIndexMap[index2], index1);
            stl::push_back_unique(neighborIndexMap[index2], index3);
            stl::push_back_unique(neighborIndexMap[index3], index2);
            stl::push_back_unique(neighborIndexMap[index3], index1);
            stl::push_back_unique(neighborIndexMap[index1], index3);
        }
    }

    // Sort neighbor arrays for fast set intersection
    for (auto iter = neighborIndexMap.begin(); iter != neighborIndexMap.end(); ++iter)
    {
        std::sort(iter->second.begin(), iter->second.end());
    }

    uint foundNeighborCount = 0;
    uint foundNoNeighborCount = 0;

    // Create data structure for mesh predictor
    STATIC_ASSERT(GeomCacheFile::kMeshPredictorLookBackMaxDist < 0xFFFF, "kMeshPredictorLookBackArraySize must be smaller than 0xFF");

    uint64 averageDelta = 0;
    const uint numPositions = staticMeshData.m_positions.size();
    std::vector<uint32> intersection;
    std::vector<std::tuple<uint, uint, uint> > foundNeighborTris;

    for (uint currentIndex = 0; currentIndex < numPositions; ++currentIndex)
    {
        // Look up neighbors for this vertex index
        std::vector<uint32>& neighbors = neighborIndexMap[currentIndex];
        const uint numNeighbors = neighbors.size();

        // Array of adjacent neighbor triangles
        foundNeighborTris.clear();

        bool bFoundNeighborTriangle = false;
        for (uint i = 0; i < numNeighbors; ++i)
        {
            const uint neighborIndex = neighbors[i];
            assert(neighborIndex != currentIndex);

            // check if neighbor index is in the predictors look back range
            const uint neighborIndexDistance = currentIndex - neighborIndex;

            if (neighborIndex < currentIndex && neighborIndexDistance <= GeomCacheFile::kMeshPredictorLookBackMaxDist)
            {
                std::vector<uint32>& neighborNeighbors = neighborIndexMap[neighborIndex];
                const uint numNeighborNeighbors = neighborNeighbors.size();

                for (uint j = 0; j < numNeighborNeighbors; ++j)
                {
                    // Check neighbor neighbor index is also a neighbor of currentIndex and if it is in range
                    const uint neighborNeighborIndex = neighborNeighbors[j];
                    const uint neighborNeighborIndexDistance = currentIndex - neighborNeighborIndex;

                    if (neighborNeighborIndexDistance < currentIndex
                        && neighborNeighborIndexDistance <= GeomCacheFile::kMeshPredictorLookBackMaxDist
                        && neighborNeighborIndex != i && stl::find(neighbors, neighborNeighborIndex))
                    {
                        // Now we have two indices that are neighbors of currentIndex and are themselves neighbors
                        // Check if they share a common neighbor that is not currentIndex. If so we have a neighbor triangle.
                        std::vector<uint32>& neighborNeighborNeighbors = neighborIndexMap[neighborNeighborIndex];

                        intersection.clear();
                        std::set_intersection(neighborNeighborNeighbors.begin(), neighborNeighborNeighbors.end(),
                            neighborNeighbors.begin(), neighborNeighbors.end(), std::back_inserter(intersection));

                        uint numSharedIndices = intersection.size();
                        for (uint k = 0; k < numSharedIndices; ++k)
                        {
                            const uint32 sharedIndex = intersection[k];

                            const uint sharedIndexDistance = currentIndex - sharedIndex;
                            if (sharedIndexDistance < currentIndex
                                && sharedIndexDistance <= GeomCacheFile::kMeshPredictorLookBackMaxDist
                                && sharedIndex != currentIndex)
                            {
                                foundNeighborTris.push_back(std::make_tuple<>(neighborIndexDistance, neighborNeighborIndexDistance, sharedIndexDistance));
                            }
                        }
                    }
                }
            }

            // Find best neighbor triangle for parallelogram prediction
            const uint numNeighbors2 = foundNeighborTris.size();
            if (numNeighbors2 > 0)
            {
                bFoundNeighborTriangle = true;
                ++foundNeighborCount;
                uint preferedNeighbor = 0;
                uint bestDelta = std::numeric_limits<uint>::max();

                for (uint j = 0; j < numNeighbors2; ++j)
                {
                    const uint uDist = std::get<0>(foundNeighborTris[j]);
                    const uint vDist = std::get<1>(foundNeighborTris[j]);
                    const uint wDist = std::get<2>(foundNeighborTris[j]);

                    GeomCacheFile::Position& u = staticMeshData.m_positions[currentIndex - uDist];
                    GeomCacheFile::Position& v = staticMeshData.m_positions[currentIndex - vDist];
                    GeomCacheFile::Position& w = staticMeshData.m_positions[currentIndex - wDist];

                    GeomCacheFile::Position& realPosition = staticMeshData.m_positions[currentIndex];
                    GeomCacheFile::Position predictedPosition = u + v - w;

                    uint delta = std::abs((int32)realPosition.x - (int32)predictedPosition.x)
                        + std::abs((int32)realPosition.y - (int32)predictedPosition.y)
                        + std::abs((int32)realPosition.z - (int32)predictedPosition.z);

                    if (delta < bestDelta)
                    {
                        preferedNeighbor = j;
                        bestDelta = delta;
                    }
                }

                averageDelta += bestDelta;

                mesh.m_predictorData.push_back(std::get<0>(foundNeighborTris[preferedNeighbor]));
                mesh.m_predictorData.push_back(std::get<1>(foundNeighborTris[preferedNeighbor]));
                mesh.m_predictorData.push_back(std::get<2>(foundNeighborTris[preferedNeighbor]));
                break;
            }
        }

        if (!bFoundNeighborTriangle)
        {
            mesh.m_predictorData.push_back(0xFFFF);
            ++foundNoNeighborCount;
        }
    }

    averageDelta /= numPositions;

    // At least first vertices can't have a valid neighbor triangle
    assert(foundNoNeighborCount >= 3);

    bool bBadConnectivity = ((double)foundNeighborCount / (double)numPositions) < 0.5;

    if (bBadConnectivity)
    {
        RCLogWarning("Less than 50%% of the vertices in mesh %s have a triangle neighbor. Mesh prediction could be impaired.", mesh.m_abcMesh.getName().c_str());
    }

    mesh.m_bUsePredictor = true;

    return true;
}
