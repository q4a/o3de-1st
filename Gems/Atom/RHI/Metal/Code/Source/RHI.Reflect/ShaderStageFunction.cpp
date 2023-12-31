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

#include <Atom/RHI.Reflect/Metal/ShaderStageFunction.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Metal
    {
        void ShaderStageFunction::Reflect(ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderStageFunction, RHI::ShaderStageFunction>()
                    ->Version(3)
                    ->Field("m_sourceCode", &ShaderStageFunction::m_sourceCode)
                    ->Field("m_byteCode", &ShaderStageFunction::m_byteCode)
                    ->Field("m_byteCodeLength", &ShaderStageFunction::m_byteCodeLength)
                    ->Field("m_entryFunctionName", &ShaderStageFunction::m_entryFunctionName);
            }
        }

        ShaderStageFunction::ShaderStageFunction(RHI::ShaderStage shaderStage)
            : RHI::ShaderStageFunction(shaderStage)
        {}
    
        RHI::Ptr<ShaderStageFunction> ShaderStageFunction::Create(RHI::ShaderStage shaderStage)
        {
            return aznew ShaderStageFunction(shaderStage);
        }

        void ShaderStageFunction::SetSourceCode(const ShaderSourceCode& sourceCode)
        {
            m_sourceCode = AZStd::string(sourceCode.begin(), sourceCode.end());
        }
        
        const AZStd::string& ShaderStageFunction::GetSourceCode() const
        {
            return m_sourceCode;
        }

        void ShaderStageFunction::SetByteCode(const ShaderByteCode& byteCode)
        {
            m_byteCode = byteCode;
            m_byteCodeLength = aznumeric_cast<uint32_t>(byteCode.size());
        }

        void ShaderStageFunction::SetEntryFunctionName(AZStd::string_view entryFunctionName)
        {
            m_entryFunctionName = entryFunctionName;
        }
        
        const ShaderByteCode& ShaderStageFunction::GetByteCode() const
        {
            return m_byteCode;
        }

        const uint32_t ShaderStageFunction::GetByteCodeLength() const
        {
            return m_byteCodeLength;
        }

        const AZStd::string& ShaderStageFunction::GetEntryFunctionName() const
        {
            return m_entryFunctionName;
        }

        RHI::ResultCode ShaderStageFunction::FinalizeInternal()
        {
            if (m_sourceCode.empty())
            {
                AZ_Error("ShaderStageFunction", false, "Finalizing shader stage function with empty sourcecode.");
                return RHI::ResultCode::InvalidArgument;
            }

            // [GFX_TODO][bethelz]: Move source code hashing from RPI branch here.
            HashValue64 hash = HashValue64{ 0 };

            hash = TypeHash64(reinterpret_cast<const uint8_t*>(m_sourceCode.data()), m_sourceCode.size(), hash);
            SetHash(hash);
            //![GFX_TODO]

            return RHI::ResultCode::Success;
        }
    }
}
