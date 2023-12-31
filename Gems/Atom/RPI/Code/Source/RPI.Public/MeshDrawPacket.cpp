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

#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>
#include <Atom/RPI.Public/Scene.h> 
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RHI/DrawPacketBuilder.h>

namespace AZ
{   
    namespace RPI
    {
        MeshDrawPacket::MeshDrawPacket(
            ModelLod& modelLod,
            size_t modelLodMeshIndex,
            Data::Instance<Material> materialOverride,
            Data::Instance<ShaderResourceGroup> objectSrg,
            const MaterialModelUvOverrideMap& materialModelUvMap
        )
            : m_modelLod(&modelLod)
            , m_modelLodMeshIndex(modelLodMeshIndex)
            , m_objectSrg(objectSrg)
            , m_material(materialOverride)
            , m_materialModelUvMap(materialModelUvMap)
        {
            if (!m_material)
            {
                const ModelLod::Mesh& mesh = m_modelLod->GetMeshes()[m_modelLodMeshIndex];
                m_material = mesh.m_material;
            }
        }

        Data::Instance<Material> MeshDrawPacket::GetMaterial()
        {
            return m_material;
        }

        bool MeshDrawPacket::Update(const Scene& parentScene, bool forceUpdate /*= false*/)
        {
            // Why we need to check "!m_material->NeedsCompile()"...
            //    Frame A:
            //      - Material::SetPropertyValue("foo",...). This bumps the material's CurrentChangeId()
            //      - Material::Compile() updates all the material's outputs (SRG data, shader selection, shader options, etc).
            //      - Material::SetPropertyValue("bar",...). This bumps the materials' CurrentChangeId() again.
            //      - We do not process Material::Compile() a second time because because you can only call SRG::Compile() once per frame. Material::Compile()
            //        will be processed on the next frame. (See implementation of Material::Compile())
            //      - MeshDrawPacket::Update() is called. It runs DoUpdate() to rebuild the draw packet, but everything is still in the state when "foo" was
            //        set. The "bar" changes haven't been applied yet. It also sets m_materialChangeId to GetCurrentChangeId(), which corresponds to "bar" not "foo".
            //    Frame B:
            //      - Something calls Material::Compile(). This finally updates the material's outputs with the latest data corresponding to "bar".
            //      - MeshDrawPacket::Update() is called. But since the GetCurrentChangeId() hasn't changed since last time, DoUpdate() is not called.
            //      - The mesh continues rendering with only the "foo" change applied, indefinitely.

            if (forceUpdate || (!m_material->NeedsCompile() && m_materialChangeId != m_material->GetCurrentChangeId()))
            {
                DoUpdate(parentScene);
                m_materialChangeId = m_material->GetCurrentChangeId();
                return true;
            }

            return false;
        }

        bool MeshDrawPacket::DoUpdate(const Scene& parentScene)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);
            const ModelLod::Mesh& mesh = m_modelLod->GetMeshes()[m_modelLodMeshIndex];

            if (!m_material)
            {
                AZ_Warning("MeshDrawPacket", false, "No material provided for mesh. Skipping.");
                return false;
            }

            RHI::DrawPacketBuilder drawPacketBuilder;
            drawPacketBuilder.Begin(nullptr);

            drawPacketBuilder.SetDrawArguments(mesh.m_drawArguments);
            drawPacketBuilder.SetIndexBufferView(mesh.m_indexBufferView);
            drawPacketBuilder.AddShaderResourceGroup(m_objectSrg->GetRHIShaderResourceGroup());
            drawPacketBuilder.AddShaderResourceGroup(m_material->GetRHIShaderResourceGroup());

            // We build the list of used shaders in a local list rather than m_activeShaders so that
            // if DoUpdate() fails it won't modify any member data.
            MeshDrawPacket::ShaderList shaderList;
            shaderList.reserve(m_activeShaders.size());

            // We have to keep a list of these outside the loops that collect all the shaders because the DrawPacketBuilder
            // keeps pointers to StreamBufferViews until DrawPacketBuilder::End() is called. And we use a fixed_vector to guarantee
            // that the memory won't be relocated when new entries are added.
            AZStd::fixed_vector<ModelLod::StreamBufferViewList, RHI::DrawPacketBuilder::DrawItemCountMax> streamBufferViewsPerShader;

            m_perDrawSrgs.clear();

