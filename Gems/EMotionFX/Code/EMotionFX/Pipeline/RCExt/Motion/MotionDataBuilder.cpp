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

#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IAnimationData.h>

#include <SceneAPIExt/Groups/IMotionGroup.h>
#include <SceneAPIExt/Rules/IMotionScaleRule.h>
#include <SceneAPIExt/Rules/CoordinateSystemRule.h>
#include <SceneAPIExt/Rules/MotionRangeRule.h>
#include <SceneAPIExt/Rules/MotionAdditiveRule.h>
#include <SceneAPIExt/Rules/MotionSamplingRule.h>
#include <RCExt/Motion/MotionDataBuilder.h>
#include <RCExt/ExportContexts.h>
#include <RCExt/CoordinateSystemConverter.h>

#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionData/MotionDataFactory.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <EMotionFX/Source/MotionData/UniformMotionData.h>
#include <MCore/Source/AzCoreConversions.h>

#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/containers/array.h>
#include <AzToolsFramework/Debug/TraceContext.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneUtil = AZ::SceneAPI::Utilities;
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneViews = AZ::SceneAPI::Containers::Views;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;

        MotionDataBuilder::MotionDataBuilder()
        {
            BindToCall(&MotionDataBuilder::BuildMotionData);
        }

        void MotionDataBuilder::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<MotionDataBuilder, AZ::SceneAPI::SceneCore::ExportingComponent>()->Version(1);
            }
        }

        void InitAndOptimizeMotionData(MotionData* finalMotionData, const NonUniformMotionData* sourceMotionData, float sampleRate, const Rule::MotionSamplingRule* samplingRule, const AZStd::vector<size_t>& rootJoints)
        {
            // Init and resample.
            finalMotionData->InitFromNonUniformData(
                sourceMotionData,
                /*keepSameSampleRate=*/false,
                /*newSampleRate=*/sampleRate,
                samplingRule ? !samplingRule->GetKeepDuration() : false);

            // Get the quality in percentages.
            const float translationQualityPercentage = samplingRule ? AZ::GetClamp(samplingRule->GetTranslationQualityPercentage(), 1.0f, 100.0f) : 75.0f;
            const float rotationQualityPercentage = samplingRule ? AZ::GetClamp(samplingRule->GetRotationQualityPercentage(), 1.0f, 100.0f) : 75.0f;
            const float scaleQualityPercentage = samplingRule ? AZ::GetClamp(samplingRule->GetScaleQualityPercentage(), 1.0f, 100.0f) : 75.0f;

            // Perform keyframe optimization.
            MotionData::OptimizeSettings optimizeSettings;
            optimizeSettings.m_maxPosError = AZ::Lerp(0.0225f, 0.0f, (translationQualityPercentage - 1.0f) / 99.0f); // The percentage is remapped from 1.100 to 0..99, as otherwise a percentage of 1 doesn't start at the start of the lerp.
            optimizeSettings.m_maxRotError = AZ::Lerp(0.0225f, 0.0f, (rotationQualityPercentage - 1.0f) / 99.0f);
            optimizeSettings.m_maxScaleError = AZ::Lerp(0.0225f, 0.0f, (scaleQualityPercentage - 1.0f) / 99.0f);

            optimizeSettings.m_maxFloatError = 0.0001f;
            optimizeSettings.m_maxMorphError = 0.0001f;
            optimizeSettings.m_jointIgnoreList = rootJoints; // Skip optimizing root joints, as that makes the feet jitter.
            optimizeSettings.m_updateDuration = samplingRule ? !samplingRule->GetKeepDuration() : false;
            finalMotionData->Optimize(optimizeSettings);
        }

        // Automatically determine what produces the smallest memory footprint motion data, either UniformMotionData or NonUniformMotionData.
        // We don't iterate through all registered motion data types, because we dont know if smaller memory footprint is always better.
        // However, when we pick between Uniform or NonUniform, we always want Uniform if that's smaller in size, as it gives higher performance and is smaller in memory footprint.
        // Later on we can add more automatic modes, where we always find the smallest size between all, or the higher performance one.
        MotionData* AutoCreateMotionData(const NonUniformMotionData* sourceMotionData, float sampleRate, const Rule::MotionSamplingRule* samplingRule, const AZStd::vector<size_t>& rootJoints)
        {
            MotionData* finalMotionData = nullptr;

            AZ_TracePrintf("EMotionFX", "*** Automatic motion data picking has been selected");
            AZStd::array<MotionData*, 2> tempData { aznew UniformMotionData(), aznew NonUniformMotionData() };
            size_t smallestNumBytes = std::numeric_limits<size_t>::max();
            size_t smallestIndex = 0;
            size_t uniformDataNumBytes = 0;
            size_t uniformDataIndex = 0;
            for (size_t m = 0; m < tempData.size(); ++m)
            {
                // Init/fill and optimize the data.
                MotionData* data = tempData[m];
                InitAndOptimizeMotionData(data, sourceMotionData, sampleRate, samplingRule, rootJoints);

                // Calculate the size required on disk.
                MotionData::SaveSettings saveSettings;
                const size_t numBytes = data->CalcStreamSaveSizeInBytes(saveSettings);
                if (numBytes < smallestNumBytes)
                {
                    smallestNumBytes = numBytes;
                    smallestIndex = m;
                }

                if (azrtti_istypeof<UniformMotionData>(data))
                {
                    uniformDataNumBytes = numBytes;
                    uniformDataIndex = m;
                }

                AZ_TracePrintf("EMotionFX", "Estimated size for '%s' is %zu bytes.", data->RTTI_GetTypeName(), numBytes);
            }

            AZ_TracePrintf("EMotionFX", "Smallest data type is '%s'", tempData[smallestIndex]->RTTI_GetTypeName());

            // If the smallest type is not the fast UniformData type, check if the byte size difference between the uniform data and
            // the smallest data type is within given limits. If so, pick the uniform data instead.
            const MotionData* smallestData = tempData[smallestIndex];
            if (!azrtti_istypeof<UniformMotionData>(smallestData))
            {
                const size_t smallestSize = smallestNumBytes;
                const size_t sizeDiffInBytes = uniformDataNumBytes - smallestSize;
                const float allowedSizeOverheadPercentage = samplingRule ? samplingRule->GetAllowedSizePercentage() : 15.0f;
                const float overheadPercentage = sizeDiffInBytes / static_cast<float>(smallestSize); // Between 0 and 1.
                if (overheadPercentage <= allowedSizeOverheadPercentage * 0.01f) // Multiply by 0.01 as allowedSizeOverheadPercentage is in range of 0..100
                {
                    smallestIndex = uniformDataIndex;
                    AZ_TracePrintf("EMotionFX", "Overriding to use UniformMotionData because the size overhead is within specified limits of '%.1f' percent (actual overhead=%.1f percent)",
                        allowedSizeOverheadPercentage,
                        overheadPercentage * 100.0f);
                }
                else
                {
                    AZ_TracePrintf("EMotionFX", "Uniform data overhead is larger than the allowed overhead percentage (allowed=%.1f percent, actual=%.1f percent)",
                        allowedSizeOverheadPercentage,
                        overheadPercentage * 100.0f);
                }
            }

            finalMotionData = tempData[smallestIndex];

            // Delete all temp datas, except the one we used as final data, as we assign that to the motion later on.
            // Which means it will be automatically destroyed by the motion destructor later.
            for (size_t m = 0; m < tempData.size(); ++m)
            {
                if (m != smallestIndex)
                {
                    delete tempData[m];
                }
            }

            return finalMotionData;
        }

        AZ::SceneAPI::Events::ProcessingResult MotionDataBuilder::BuildMotionData(MotionDataBuilderContext& context)
        {
            if (context.m_phase != AZ::RC::Phase::Filling)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            const Group::IMotionGroup& motionGroup = context.m_group;
            const char* rootBoneName = motionGroup.GetSelectedRootBone().c_str();
            AZ_TraceContext("Root bone", rootBoneName);

            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();

            SceneContainers::SceneGraph::NodeIndex rootBoneNodeIndex = graph.Find(rootBoneName);
            if (!rootBoneNodeIndex.IsValid())
            {
                AZ_TracePrintf(SceneUtil::ErrorWindow, "Root bone cannot be found.\n");
                return SceneEvents::ProcessingResult::Failure;
            }

            // Get the coordinate system conversion rule.
            CoordinateSystemConverter ruleCoordSysConverter;
            AZStd::shared_ptr<Rule::CoordinateSystemRule> coordinateSystemRule = motionGroup.GetRuleContainerConst().FindFirstByType<Rule::CoordinateSystemRule>();
            if (coordinateSystemRule)
            {
                coordinateSystemRule->UpdateCoordinateSystemConverter();
                ruleCoordSysConverter = coordinateSystemRule->GetCoordinateSystemConverter();
            }

            CoordinateSystemConverter coordSysConverter = ruleCoordSysConverter;
            if (context.m_scene.GetOriginalSceneOrientation() != SceneContainers::Scene::SceneOrientation::ZUp)
            {
                AZ::Transform orientedTarget = ruleCoordSysConverter.GetTargetTransform();
                AZ::Transform rotationZ = AZ::Transform::CreateRotationZ(-AZ::Constants::Pi);
                orientedTarget = orientedTarget * rotationZ;

                //same as rule
                // X, Y and Z are all at the same indices inside the target coordinate system, compared to the source coordinate system.
                const AZ::u32 targetBasisIndices[3] = { 0, 1, 2 };
                coordSysConverter = CoordinateSystemConverter::CreateFromTransforms(ruleCoordSysConverter.GetSourceTransform(), orientedTarget, targetBasisIndices);
            }

            // Set new motion data.
            NonUniformMotionData* motionData = aznew NonUniformMotionData();

            // Grab the rules we need before visiting the scene graph.
            AZStd::shared_ptr<const Rule::MotionSamplingRule> samplingRule = motionGroup.GetRuleContainerConst().FindFirstByType<Rule::MotionSamplingRule>();
            AZStd::shared_ptr<const Rule::MotionAdditiveRule> additiveRule = motionGroup.GetRuleContainerConst().FindFirstByType<Rule::MotionAdditiveRule>();
            motionData->SetAdditive(additiveRule ? true : false);

            AZStd::vector<size_t> rootJoints; // The list of root nodes.

            size_t maxNumFrames = 0;
            size_t lastNumFrames = InvalidIndex;
            double lowestTimeStep = 999999999.0;

            auto nameStorage = graph.GetNameStorage();
            auto contentStorage = graph.GetContentStorage();
            auto nameContentView = SceneContainers::Views::MakePairView(nameStorage, contentStorage);
            auto graphDownwardsView = SceneViews::MakeSceneGraphDownwardsView<SceneViews::BreadthFirst>(graph, rootBoneNodeIndex, nameContentView.begin(), true);
            for (auto it = graphDownwardsView.begin(); it != graphDownwardsView.end(); ++it)
            {
                if (!it->second)
                {
                    it.IgnoreNodeDescendants();
                    continue;
                }

                AZStd::shared_ptr<const SceneDataTypes::IBoneData> nodeBone = azrtti_cast<const SceneDataTypes::IBoneData*>(it->second);
                if (!nodeBone)
                {
                    it.IgnoreNodeDescendants();
                    continue;
                }

                // Currently only get the first (one) AnimationData
                auto childView = SceneViews::MakeSceneGraphChildView<SceneViews::AcceptEndPointsOnly>(graph, graph.ConvertToNodeIndex(it.GetHierarchyIterator()),
                        graph.GetContentStorage().begin(), true);
                auto result = AZStd::find_if(childView.begin(), childView.end(), SceneContainers::DerivedTypeFilter<SceneDataTypes::IAnimationData>());
                if (result == childView.end())
                {
                    continue;
                }

                const SceneDataTypes::IAnimationData* animation = azrtti_cast<const SceneDataTypes::IAnimationData*>(result->get());

                const char* nodeName = it->first.GetName();
                const size_t jointDataIndex = motionData->AddJoint(nodeName, Transform::CreateIdentity(), Transform::CreateIdentity());

                // If we deal with a root bone or one of its child nodes, disable the keytrack optimization.
                // This prevents sliding feet etc. A better solution is probably to increase compression rates based on the "distance" from the root node, hierarchy wise.
                const SceneContainers::SceneGraph::NodeIndex boneNodeIndex = graph.Find(it->first.GetPath());
                if (graph.GetNodeParent(boneNodeIndex) == rootBoneNodeIndex || boneNodeIndex == rootBoneNodeIndex)
                {
                    rootJoints.emplace_back(jointDataIndex);
                }

                const size_t sceneFrameCount = aznumeric_caster(animation->GetKeyFrameCount());
                size_t startFrame = 0;
                size_t endFrame = 0;
                if (motionGroup.GetRuleContainerConst().ContainsRuleOfType<const Rule::MotionRangeRule>())
                {
                    AZStd::shared_ptr<const Rule::MotionRangeRule> motionRangeRule = motionGroup.GetRuleContainerConst().FindFirstByType<const Rule::MotionRangeRule>();
                    startFrame = aznumeric_caster(motionRangeRule->GetStartFrame());
                    endFrame = aznumeric_caster(motionRangeRule->GetEndFrame());

                    // Sanity check
                    if (startFrame >= sceneFrameCount)
                    {
                        AZ_TracePrintf(SceneUtil::ErrorWindow, "Start frame %d is greater or equal than the actual number of frames %d in animation.\n", startFrame, sceneFrameCount);
                        return SceneEvents::ProcessingResult::Failure;
                    }
                    if (endFrame >= sceneFrameCount)
                    {
                        AZ_TracePrintf(SceneUtil::WarningWindow, "End frame %d is greater or equal than the actual number of frames %d in animation. Clamping the end frame to %d\n", endFrame, sceneFrameCount, sceneFrameCount - 1);
                        endFrame = sceneFrameCount - 1;
                    }
                }
                else
                {
                    startFrame = 0;
                    endFrame = sceneFrameCount - 1;
                }

                const size_t numFrames = (endFrame - startFrame) + 1;

                maxNumFrames = AZ::GetMax(numFrames, maxNumFrames);

                motionData->AllocateJointPositionSamples(jointDataIndex, numFrames);
                motionData->AllocateJointRotationSamples(jointDataIndex, numFrames);
                EMFX_SCALECODE
                (
                    motionData->AllocateJointScaleSamples(jointDataIndex, numFrames);
                )

                // Get the bind pose transform in local space.
                using SceneAPIMatrixType = AZ::SceneAPI::DataTypes::MatrixType;
                SceneAPIMatrixType bindSpaceLocalTransform;
                const SceneContainers::SceneGraph::NodeIndex parentIndex = graph.GetNodeParent(boneNodeIndex);
                if (boneNodeIndex != rootBoneNodeIndex)
                {
                    auto parentNode = graph.GetNodeContent(parentIndex);
                    AZStd::shared_ptr<const SceneDataTypes::IBoneData> parentNodeBone = azrtti_cast<const SceneDataTypes::IBoneData*>(parentNode);
                    bindSpaceLocalTransform = parentNodeBone->GetWorldTransform().GetInverseFull() * nodeBone->GetWorldTransform();
                }
                else
                {
                    bindSpaceLocalTransform = nodeBone->GetWorldTransform();
                }

                // Get the time step and make sure it didn't change compared to other joint animations.
                const double timeStep = animation->GetTimeStepBetweenFrames();
                lowestTimeStep = AZ::GetMin<double>(timeStep, lowestTimeStep);

                SceneAPIMatrixType sampleFrameTransformInverse;
                if (additiveRule)
                {
                    size_t sampleFrameIndex = additiveRule->GetSampleFrameIndex();
                    if (sampleFrameIndex >= sceneFrameCount)
                    {
                        AZ_Assert(false, "The requested sample frame index is greater than the total frame number. Please fix it, or the frame 0 will be used as the sample frame.");
                        sampleFrameIndex = 0;
                    }
                    sampleFrameTransformInverse = animation->GetKeyFrame(sampleFrameIndex).GetInverseFull();
                }
                
                for (size_t frame = 0; frame < numFrames; ++frame)
                {
                    const float time = aznumeric_cast<float>(frame * timeStep);
                    SceneAPIMatrixType boneTransform = animation->GetKeyFrame(frame + startFrame);
                    if (additiveRule)
                    {
                        // For additive motion, we stores the relative transform.
                        boneTransform = sampleFrameTransformInverse * boneTransform;
                    }
                    
                    SceneAPIMatrixType boneTransformNoScale(boneTransform);
                    const AZ::Vector3 position = coordSysConverter.ConvertVector3(boneTransform.GetTranslation());
                    const AZ::Quaternion rotation = coordSysConverter.ConvertQuaternion(AZ::Quaternion::CreateFromMatrix3x4(boneTransformNoScale));
                    const AZ::Vector3 scale = coordSysConverter.ConvertScale(boneTransformNoScale.ExtractScale());
                    
                    // Set the pose when this is the first frame.
                    // This is used as optimization so that poses or non-animated submotions do not need any key tracks.
                    if (frame == 0)
                    {
                        motionData->SetJointStaticPosition(jointDataIndex, position);
                        motionData->SetJointStaticRotation(jointDataIndex, rotation);
                        EMFX_SCALECODE
                        (
                            motionData->SetJointStaticScale(jointDataIndex, scale);
                        )
                    }

                    motionData->SetJointPositionSample(jointDataIndex, frame, {time, position});
                    motionData->SetJointRotationSample(jointDataIndex, frame, {time, rotation});
                    EMFX_SCALECODE
                    (
                        motionData->SetJointScaleSample(jointDataIndex, frame, {time, scale});
                    )
                }

                // Set the bind pose transform.
                SceneAPIMatrixType bindBoneTransformNoScale(bindSpaceLocalTransform);
                const AZ::Vector3    bindPos   = coordSysConverter.ConvertVector3(bindSpaceLocalTransform.GetTranslation());
                const AZ::Vector3    bindScale = coordSysConverter.ConvertScale(bindBoneTransformNoScale.ExtractScale());
                const AZ::Quaternion bindRot   = coordSysConverter.ConvertQuaternion(AZ::Quaternion::CreateFromMatrix3x4(bindBoneTransformNoScale)).GetNormalized();

                motionData->SetJointBindPosePosition(jointDataIndex, bindPos);
                motionData->SetJointBindPoseRotation(jointDataIndex, bindRot);
                EMFX_SCALECODE
                (
                    motionData->SetJointBindPoseScale(jointDataIndex, bindScale);
                )
            }

            AZStd::shared_ptr<Rule::IMotionScaleRule> scaleRule = motionGroup.GetRuleContainerConst().FindFirstByType<Rule::IMotionScaleRule>();
            if (scaleRule)
            {
                float scaleFactor = scaleRule->GetScaleFactor();
                if (!AZ::IsClose(scaleFactor, 1.0f, FLT_EPSILON)) // If the scale factor is 1, no need to call Scale
                {
                    motionData->Scale(scaleFactor);
                }
            }

            // Process morphs.
            auto sceneGraphView = SceneViews::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());
            auto sceneGraphDownardsIteratorView = SceneViews::MakeSceneGraphDownwardsView<SceneViews::BreadthFirst>(
                graph, graph.GetRoot(), sceneGraphView.begin(), true);

            auto iterator = sceneGraphDownardsIteratorView.begin();
            for (; iterator != sceneGraphDownardsIteratorView.end(); ++iterator)
            {
                SceneContainers::SceneGraph::HierarchyStorageConstIterator hierarchy = iterator.GetHierarchyIterator();
                SceneContainers::SceneGraph::NodeIndex currentIndex = graph.ConvertToNodeIndex(hierarchy);
                AZ_Assert(currentIndex.IsValid(), "While iterating through the Scene Graph an unexpected invalid entry was found.");
                AZStd::shared_ptr<const SceneDataTypes::IGraphObject> currentItem = iterator->second;
                if (hierarchy->IsEndPoint())
                {
                    if (currentItem->RTTI_IsTypeOf(SceneDataTypes::IBlendShapeAnimationData::TYPEINFO_Uuid()))
                    {
                        const SceneDataTypes::IBlendShapeAnimationData* blendShapeAnimationData = static_cast<const SceneDataTypes::IBlendShapeAnimationData*>(currentItem.get());

                        const size_t morphDataIndex = motionData->AddMorph(blendShapeAnimationData->GetBlendShapeName(), 0.0f);
                        const size_t keyFrameCount = blendShapeAnimationData->GetKeyFrameCount();
                        motionData->AllocateMorphSamples(morphDataIndex, keyFrameCount);
                        const double keyFrameStep = blendShapeAnimationData->GetTimeStepBetweenFrames();
                        for (int keyFrameIndex = 0; keyFrameIndex < keyFrameCount; keyFrameIndex++)
                        {
                            const float keyFrameValue = static_cast<float>(blendShapeAnimationData->GetKeyFrame(keyFrameIndex));
                            const float keyframeTime = static_cast<float>(keyFrameIndex * keyFrameStep);
                            motionData->SetMorphSample(morphDataIndex, keyFrameIndex, {keyframeTime, keyFrameValue});
                        }
                    }
                }
            }

            // Let's prepare the motion data in the type we want.
            // This can later be extended with other types of motion data like least square fit curves etc.
            motionData->UpdateDuration();

            // Get the sample rate we have setup or that we have used.
            // Also make sure we don't sample at higher rate than we want.
            float sampleRate = 30.0f;
            if (lowestTimeStep > 0.0)
            {
                const float maxSampleRate = static_cast<float>(1.0 / lowestTimeStep);
                if (samplingRule && samplingRule->GetSampleRateMethod() == EMotionFX::Pipeline::Rule::MotionSamplingRule::SampleRateMethod::Custom)
                {
                    sampleRate = AZ::GetMin(maxSampleRate, samplingRule->GetCustomSampleRate());
                }
                else
                {
                    sampleRate = maxSampleRate;
                }
            }
            AZ_TracePrintf("EMotionFX", "Motion sample rate = %f", sampleRate);
            motionData->RemoveRedundantKeyframes(samplingRule ? !samplingRule->GetKeepDuration() : false); // Clear any tracks of non-animated parts.

            // Create the desired type of motion data, based on what is selected in the motion sampling rule.
            MotionData* finalMotionData = nullptr;
            const AZ::TypeId motionDataTypeId = samplingRule ? samplingRule->GetMotionDataTypeId() : AZ::TypeId::CreateNull();
            const bool isAutomaticMode = motionDataTypeId.IsNull();
            const MotionDataFactory& motionDataFactory = GetMotionManager().GetMotionDataFactory();
            if (isAutomaticMode) // Automatically pick a motion data type, based on the data size.
            {
                finalMotionData = AutoCreateMotionData(motionData, sampleRate, samplingRule.get(), rootJoints);
            }
            else if (motionDataFactory.IsRegisteredTypeId(motionDataTypeId)) // Yay, we found the typeId, so let's create it through the factory.
            {
                finalMotionData = motionDataFactory.Create(motionDataTypeId);
                AZ_Assert(finalMotionData, "Expected a valid motion data pointer.");
                if (finalMotionData)
                {
                    AZ_TracePrintf("EMotionFX", "*************** Created type = '%s' (%s)", motionDataTypeId.ToString<AZStd::string>().c_str(), finalMotionData->RTTI_GetTypeName());
                }
            }
            else // The type somehow isn't registered.
            {
                AZ_Warning("EMotionFX", false, "The motion data factory has no registered type with typeId %s", motionDataTypeId.ToString<AZStd::string>().c_str());
            }

            // Fall back to a default type.
            if (!finalMotionData)
            {
                AZ_Warning("EMotionFX", false, "As we failed to create a valid motion data type, we're using a fallback UniformMotionData.");
                finalMotionData = aznew UniformMotionData();
            }

            // Initialize the final motion data.
            // We already have done this one page above, when we are in automatic mode, so skip when we use automatic mode.
            if (!isAutomaticMode)
            {
                InitAndOptimizeMotionData(finalMotionData, motionData, sampleRate, samplingRule.get(), rootJoints);
            }

            // Delete the data that we created out of the Scene API as it is no longer needed as we already extracted all the data from it
            // into our finalMotionData.
            delete motionData;
            context.m_motion.SetMotionData(finalMotionData);

            return SceneEvents::ProcessingResult::Success;
        }
    } // namespace Pipeline
} // namespace EMotionFX
