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

#pragma once

#include <Atom/RHI.Reflect/Base.h> // for AZ_BITS and AZ_DEFINE_ENUM_BITWISE_OPERATORS

#include <Atom/RPI.Public/PipelineState.h>

#include <AzCore/std/smart_ptr/intrusive_base.h>

namespace AZ
{
    namespace RPI
    {
        //! This class helps setup dynamic draw data as well as provide draw functions to draw dynamic items.
        //! The draw calls added to the context are only valid for one frame.
        //! DynamicDrawContext is only associated with
        //!     * One shader.
        //!     * One draw list tag which is initialized from shader but can be overwritten.
        //! DynamicDrawContext may allow some render states change or few other changes which are defined in DynamicDrawContext::DrawVariation
        class DynamicDrawContext
            : public AZStd::intrusive_base
        {
            friend class DynamicDrawSystem;
            AZ_RTTI(AZ::RPI::DynamicDrawContext, "{9F6645D7-2C64-4963-BAAB-5144E92F61E2}");
            AZ_CLASS_ALLOCATOR(DynamicDrawContext, AZ::SystemAllocator, 0);

        public:
            virtual ~DynamicDrawContext() = default;

            // Type of render state which can be changed for dynamic draw context
            enum class DrawStateOptions : uint32_t
            {
                PrimitiveType = AZ_BIT(0),
                DepthState = AZ_BIT(1),
                EnableStencil = AZ_BIT(2),
                FaceCullMode = AZ_BIT(3),
                BlendMode = AZ_BIT(4)
            };

            struct VertexChannel
            {
                VertexChannel(const AZStd::string& name, RHI::Format format)
                    : m_channel(name)
                    , m_format(format)
                {}

                AZStd::string m_channel;
                RHI::Format m_format;
            };

            // Required initialization functions
            //! Initialize this context with the input shader/shader asset.
            void InitShader(Data::Asset<ShaderAsset> shaderAsset);
            void InitShader(Data::Instance<Shader> shader);

            // Optional initialization functions
            //! Initialize input stream layout with vertex channel information
            void InitVertexFormat(const AZStd::vector<VertexChannel>& vertexChannels);
            //! Initialize draw list tag of this 
            void InitDrawListTag(RHI::DrawListTag drawListTag);

            //! Customize pipeline state through a function
            //! This function is intended to do pipeline state customization after all initialization function calls but before EndInit
            void CustomizePipelineState(AZStd::function<void(Ptr<PipelineStateForDraw>)> updatePipelineState);

            //! Enable draw state changes for this DynamicDrawContext.
            //! This function can only be called before EndInit() is called
            void AddDrawStateOptions(DrawStateOptions options);

            //! Finalize and validate initialization. Any initialization functions should be called before EndInit is called. 
            void EndInit();

            //! Return if this DynamicDrawContext is ready to add draw calls
            bool IsReady();

            //! Return if some draw state options change are enabled. 
            bool HasDrawStateOptions(DrawStateOptions options);

            // States which can be changed for this DyanimcDrawContext

            //! Set DepthState if DrawStateOptions::DepthState option is enabled
            void SetDepthState(RHI::DepthState depthState);
            //! Enable/disable stencil if DrawStateOptions::EnableStencil option is enabled
            void SetEnableStencil(bool enable);
            //! Set CullMode if DrawStateOptions::FaceCullMode option is enabled
            void SetCullMode(RHI::CullMode cullMode);
            //! Set TargetBlendState for target 0 if DrawStateOptions::BlendMode option is enabled
            void SetTarget0BlendState(RHI::TargetBlendState blendState);
            //! Set PrimitiveType if DrawStateOptions::PrimitiveType option is enabled
            void SetPrimitiveType(RHI::PrimitiveTopology topology);

            //! Setup scissor for following draws which are added to this DynamicDrawContext
            //! Note: it won't effect any draws submitted out of this DynamicDrawContext
            void SetScissor(RHI::Scissor scissor);

            //! Remove per draw scissor for draws added to this DynamicDrawContext
            //! Without per draw scissor, the scissor setup in pass is usually used. 
            void UnsetScissor();

            //! Setup viewport for following draws which are added to this DynamicDrawContext
            //! Note: it won't effect any draws submitted out of this DynamicDrawContext
            void SetViewport(RHI::Viewport viewport);

            //! Remove per draw viewport for draws added to this DynamicDrawContext
            //! Without per draw viewport, the viewport setup in pass is usually used. 
            void UnsetViewport();

            //! Draw Indexed primitives with vertex and index data and per draw srg
            //! The per draw srg need to be provided if it's required by shader. 
            void DrawIndexed(void* vertexData, uint32_t vertexCount, void* indexData, uint32_t indexCount, RHI::IndexFormat indexFormat, Data::Instance < ShaderResourceGroup> drawSrg = nullptr);

            //! Get per vertex size. The size was evaluated when vertex format was set
            uint32_t GetPerVertexDataSize();

            //! Get DrawListTag of this DyanmicDrawContext
            RHI::DrawListTag GetDrawListTag();

            //! Create a draw srg
            Data::Instance<ShaderResourceGroup> NewDrawSrg();

            //! Get per context srg
            Data::Instance<ShaderResourceGroup> GetPerContextSrg();

            //! return whether the vertex data size is valid
            bool IsVertexSizeValid(uint32_t vertexSize);

        private:
            DynamicDrawContext() = default;

            // Submit draw items to a view
            void SubmitDrawData(ViewPtr view);
            
            // Reset cached draw data when frame is end (draw data was submitted)
            void FrameEnd();

            // Get rhi pipeline state which matches current states
            const RHI::PipelineState* GetCurrentPipelineState();

            struct MultiStates
            {
                // states available for change 
                RHI::CullMode m_cullMode;
                RHI::DepthState m_depthState;
                bool m_enableStencil;
                RHI::PrimitiveTopology m_topology;
                RHI::TargetBlendState m_blendState0;

                HashValue64 m_hash = HashValue64{ 0 };
                bool m_isDirty = false;

                void UpdateHash(const DrawStateOptions& drawStateOptions);
            };

            MultiStates m_currentStates;

            // current scissor
            bool m_useScissor = false;
            RHI::Scissor m_scissor;

            // current scissor
            bool m_useViewport = false;
            RHI::Viewport m_viewport;

            // Cached RHI pipeline states for different combination of render states 
            AZStd::unordered_map<HashValue64, const RHI::PipelineState*> m_cachedRhiPipelineStates;

            // Current RHI pipeline state for current MultiStates
            const RHI::PipelineState* m_rhiPipelineState = nullptr;

            // Data for draw item
            Ptr<PipelineStateForDraw> m_pipelineState;
            Data::Instance<ShaderResourceGroup> m_srgPerContext;
            RHI::ShaderResourceGroup* m_srgGroups[1]; // array for draw item's srg groups
            uint32_t m_perVertexDataSize = 0;
            Data::Asset<ShaderResourceGroupAsset> m_drawSrgAsset;

            // Draw variations allowed in this DynamicDrawContext
            DrawStateOptions m_drawStateOptions;

            // For generate output attachment layout and filter draw items
            Scene* m_scene = nullptr;
            RHI::DrawListTag m_drawListTag;

            // Cached draw data
            AZStd::vector<RHI::StreamBufferView> m_cachedStreamBufferViews;
            AZStd::vector<RHI::IndexBufferView> m_cachedIndexBufferViews;
            AZStd::vector<Data::Instance<ShaderResourceGroup>> m_cachedDrawSrg;

            // structure includes DrawItem and stream and index buffer index
            static const uint32_t InvalidIndex = static_cast<uint32_t>(-1);
            struct DrawItemInfo
            {
                RHI::DrawItem m_drawItem;
                uint32_t m_vertexBufferViewIndex = InvalidIndex;
                uint32_t m_indexBufferViewIndex = InvalidIndex;
            };

            AZStd::vector<DrawItemInfo> m_cachedDrawItems;

            // This variable is used to see if the context is initialized.
            // You can only add draw calls when the context is initialized.
            bool m_initialized = false;
        };

        AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RPI::DynamicDrawContext::DrawStateOptions);
    }
}
