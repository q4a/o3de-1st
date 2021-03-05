/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include <AzCore/std/string/string.h>

#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>

#include <Atom/RHI.Edit/ShaderCompilerArguments.h>

namespace AssetBuilderSDK
{
    struct PlatformInfo;
}

namespace AZ
{
    namespace Data
    {
        struct AssetId;
    }

    namespace RHI
    {
        class ShaderResourceGroupLayout;
        class ShaderStageFunction;

        // [GFX TODO] ATOM-1668 This enum is a temporary copy of the RPI::ShaderStageType.
        // We need to decide if virtual stages are a good design for the RHI and expose one
        // unique shader stage enum that the RHI and RPI can use.
        enum ShaderHardwareStage : uint32_t
        {
            Invalid = static_cast<uint32_t>(-1),
            Vertex = 0,
            Geometry,
            TessellationControl,
            TessellationEvaluation,
            Fragment,
            Compute,
            RayTracing,
        };

        //! This class provides a platform agnostic interface for the creation
        //! and manipulation of platform shader objects.
        class ShaderPlatformInterface
        {
        public:
            //! This struct describes layout information of a Shader Resource Group
            //! that is part of a Pipeline.
            struct ShaderResourceGroupInfo
            {
                /// Layout of the Shader Resource Group.
                const RHI::ShaderResourceGroupLayout* m_layout;

                RHI::ShaderResourceGroupBindingInfo m_bindingInfo;
            };

            //! This struct describes binding information about root constants
            //! that is part of a Pipeline.
            struct RootConstantsInfo
            {
                /// The space id used by the constant buffer that contains the inline constants.
                uint32_t m_spaceId = ~0u;
                /// The register id used by the constant buffer that contains the inline constants.
                uint32_t m_registerId = ~0u;
                /// The total size in bytes of all inline constants.
                uint32_t m_totalSizeInBytes = 0;
            };

            using ShaderResourceGroupInfoList = AZStd::fixed_vector<ShaderResourceGroupInfo, RHI::Limits::Pipeline::ShaderResourceGroupCountMax>;

            struct ByProducts
            {
                AZStd::set<AZStd::string> m_intermediatePaths;  //!< intermediate file paths (like dxil text form)
                static constexpr uint32_t UnknownDynamicBranchCount = -1;
                uint32_t m_dynamicBranchCount = UnknownDynamicBranchCount;
            };

            //! Struct used to return data when compiling the AZSL shader to the appropriate platform.
            struct StageDescriptor
            {
                ShaderHardwareStage m_stageType = ShaderHardwareStage::Invalid;
                AZStd::vector<uint8_t> m_byteCode;
                AZStd::vector<char> m_sourceCode;
                AZStd::string m_entryFunctionName;

                ByProducts m_byProducts;  //!< Optional; used for debug information
            };

            //! @apiUniqueIndex See GetApiUniqueIndex() for details.
            explicit ShaderPlatformInterface(uint32_t apiUniqueIndex) : m_apiUniqueIndex(apiUniqueIndex) {}

            virtual ~ShaderPlatformInterface() = default;

            //! Returns the RHI API Type that this ShaderPlatformInterface supports.
            virtual RHI::APIType GetAPIType() const = 0;

            //! Returns the RHI API Name that this ShaderPlatformInterface supports.
            virtual AZ::Name GetAPIName() const = 0;

            //! Creates the platform specific pipeline layout descriptor.
            virtual RHI::Ptr<RHI::PipelineLayoutDescriptor> CreatePipelineLayoutDescriptor() = 0;

            virtual RHI::Ptr<RHI::ShaderStageFunction> CreateShaderStageFunction(const StageDescriptor& stageDescriptor) = 0;

            virtual bool IsShaderStageForRaster(ShaderHardwareStage shaderStageType) const = 0;
            virtual bool IsShaderStageForCompute(ShaderHardwareStage shaderStageType) const = 0;
            virtual bool IsShaderStageForRayTracing(ShaderHardwareStage shaderStageType) const = 0;

            //! Compiles AZSL shader to the appropriate platform.
            virtual bool CompilePlatformInternal(
                const AssetBuilderSDK::PlatformInfo& platform,
                const AZStd::string& shaderSource,
                const AZStd::string& functionName,
                ShaderHardwareStage shaderStage,
                const AZStd::string& tempFolderPath,
                StageDescriptor& outputDescriptor) const = 0;

            //! Get the parameters (except warning related) from that platform interface, and the configuration files.
            virtual AZStd::string GetAzslCompilerParameters() const = 0;

            //! Get only the warning-related parameters from that platform interface.
            virtual AZStd::string GetAzslCompilerWarningParameters() const = 0;

            //! Query whether the shaders are set to build with debug information
            virtual bool BuildHasDebugInfo() const = 0;

            //! Get the filename of include file to prefix shader programs with
            virtual const char* GetAzslHeader(const AssetBuilderSDK::PlatformInfo& platform) const = 0;

            //! Builds additional platform specific data to the pipeline layout descriptor.
            //! Will be called before CompilePlatformInternal().
            virtual bool BuildPipelineLayoutDescriptor(
                RHI::Ptr<RHI::PipelineLayoutDescriptor> pipelineLayoutDescriptor,
                const ShaderResourceGroupInfoList& srgInfoList,
                const RootConstantsInfo& rootConstantsInfo) = 0;

            //! See AZ::RHI::Factory::GetAPIUniqueIndex() for details.
            //! See AZ::RHI::Limits::APIType::PerPlatformApiUniqueIndexMax.
            uint32_t GetAPIUniqueIndex() const { return m_apiUniqueIndex; }

            //! To set when you can read from a config file: additional arguments or compiler settings
            void SetExternalArguments(const ShaderCompilerArguments& arguments)
            {
                m_settings = arguments;
            }

        protected:
            ShaderCompilerArguments m_settings;

        private:
            ShaderPlatformInterface() = delete;
            const uint32_t m_apiUniqueIndex;
        };
    }
}