            auto appendShader = [&](const ShaderCollection::Item& shaderItem)
            {
                AZ_PROFILE_SCOPE(Debug::ProfileCategory::AzRender, "appendShader()");
                Data::Instance<Shader> shader = RPI::Shader::FindOrCreate(shaderItem.GetShaderAsset());
                if (!shader)
                {
                    AZ_Error("MeshDrawPacket", false, "Shader '%s'. Failed to find or create instance", shaderItem.GetShaderAsset()->GetName().GetCStr());
                    return false;
                }

                const AZ::Data::Asset<ShaderResourceGroupAsset>& drawSrgAsset = shader->GetAsset()->GetDrawSrgAsset();

                // Set all unspecified shader options to default values, so that we get the most specialized variant possible.
                // (because FindVariantStableId treats unspecified options as a request specificlly for a variant that doesn't specify those options)
                // [GFX TODO][ATOM-3883] We should consider updating the FindVariantStableId algorithm to handle default values for us, and remove this step here.
                RPI::ShaderOptionGroup shaderOptions = *shaderItem.GetShaderOptions();
                shaderOptions.SetUnspecifiedToDefaultValues();

                // [GFX_TODO][ATOM-14476]: according to this usage, we should make the shader input contract uniform across all shader variants.
                m_modelLod->CheckOptionalStreams(
                    shaderOptions,
                    shader->GetVariant(ShaderAsset::RootShaderVariantStableId).GetInputContract(),
                    m_modelLodMeshIndex,
                    m_materialModelUvMap,
                    m_material->GetAsset()->GetMaterialTypeAsset()->GetUvNameMap());

                const ShaderVariantId finalVariantId = shaderOptions.GetShaderVariantId();

                ShaderVariantSearchResult findVariantResult = shader->FindVariantStableId(finalVariantId);
                const ShaderVariant& variant = shader->GetVariant(findVariantResult.GetStableId());

                Data::Instance<ShaderResourceGroup> drawSrg;
                if (drawSrgAsset)
                {
                    AZ_PROFILE_SCOPE(Debug::ProfileCategory::AzRender, "create drawSrg");
                    // If the DrawSrg exists we must create and bind it, otherwise the CommandList will fail validation for SRG being null
                    drawSrg = RPI::ShaderResourceGroup::Create(drawSrgAsset);

                    if (!variant.IsFullyBaked() && drawSrgAsset->GetLayout()->HasShaderVariantKeyFallbackEntry())
                    {
                        drawSrg->SetShaderVariantKeyFallbackValue(shaderOptions.GetShaderVariantKeyFallbackValue());
                    }

                    drawSrg->Compile();
                }

                RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
                variant.ConfigurePipelineState(pipelineStateDescriptor);

                // Render states need to merge the runtime variation.
                // This allows materials to customize the render states that the shader uses.
                const RHI::RenderStates& renderStatesOverlay = *shaderItem.GetRenderStatesOverlay();
                RHI::MergeStateInto(renderStatesOverlay, pipelineStateDescriptor.m_renderStates);

                streamBufferViewsPerShader.push_back();
                auto& streamBufferViews = streamBufferViewsPerShader.back();

                if (!m_modelLod->GetStreamsForMesh(
                    pipelineStateDescriptor.m_inputStreamLayout,
                    streamBufferViews,
                    variant.GetInputContract(),
                    m_modelLodMeshIndex,
                    m_materialModelUvMap,
                    m_material->GetAsset()->GetMaterialTypeAsset()->GetUvNameMap()))
                {
                    return false;
                }

                // Use the default draw list tag from the shader variant.
                RHI::DrawListTag drawListTag = shader->GetDrawListTag();

                // Use the explicit draw list override if exist.
                RHI::DrawListTag runtimeTag = shaderItem.GetDrawListTagOverride();
                if (!runtimeTag.IsNull())
                {
                    drawListTag = runtimeTag;
                }

                if (!parentScene.ConfigurePipelineState(drawListTag, pipelineStateDescriptor))
                {
                    // drawListTag not found in this scene, so don't render this item
                    return false;
                }

                const RHI::PipelineState* pipelineState = shader->AcquirePipelineState(pipelineStateDescriptor);
                if (!pipelineState)
                {
                    AZ_Error("MeshDrawPacket", false, "Shader '%s'. Failed to acquire default pipeline state", shaderItem.GetShaderAsset()->GetName().GetCStr());
                    return false;
                }

                RHI::DrawPacketBuilder::DrawRequest drawRequest;
                drawRequest.m_listTag = drawListTag;
                drawRequest.m_pipelineState = pipelineState;
                drawRequest.m_streamBufferViews = streamBufferViews;
                drawRequest.m_stencilRef = m_stencilRef;
                drawRequest.m_sortKey = m_sortKey;
                if (drawSrg)
                {
                    drawRequest.m_uniqueShaderResourceGroup = drawSrg->GetRHIShaderResourceGroup();
                    m_perDrawSrgs.push_back(drawSrg);
                }
                drawPacketBuilder.AddDrawItem(drawRequest);

                shaderList.emplace_back(AZStd::move(shader));

                return true;
            };

            // [GFX TODO][ATOM-5625] This really needs to be optimized to put the burden on setting global shader options, not applying global shader options.
            // For example, make the shader system collect a map of all shaders and ShaderVaraintIds, and look up the shader option names at set-time.
            RPI::ShaderSystemInterface* shaderSystem = RPI::ShaderSystemInterface::Get();
            for (auto iter : shaderSystem->GetGlobalShaderOptions())
            {
                const AZ::Name& shaderOptionName = iter.first;
                ShaderOptionValue value = iter.second;
                if (!m_material->SetSystemShaderOption(shaderOptionName, value).IsSuccess())
                {
                    AZ_Warning("MeshDrawPacket", false, "Shader option '%s' is owned by this this material. Global value for this option was ignored.", shaderOptionName.GetCStr());
                }
            }

            for (auto& shaderItem : m_material->GetShaderCollection())
            {
                if (shaderItem.IsEnabled())
                {
                    if (shaderList.size() == RHI::DrawPacketBuilder::DrawItemCountMax)
                    {
                        AZ_Error("MeshDrawPacket", false, "Material has more than the limit of %d active shader items.", RHI::DrawPacketBuilder::DrawItemCountMax);
                        return false;
                    }

                    appendShader(shaderItem);
                }
            }

            m_drawPacket = drawPacketBuilder.End();

            if (m_drawPacket)
            {
                m_activeShaders = shaderList;
                m_materialSrg = m_material->GetRHIShaderResourceGroup();
                return true;
            }
            else
            {
                return false;
            }
        }

        const RHI::DrawPacket* MeshDrawPacket::GetRHIDrawPacket() const
        {
            return m_drawPacket.get();
        }
    } // namespace RPI
} // namespace AZ
