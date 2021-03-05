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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/conversions.h>

#include <AtomCore/Instance/InstanceDatabase.h>
#include <AtomCore/std/containers/vector_set.h>

#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphBuilder.h>

#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/PassLibrary.h>
#include <Atom/RPI.Public/Pass/PassDefines.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>

#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>
#include <Atom/RPI.Reflect/Pass/PassRequest.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Pass/PassName.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace AZ
{
    namespace RPI
    {
        // --- Constructors ---

        Pass::Pass(const PassDescriptor& descriptor)
            : m_name(descriptor.m_passName)
            , m_path(descriptor.m_passName)
        {
            AZ_RPI_PASS_ASSERT((descriptor.m_passRequest == nullptr) || (descriptor.m_passTemplate != nullptr),
                "Pass::Pass - request is valid but template is nullptr. This is not allowed. Passing a valid passRequest also requires a valid passTemplate.");

            m_flags.m_enabled = true;
            m_flags.m_timestampQueryEnabled = false;
            m_flags.m_pipelineStatisticsQueryEnabled = false;

            m_template = descriptor.m_passTemplate;

            if (descriptor.m_passRequest != nullptr)
            {
                // Assert m_template is the same as the one in the pass request
                if (PassValidation::IsEnabled())
                {
                    const AZStd::shared_ptr<PassTemplate> passTemplate = PassSystemInterface::Get()->GetPassTemplate(descriptor.m_passRequest->m_templateName);
                    AZ_RPI_PASS_ASSERT(m_template == passTemplate, "Error: template in PassDescriptor doesn't match template from PassRequest!");
                }

                m_request = *descriptor.m_passRequest;
                m_flags.m_createdByPassRequest = true;
                m_flags.m_enabled = m_request.m_passEnabled;
            }

            PassSystemInterface::Get()->RegisterPass(this);
            QueueForBuildAttachments();
        }

        Pass::~Pass()
        {
            PassSystemInterface::Get()->UnregisterPass(this);
        }

        PassDescriptor Pass::GetPassDescriptor() const
        {
            PassDescriptor desc;
            desc.m_passName = m_name;
            desc.m_passTemplate = m_template ? PassSystemInterface::Get()->GetPassTemplate(m_template->m_name) : nullptr;
            desc.m_passRequest = m_flags.m_createdByPassRequest ? &m_request : nullptr;
            return desc;
        }

        // --- Enable/Disable ---

        void Pass::SetEnabled(bool enabled)
        {
            m_flags.m_enabled = enabled;
        }

        bool Pass::IsEnabled() const
        {
            return m_flags.m_enabled;
        }

        // --- Error Logging ---

        void Pass::LogError(AZStd::string&& message)
        {
#if AZ_RPI_ENABLE_PASS_DEBUGGING
            AZ::Debug::Trace::Break();
#endif

            if (PassValidation::IsEnabled())
            {
                ++m_errors;
                if (m_errorMessages.size() < MessageLogLimit)
                {
                    m_errorMessages.push_back(AZStd::move(message));
                }
            }
        }

        void Pass::LogWarning(AZStd::string&& message)
        {
            if (PassValidation::IsEnabled())
            {
                ++m_warnings;
                if (m_warningMessages.size() < MessageLogLimit)
                {
                    m_warningMessages.push_back(AZStd::move(message));
                }
            }
        }

        // --- Hierarchy functions ---

        void Pass::OnHierarchyChange()
        {
            if (m_parent == nullptr)
            {
                return;
            }

            // Set new tree depth and path
            m_treeDepth = m_parent->m_treeDepth + 1;
            m_path = ConcatPassName(m_parent->m_path, m_name);
            m_flags.m_partOfHierarchy = m_parent->m_flags.m_partOfHierarchy;
        }

        void Pass::RemoveFromParent()
        {
            AZ_RPI_PASS_ASSERT(m_parent != nullptr, "Trying to remove pass from parent but pointer to the parent pass is null.");
            m_parent->RemoveChild(Ptr<Pass>(this));
        }

        void Pass::OnOrphan()
        {
            m_parent = nullptr;
            m_flags.m_partOfHierarchy = false;
            m_treeDepth = 0;
        }

        // --- Getters & Setters ---

        ParentPass* Pass::GetParent() const
        {
            return m_parent;
        }

        ParentPass* Pass::AsParent()
        {
            return azrtti_cast<ParentPass*>(this);
        }

        const ParentPass* Pass::AsParent() const
        {
            return azrtti_cast<const ParentPass*>(this);
        }

        const Name& Pass::GetName() const
        {
            return m_name;
        }

        const Name& Pass::GetPathName() const
        {
            return m_path;
        }

        uint32_t Pass::GetTreeDepth() const
        {
            return m_treeDepth;
        }

        PassAttachmentBindingListView Pass::GetAttachmentBindings() const
        {
            return m_attachmentBindings;
        }

        uint32_t Pass::GetInputCount() const
        {
            return uint32_t(m_inputBindingIndices.size());
        }

        uint32_t Pass::GetInputOutputCount() const
        {
            return uint32_t(m_inputOutputBindingIndices.size());
        }

        uint32_t Pass::GetOutputCount() const
        {
            return uint32_t(m_outputBindingIndices.size());
        }

        PassAttachmentBinding& Pass::GetInputBinding(uint32_t index)
        {
            uint32_t bindingIndex = m_inputBindingIndices[index];
            return m_attachmentBindings[bindingIndex];
        }

        PassAttachmentBinding& Pass::GetInputOutputBinding(uint32_t index)
        {
            uint32_t bindingIndex = m_inputOutputBindingIndices[index];
            return m_attachmentBindings[bindingIndex];
        }

        PassAttachmentBinding& Pass::GetOutputBinding(uint32_t index)
        {
            uint32_t bindingIndex = m_outputBindingIndices[index];
            return m_attachmentBindings[bindingIndex];
        }

        void Pass::AddAttachmentBinding(PassAttachmentBinding attachmentBinding)
        {
            // Add the index of the binding to the input, output or input/output list based on the slot type
            switch (attachmentBinding.m_slotType)
            {
            case PassSlotType::Input:
                m_inputBindingIndices.push_back(uint8_t(m_attachmentBindings.size()));
                break;
            case PassSlotType::InputOutput:
                m_inputOutputBindingIndices.push_back(uint8_t(m_attachmentBindings.size()));
                break;
            case PassSlotType::Output:
                m_outputBindingIndices.push_back(uint8_t(m_attachmentBindings.size()));
                break;
            default:
                break;
            }

            // Add the binding
            m_attachmentBindings.push_back(attachmentBinding);
        }

        // --- Finders ---

        Ptr<Pass> Pass::FindAdjacentPass(const Name& passName)
        {
            // 1. Check This
            if (passName == PassNameThis)
            {
                return Ptr<Pass>(this);
            }

            // 2. Check Parent
            if (m_parent == nullptr)
            {
                return nullptr;
            }
            if (passName == PassNameParent || passName == m_parent->GetName())
            {
                return Ptr<Pass>(m_parent);
            }

            // 3. Check Siblings
            Ptr<Pass> foundPass = m_parent->FindChildPass(passName);

            // 4. Check Children
            if (!foundPass && AsParent())
            {
                foundPass = AsParent()->FindChildPass(passName);
            }

            // Finished search, return
            return foundPass;
        }

        PassAttachmentBinding* Pass::FindAttachmentBinding(const Name& slotName)
        {
            for (PassAttachmentBinding& binding : m_attachmentBindings)
            {
                if (slotName == binding.m_name)
                {
                    return &binding;
                }
            }
            return nullptr;
        }

        const PassAttachmentBinding* Pass::FindAttachmentBinding(const Name& slotName) const
        {
            for (const PassAttachmentBinding& binding : m_attachmentBindings)
            {
                if (slotName == binding.m_name)
                {
                    return &binding;
                }
            }
            return nullptr;
        }

        Ptr<PassAttachment> Pass::FindOwnedAttachment(const Name& attachmentName) const
        {
            for (const Ptr<PassAttachment>& attachment : m_ownedAttachments)
            {
                if (attachment->m_name == attachmentName)
                {
                    return attachment;
                }
            }

            return nullptr;
        }

        Ptr<PassAttachment> Pass::FindAttachment(const Name& slotName) const
        {
            if (const PassAttachmentBinding* binding = FindAttachmentBinding(slotName))
            {
                return binding->m_attachment;
            }

            return FindOwnedAttachment(slotName);
        }

        const PassAttachmentBinding* Pass::FindAdjacentBinding(const PassAttachmentRef& attachmentRef)
        {
            // Validate attachmentRef
            if (attachmentRef.m_pass.IsEmpty() || attachmentRef.m_attachment.IsEmpty())
            {
                return nullptr;
            }
            // Find pass
            if (Ptr<Pass> pass = FindAdjacentPass(attachmentRef.m_pass))
            {
                // Find attachment within pass
                return pass->FindAttachmentBinding(attachmentRef.m_attachment);
            }
            return nullptr;
        }

        // --- Queuing functions with PassSystem ---

        void Pass::QueueForBuildAttachments()
        {
            // Don't queue if we're in building phase
            if (!PassSystemInterface::Get()->IsBuilding())
            {
                // m_queuedForBuildAttachment makes sure the pass only be queue for once
                if (!m_flags.m_queuedForBuildAttachment)
                {
                    PassSystemInterface::Get()->QueueForBuildAttachments(this);
                    m_flags.m_queuedForBuildAttachment = true;

                    // Set these two flags to false since when queue build attachments request, they should all be already be false except one use
                    // case that the pass system processed all queued requests when active a scene. 
                    m_flags.m_alreadyPrepared = false;
                    m_flags.m_alreadyReset = false;
                }
            }
        }

        void Pass::QueueForRemoval([[maybe_unused]] bool needsDeletion)
        {
            PassSystemInterface::Get()->QueueForRemoval(this);
        }

        // --- PassTemplate related functions ---

        void Pass::CreateSlotsFromTemplate()
        {
            if (m_template)
            {
                for (const PassSlot& slot : m_template->m_slots)
                {
                    PassAttachmentBinding binding(slot);
                    AddAttachmentBinding(binding);
                }
            }
        }

        void Pass::AttachBufferToSlot(AZStd::string_view slot, Data::Instance<Buffer> buffer)
        {
            AttachBufferToSlot(Name(slot), buffer);
        }

        void Pass::AttachBufferToSlot(const Name& slot, Data::Instance<Buffer> buffer)
        {
            PassAttachmentBinding* localBinding = FindAttachmentBinding(slot);
            if (!localBinding)
            {
                AZ_RPI_PASS_ERROR(false, "Pass::AttachBufferToSlot - Pass %s failed to find slot %s.",
                    m_path.GetCStr(), slot.GetCStr());
                return;
            }

            // We can't handle the case that there is already an attachment attached yet.
            // We could consider to add it later if there are needs. It may require remove from the owned attachment list and
            // handle the connected bindings
            if (localBinding->m_attachment)
            {
                AZ_RPI_PASS_ERROR(false, "Pass::AttachBufferToSlot - Slot %s already has attachment %s.",
                    slot.GetCStr(), localBinding->m_attachment->m_name.GetCStr());
                return;
            }

            PassBufferAttachmentDesc desc;
            desc.m_bufferDescriptor = buffer->GetRHIBuffer()->GetDescriptor();
            desc.m_lifetime = RHI::AttachmentLifetimeType::Imported;
            desc.m_name = buffer->GetAttachmentId();
            Ptr<PassAttachment> attachment = CreateAttachmentFromDesc(desc);
            attachment->m_importedResource = buffer;
            m_ownedAttachments.push_back(attachment);

            localBinding->SetAttachment(attachment);
            localBinding->m_originalAttachment = attachment;
        }
        
        void Pass::AttachImageToSlot(const Name& slot, Data::Instance<AttachmentImage> image)
        {
            PassAttachmentBinding* localBinding = FindAttachmentBinding(slot);
            if (!localBinding)
            {
                AZ_RPI_PASS_ERROR(false, "Pass::AttachImageToSlot - Pass %s failed to find slot %s.",
                    m_path.GetCStr(), slot.GetCStr());
                return;
            }

            // We can't handle the case that there is already an attachment attached yet.
            // We could consider to add it later if there are needs. It may require remove from the owned attachment list and
            // handle the connected bindings
            if (localBinding->m_attachment)
            {
                AZ_RPI_PASS_ERROR(false, "Pass::AttachImageToSlot - Slot %s already has attachment %s.",
                    slot.GetCStr(), localBinding->m_attachment->m_name.GetCStr());
                return;
            }

            PassImageAttachmentDesc desc;
            desc.m_imageDescriptor = image->GetRHIImage()->GetDescriptor();
            desc.m_lifetime = RHI::AttachmentLifetimeType::Imported;
            desc.m_name = image->GetAttachmentId();
            Ptr<PassAttachment> attachment = CreateAttachmentFromDesc(desc);
            attachment->m_importedResource = image;
            m_ownedAttachments.push_back(attachment);

            localBinding->SetAttachment(attachment);
            localBinding->m_originalAttachment = attachment;
        }               

        void Pass::ProcessConnection(const PassConnection& connection)
        {
            // Get the input from this pass that forms one end of the connection
            PassAttachmentBinding* localBinding = FindAttachmentBinding(connection.m_localSlot);
            if (!localBinding)
            {
                AZ_RPI_PASS_ERROR(false, "Pass::ProcessConnection - Pass %s failed to find slot %s.",
                    m_path.GetCStr(),
                    connection.m_localSlot.GetCStr());
                return;
            }

            Name connectedPassName = connection.m_attachmentRef.m_pass;
            Name connectedSlotName = connection.m_attachmentRef.m_attachment;
            Ptr<Pass> connectedPass;
            PassAttachmentBinding* connectedBinding = nullptr;
            bool foundPass = false;

            if (connectedPassName == PassNameThis)
            {
                foundPass = true;
                const Ptr<PassAttachment> attachment = FindOwnedAttachment(connectedSlotName);
                AZ_RPI_PASS_ERROR(attachment, "Pass::ProcessConnection - Pass %s doesn't own an attachment named %s.",
                    m_path.GetCStr(),
                    connectedSlotName.GetCStr());
                localBinding->SetAttachment(attachment);
                localBinding->m_originalAttachment = attachment;
                return;
            }

            if (m_parent)
            {
                if (connectedPassName == PassNameParent || connectedPassName == m_parent->GetName())
                {
                    foundPass = true;

                    // Get the connected binding from the parent
                    connectedBinding = m_parent->FindAttachmentBinding(connectedSlotName);

                    bool slotTypeMismatch = connectedBinding != nullptr &&
                        connectedBinding->m_slotType != localBinding->m_slotType &&
                        connectedBinding->m_slotType != PassSlotType::InputOutput &&
                        localBinding->m_slotType != PassSlotType::InputOutput;

                    if (slotTypeMismatch)
                    {
                        AZ_RPI_PASS_ERROR(false, "Pass::ProcessConnection - When connecting to a parent slot, both slots must be of the same type (or one must be InputOutput)");
                        connectedBinding = nullptr;
                    }
                }
                else
                {
                    // Use the connection Name to find a sibling pass
                    Ptr<Pass> siblingPass = m_parent->FindChildPass(connectedPassName);
                    foundPass = foundPass || (siblingPass != nullptr);
                    if (siblingPass)
                    {
                        connectedBinding = siblingPass->FindAttachmentBinding(connectedSlotName);

                        bool slotTypeMismatch = connectedBinding != nullptr &&
                            connectedBinding->m_slotType == localBinding->m_slotType &&
                            connectedBinding->m_slotType != PassSlotType::InputOutput;

                        if (slotTypeMismatch)
                        {
                            AZ_RPI_PASS_ERROR(false, "Pass::ProcessConnection - When connecting to a sibling slot, both slots must be of different types (or be InputOutputs)");
                            connectedBinding = nullptr;
                        }
                    }
                }
            }

            ParentPass* asParent = AsParent();
            if (!foundPass && asParent)
            {
                Ptr<Pass> childPass = asParent->FindChildPass(connectedPassName);
                foundPass = foundPass || (childPass != nullptr);
                if (childPass)
                {
                    connectedBinding = childPass->FindAttachmentBinding(connectedSlotName);

                    bool slotTypeMismatch = connectedBinding != nullptr &&
                        connectedBinding->m_slotType != localBinding->m_slotType &&
                        connectedBinding->m_slotType != PassSlotType::InputOutput &&
                        localBinding->m_slotType != PassSlotType::InputOutput;

                    if (slotTypeMismatch)
                    {
                        AZ_RPI_PASS_ERROR(false, "Pass::ProcessConnection - When connecting to a child slot, both slots must be of the same type (or one must be InputOutput)");
                        connectedBinding = nullptr;
                    }
                }
            }

            if (!connectedBinding)
            {
                if (!m_flags.m_partOfHierarchy)
                {
                    // [GFX TODO][ATOM-13693]: REMOVE POST R1 - passes not in hierarchy should no longer have this function called
                    // When view is changing, removal of the passes can occur (cascade shadow passes for example)
                    // resulting in temporary orphan passes that will be removed over the next frame.
                    AZ_RPI_PASS_WARNING(false,
                        "Pass::ProcessConnection - Pass [%s] is no longer part of the heirarchy and about to be removed",
                        m_path.GetCStr());
                }
                else if (foundPass)
                {
                    AZ_RPI_PASS_ERROR(false, "Pass::ProcessConnection - Pass [%s] couldn't find a valid binding [%s] on pass [%s].",
                        m_path.GetCStr(),
                        connectedSlotName.GetCStr(),
                        connectedPassName.GetCStr());
                }
                else
                {
                    AZ_RPI_PASS_ERROR(
                        false, "Pass::ProcessConnection - Pass [%s] is trying to connect to but could not find neighbor or child pass named [%s].",
                        m_path.GetCStr(),
                        connectedPassName.GetCStr());
                }
                return;
            }

            localBinding->m_connectedBinding = connectedBinding;
            UpdateConnectedBinding(*localBinding);
        }

        void Pass::ProcessFallbackConnection(const PassFallbackConnection& connection)
        {
            PassAttachmentBinding* inputBinding = FindAttachmentBinding(connection.m_inputSlotName);
            PassAttachmentBinding* outputBinding = FindAttachmentBinding(connection.m_outputSlotName);

            if (!outputBinding || !inputBinding)
            {
                AZ_RPI_PASS_ERROR(inputBinding, "Pass::ProcessFallbackConnection - Pass %s failed to find input slot %s.",
                    m_path.GetCStr(), connection.m_inputSlotName.GetCStr());

                AZ_RPI_PASS_ERROR(outputBinding, "Pass::ProcessFallbackConnection - Pass %s failed to find output slot %s.",
                    m_path.GetCStr(), connection.m_outputSlotName.GetCStr());

                return;
            }

            bool typesAreValid = inputBinding->m_slotType == PassSlotType::Input && outputBinding->m_slotType == PassSlotType::Output;

            if (!typesAreValid)
            {
                AZ_RPI_PASS_ERROR(inputBinding->m_slotType == PassSlotType::Input, "Pass::ProcessFallbackConnection - Pass %s specifies fallback connection input %s, which is not an input.",
                    m_path.GetCStr(), connection.m_inputSlotName.GetCStr());

                AZ_RPI_PASS_ERROR(outputBinding->m_slotType == PassSlotType::Output, "Pass::ProcessFallbackConnection - Pass %s specifies fallback connection output %s, which is not an output.",
                    m_path.GetCStr(), connection.m_inputSlotName.GetCStr());

                return;
            }

            outputBinding->m_fallbackBinding = inputBinding;
            UpdateConnectedBinding(*outputBinding);
        }

        template<typename AttachmentDescType>
        Ptr<PassAttachment> Pass::CreateAttachmentFromDesc(const AttachmentDescType& desc)
        {
            Ptr<PassAttachment> attachment = aznew PassAttachment(desc);

            // If the attachment is imported, we will create the resource (buffer or image) of this attachment
            // from asset referenced in m_assetRef
            // The resource instance will be saved in m_importedResource and the attachment id is acquired from resource instance
            if (desc.m_lifetime == RHI::AttachmentLifetimeType::Imported)
            {
                attachment->m_path = desc.m_name;
                if (attachment->m_descriptor.m_type == RHI::AttachmentType::Buffer)
                {
                    Data::Asset<BufferAsset> bufferAsset = AssetUtils::LoadAssetById<BufferAsset>(desc.m_assetRef.m_assetId, AssetUtils::TraceLevel::None);

                    if (bufferAsset.IsReady())
                    {
                        Data::Instance<Buffer> buffer = Buffer::FindOrCreate(bufferAsset);
                        if (buffer)
                        {
                            attachment->m_path = buffer->GetAttachmentId();
                            attachment->m_importedResource = buffer;
                        }
                    }
                }
                else if (attachment->m_descriptor.m_type == RHI::AttachmentType::Image)
                {
                    Data::Asset<AttachmentImageAsset> imageAsset = AssetUtils::LoadAssetById<AttachmentImageAsset>(desc.m_assetRef.m_assetId, AssetUtils::TraceLevel::None);

                    if (imageAsset.IsReady())
                    {
                        Data::Instance<AttachmentImage> image = AttachmentImage::FindOrCreate(imageAsset);
                        if (image)
                        {
                            attachment->m_path = image->GetAttachmentId();
                            attachment->m_importedResource = image;
                        }
                    }
                }
                else
                {
                    AZ_RPI_PASS_ASSERT(false, "Unsupported imported attachment type");
                }
            }
            else
            {
                // Only apply path name to transient attachment. Keep the original name for imported attachment
                attachment->ComputePathName(m_path);
            }

            // Setup attachment sources...

            if (desc.m_sizeSource.m_source.m_pass == PipelineKeyword)         // if source is pipeline
            {
                attachment->m_renderPipelineSource = m_pipeline;
                attachment->m_settingFlags.m_getSizeFromPipeline = true;
                attachment->m_sizeMultipliers = desc.m_sizeSource.m_multipliers;
            }
            else if (const PassAttachmentBinding* source = FindAdjacentBinding(desc.m_sizeSource.m_source))
            {
                attachment->m_sizeSource = source;
                attachment->m_sizeMultipliers = desc.m_sizeSource.m_multipliers;
            }

            if (desc.m_formatSource.m_pass == PipelineKeyword)                // if source is pipeline
            {
                attachment->m_renderPipelineSource = m_pipeline;
                attachment->m_settingFlags.m_getFormatFromPipeline = true;
            }
            else if (const PassAttachmentBinding* source = FindAdjacentBinding(desc.m_formatSource))
            {
                attachment->m_formatSource = source;
            }

            if (desc.m_multisampleSource.m_pass == PipelineKeyword)           // if source is pipeline
            {
                attachment->m_renderPipelineSource = m_pipeline;
                attachment->m_settingFlags.m_getMultisampleStateFromPipeline = true;
            }
            else if (const PassAttachmentBinding* source = FindAdjacentBinding(desc.m_multisampleSource))
            {
                attachment->m_multisampleSource = source;
            }

            if (const PassAttachmentBinding* source = FindAdjacentBinding(desc.m_arraySizeSource))
            {
                attachment->m_arraySizeSource = source;
            }

            attachment->m_ownerPass = this;

            return attachment;
        }

        void Pass::SetupInputsFromRequest()
        {
            if (m_flags.m_createdByPassRequest)
            {
                for (const PassConnection& connection : m_request.m_inputConnections)
                {
                    ProcessConnection(connection);
                }
            }
        }

        void Pass::SetupPassDependencies()
        {
            // Get dependencies declared in the PassRequest
            if (m_flags.m_createdByPassRequest)
            {
                for (const Name& passName : m_request.m_executeAfterPasses)
                {
                    Ptr<Pass> executeAfterPass = FindAdjacentPass(passName);
                    if (executeAfterPass != nullptr)
                    {
                        m_executeAfterPasses.push_back(executeAfterPass.get());
                    }
                }
                for (const Name& passName : m_request.m_executeBeforePasses)
                {
                    Ptr<Pass> executeBeforePass = FindAdjacentPass(passName);
                    if (executeBeforePass != nullptr)
                    {
                        m_executeBeforePasses.push_back(executeBeforePass.get());
                    }
                }
            }
            // Inherit dependencies from ParentPass
            if (m_parent)
            {
                for (Pass* pass : m_parent->m_executeAfterPasses)
                {
                    m_executeAfterPasses.push_back(pass);
                }
                for (Pass* pass : m_parent->m_executeBeforePasses)
                {
                    m_executeBeforePasses.push_back(pass);
                }
            }
        }

        void Pass::SetupOutputsFromTemplate()
        {
            if (m_template)
            {
                for (const PassConnection& outputConnection : m_template->m_outputConnections)
                {
                    ProcessConnection(outputConnection);
                }
                for (const PassFallbackConnection& fallbackConnection : m_template->m_fallbackConnections)
                {
                    ProcessFallbackConnection(fallbackConnection);
                }
            }
        }

        void Pass::CreateAttachmentsFromTemplate()
        {
            if (m_template)
            {
                // Create image attachments
                for (const PassImageAttachmentDesc& desc : m_template->m_imageAttachments)
                {
                    m_ownedAttachments.emplace_back(CreateAttachmentFromDesc(desc));
                }
                // Create buffer attachments
                for (const PassBufferAttachmentDesc& desc : m_template->m_bufferAttachments)
                {
                    m_ownedAttachments.emplace_back(CreateAttachmentFromDesc(desc));
                }
            }
        }

        // --- Attachment and Binding related functions ---

        void Pass::StoreImportedAttachmentReferences()
        {
            m_importedAttachmentStore.clear();

            for (const Ptr<PassAttachment>& attachment : m_ownedAttachments)
            {
                if (attachment->m_lifetime == RHI::AttachmentLifetimeType::Imported)
                {
                    m_importedAttachmentStore.push_back(attachment);
                }
            }
        }

        void Pass::CreateTransientAttachments(RHI::FrameGraphAttachmentInterface attachmentDatabase)
        {
            for (const Ptr<PassAttachment>& attachment : m_ownedAttachments)
            {
                if (attachment->m_lifetime == RHI::AttachmentLifetimeType::Transient)
                {
                    switch (attachment->m_descriptor.m_type)
                    {
                    case RHI::AttachmentType::Image:
                        attachmentDatabase.CreateTransientImage(attachment->GetTransientImageDescriptor());
                        break;
                    case RHI::AttachmentType::Buffer:
                        attachmentDatabase.CreateTransientBuffer(attachment->GetTransientBufferDescriptor());
                        break;
                    default:
                        AZ_RPI_PASS_ASSERT(false, "Error, transient attachment is neither an image nor a buffer!");
                        break;
                    }
                }
            }
        }

        void Pass::ImportAttachments(RHI::FrameGraphAttachmentInterface attachmentDatabase)
        {
            for (const Ptr<PassAttachment>& attachment : m_ownedAttachments)
            {
                if (attachment->m_lifetime == RHI::AttachmentLifetimeType::Imported)
                {
                    // make sure to only import the resource one time
                    RHI::AttachmentId attachmentId = attachment->GetAttachmentId();
                    if (!attachmentDatabase.IsAttachmentValid(attachmentId))
                    {
                        if (azrtti_istypeof<Image>(attachment->m_importedResource.get()))
                        {
                            Image* image = static_cast<Image*>(attachment->m_importedResource.get());
                            attachmentDatabase.ImportImage(attachmentId, image->GetRHIImage());
                        }
                        else if (azrtti_istypeof<Buffer>(attachment->m_importedResource.get()))
                        {
                            Buffer* buffer = static_cast<Buffer*>(attachment->m_importedResource.get());
                            attachmentDatabase.ImportBuffer(attachmentId, buffer->GetRHIBuffer());
                        }
                        else
                        {
                            AZ_RPI_PASS_ERROR(false, "Can't import unknown resource type");
                        }
                    }
                }
            }
        }

        void Pass::UpdateAttachmentUsageIndices()
        {
            // We want to find attachments that are used more than once by the same pass
            // An example of this could be reading from and writing to different mips of the same texture

            // Loop over all attachments bound to this pass
            size_t size = m_attachmentBindings.size();
            for (size_t i = 0; i < size; ++i)
            {
                PassAttachmentBinding& binding01 = m_attachmentBindings[i];

                // For the outer loop, only consider bindings which are the
                // first occurrence of their given attachment in the pass
                if (binding01.m_attachmentUsageIndex != 0)
                {
                    continue;
                }

                // Loop over all subsequent bindings in the pass
                uint8_t duplicateCount = 0;
                for (size_t j = i + 1; j < size; ++j)
                {
                    PassAttachmentBinding& binding02 = m_attachmentBindings[j];

                    // Bindings are considered having the same attachment if they are connected to the same binding...
                    bool haveSameConnection = binding01.m_connectedBinding == binding02.m_connectedBinding;
                    haveSameConnection = haveSameConnection && binding01.m_connectedBinding != nullptr;

                    // ... Or if they point to the same attachment
                    bool isSameAttachment = binding01.m_attachment == binding02.m_attachment;
                    isSameAttachment = isSameAttachment && binding01.m_attachment != nullptr;

                    // If binding 01 and binding 02 have the same attachment, update the attachment usage index on binding 02
                    if (haveSameConnection || isSameAttachment)
                    {
                        binding02.m_attachmentUsageIndex = ++duplicateCount;
                    }
                }
            }
        }

        void Pass::UpdateOwnedAttachments()
        {
            // Update the output attachments to coincide with their source attachments (if specified)
            // This involves getting the format and calculating the size from the source attachment
            for (Ptr<PassAttachment>& attachment: m_ownedAttachments)
            {
                attachment->Update();
            }
        }

        void Pass::UpdateConnectedBinding(PassAttachmentBinding& binding)
        {
            Ptr<PassAttachment> targetAttachment = nullptr;

            if (!m_flags.m_isBuildingAttachments && !IsEnabled() && binding.m_slotType == PassSlotType::Output && binding.m_fallbackBinding)
            {
                targetAttachment = binding.m_fallbackBinding->m_attachment;
            }
            else if(binding.m_connectedBinding)
            {
                targetAttachment = binding.m_connectedBinding->m_attachment;
            }
            else if (binding.m_originalAttachment != nullptr)
            {
                targetAttachment = binding.m_originalAttachment;
            }

            if (targetAttachment == nullptr)
            {
                return;
            }

            // Check whether the template's slot allows this attachment
            if (m_template && !m_template->AttachmentFitsSlot(targetAttachment->m_descriptor, binding.m_name))
            {
                AZ_RPI_PASS_ERROR(false, "Pass::UpdateConnectedBinding - Attachment %s did not match the filters of input slot %s on pass %s.",
                    targetAttachment->m_name.GetCStr(),
                    binding.m_name.GetCStr(),
                    m_path.GetCStr());

                binding.m_attachment = nullptr;
                return;
            }

            binding.SetAttachment(targetAttachment);
        }

        void Pass::UpdateConnectedBindings()
        {
            // Depending on whether a pass is enabled or not, it may switch it's bindings to become a pass-through
            // For this reason we update connecting bindings on a per-frame basis
            for (PassAttachmentBinding& binding : m_attachmentBindings)
            {
                UpdateConnectedBinding(binding);
            }
        }

        // --- Pass behavior functions ---

        void Pass::Reset()
        {
            // Flag prevents the function from executing multiple times a frame. Can happen
            // as pass system has a list of passes for which it needs to call this function.
            if (m_flags.m_alreadyReset)
            {
                return;
            }
            m_flags.m_alreadyReset = true;

            // Store references to imported attachments to underlying images and buffers aren't deleted during attachment building
            StoreImportedAttachmentReferences();

            // Clear lists
            m_inputBindingIndices.clear();
            m_inputOutputBindingIndices.clear();
            m_outputBindingIndices.clear();
            m_attachmentBindings.clear();
            m_ownedAttachments.clear();
            m_executeAfterPasses.clear();
            m_executeBeforePasses.clear();

            ResetInternal();
        }

        void Pass::BuildAttachments()
        {
            m_flags.m_queuedForBuildAttachment = false;

            // Flag prevents the function from executing multiple times a frame. Can happen
            // as pass system has a list of passes for which it needs to call this function.
            if (m_flags.m_alreadyPrepared)
            {
                return;
            }
            m_flags.m_alreadyPrepared = true;
            m_flags.m_isBuildingAttachments = true;

            AZ_RPI_BREAK_ON_TARGET_PASS;

            CreateSlotsFromTemplate();
            SetupInputsFromRequest();
            SetupPassDependencies();
            CreateAttachmentsFromTemplate();
            BuildAttachmentsInternal();
            SetupOutputsFromTemplate();
            UpdateConnectedBindings();
            UpdateOwnedAttachments();
            UpdateAttachmentUsageIndices();
            m_flags.m_isBuildingAttachments = false;
        }

        void Pass::OnBuildAttachmentsFinished()
        {
            AZ_RPI_BREAK_ON_TARGET_PASS;

            // These flags are to prevent a pass from being built multiple times.
            // We reset them after each build phase.
            m_flags.m_alreadyCreated = false;
            m_flags.m_alreadyPrepared = false;
            m_flags.m_alreadyReset = false;
            m_flags.m_queuedForBuildAttachment = false;
            m_importedAttachmentStore.clear();
            OnBuildAttachmentsFinishedInternal();
        }

        void Pass::Validate(PassValidationResults& validationResults)
        {
            if (PassValidation::IsEnabled())
            {
                // Log passes with missing input
                for (uint8_t idx : m_inputBindingIndices)
                {
                    if (!m_attachmentBindings[idx].m_attachment)
                    {
                        validationResults.m_passesWithMissingInputs.push_back(this);
                        break;
                    }
                }
                // Log passes with missing input/output
                for (uint8_t idx : m_inputOutputBindingIndices)
                {
                    if (!m_attachmentBindings[idx].m_attachment)
                    {
                        validationResults.m_passesWithMissingInputOutputs.push_back(this);
                        break;
                    }
                }
                // Log passes with missing output (note that missing output connections are not considered an error)
                for (uint8_t idx : m_outputBindingIndices)
                {
                    if (!m_attachmentBindings[idx].m_attachment)
                    {
                        validationResults.m_passesWithMissingOutputs.push_back(this);
                        break;
                    }
                }

                if (m_errorMessages.size() > 0)
                {
                    validationResults.m_passesWithErrors.push_back(this);
                }

                if (m_warningMessages.size() > 0)
                {
                    validationResults.m_passesWithWarnings.push_back(this);
                }
            }
        }

        void Pass::FrameBegin(FramePrepareParams params)
        {
            AZ_RPI_BREAK_ON_TARGET_PASS;

            if (!IsEnabled())
            {
                UpdateConnectedBindings();
                return;
            }
            m_flags.m_isRendering = true;

            UpdateConnectedBindings();
            UpdateOwnedAttachments();

            CreateTransientAttachments(params.m_frameGraphBuilder->GetAttachmentDatabase());
            ImportAttachments(params.m_frameGraphBuilder->GetAttachmentDatabase());

            // FrameBeginInternal needs to be the last function be called in FrameBegin because its implementation expects 
            // all the attachments are imported to database (for example, ImageAttachmentPreview)
            FrameBeginInternal(params);
        }

        void Pass::FrameEnd()
        {
            if (m_flags.m_isRendering)
            {
                FrameEndInternal();
                m_flags.m_isRendering = false;
            }
        }

        // --- RenderPipeline, PipelineViewTag and DrawListTag ---
        
        bool Pass::HasDrawListTag() const
        {
            return m_flags.m_hasDrawListTag;
        }
        
        RHI::DrawListTag Pass::GetDrawListTag() const
        {
            static RHI::DrawListTag invalidTag;
            return invalidTag;
        }

        bool Pass::HasPipelineViewTag() const
        {
            return m_flags.m_hasPipelineViewTag;
        }

        const PipelineViewTag& Pass::GetPipelineViewTag() const
        {
            static PipelineViewTag viewTag;
            return viewTag;
        }

        void Pass::SetRenderPipeline(RenderPipeline* pipeline)
        {
            m_pipeline = pipeline;
        }
        
        RenderPipeline* Pass::GetRenderPipeline() const
        {
            return m_pipeline;
        }

        Scene* Pass::GetScene() const
        {
            if (m_pipeline)
            {
                return m_pipeline->GetScene();
            }
            return nullptr;
        }

        void Pass::GetViewDrawListInfo(RHI::DrawListMask& outDrawListMask, PassesByDrawList& outPassesByDrawList, const PipelineViewTag& viewTag) const
        {
            // NOTE: we always collect the draw list mask regardless if the pass enabled or not. The reason is we want to keep the view information
            // even when pass is disabled so it can continue work correctly when re-enable it.

            // Only get the DrawListTag if this pass has a DrawListTag and it's PipelineViewId matches
            if (HasPipelineViewTag() && HasDrawListTag() && GetPipelineViewTag() == viewTag)
            {
                RHI::DrawListTag drawListTag = GetDrawListTag();
                if (drawListTag.IsValid() && outPassesByDrawList.find(drawListTag) == outPassesByDrawList.end())
                {
                    outPassesByDrawList[drawListTag] = this;
                    outDrawListMask.set(drawListTag.GetIndex());
                }
            }
        }

        void Pass::GetPipelineViewTags(SortedPipelineViewTags& outTags) const
        {
            if (HasPipelineViewTag())
            {
                outTags.insert(GetPipelineViewTag());
            }
        }

        void Pass::SortDrawList(RHI::DrawList& drawList) const
        {
            RHI::SortDrawList(drawList, m_drawListSortType);
        }

        // --- Debug & Validation functions ---

        bool PassValidationResults::IsValid()
        {
            if (PassValidation::IsEnabled())
            {
                // Pass validation fail if there are any passes with build errors or missing inputs (or input/outputs)
                return (m_passesWithErrors.size() == 0) && (m_passesWithMissingInputs.size() == 0) && (m_passesWithMissingInputOutputs.size() == 0);
            }
            else
            {
                return true;
            }
        }

        TimestampResult Pass::GetTimestampResult() const
        {
            if (IsEnabled() && IsTimestampQueryEnabled())
            {
                return GetTimestampResultInternal();
            }

            return TimestampResult();
        }

        PipelineStatisticsResult Pass::GetPipelineStatisticsResult() const
        {
            if (IsEnabled() && IsPipelineStatisticsQueryEnabled())
            {
                return GetPipelineStatisticsResultInternal();
            }

            return PipelineStatisticsResult();
        }

        TimestampResult Pass::GetTimestampResultInternal() const
        {
            return TimestampResult();
        }

        PipelineStatisticsResult Pass::GetPipelineStatisticsResultInternal() const
        {
            return PipelineStatisticsResult();
        }

        void Pass::SetTimestampQueryEnabled(bool enable)
        {
            m_flags.m_timestampQueryEnabled = enable;
        }

        void Pass::SetPipelineStatisticsQueryEnabled(bool enable)
        {
            m_flags.m_pipelineStatisticsQueryEnabled = enable;
        }

        bool Pass::IsTimestampQueryEnabled() const
        {
            return m_flags.m_timestampQueryEnabled;
        }

        bool Pass::IsPipelineStatisticsQueryEnabled() const
        {
            return m_flags.m_pipelineStatisticsQueryEnabled;
        }

        void Pass::PrintIndent(AZStd::string& stringOutput, uint32_t indent) const
        {
            if (PassValidation::IsEnabled())
            {
                for (uint32_t i = 0; i < indent; ++i)
                {
                    stringOutput += "   ";
                }
            }
        }

        void Pass::PrintPassName(AZStd::string& stringOutput, uint32_t indent) const
        {
            if (PassValidation::IsEnabled())
            {
                stringOutput += "\n";

                PrintIndent(stringOutput, indent);

                stringOutput += "- ";
                stringOutput += m_name.GetStringView();
                stringOutput += "\n";
            }
        }

        void Pass::PrintErrors() const
        {
            if (PassValidation::IsEnabled())
            {
                PrintMessages(m_errorMessages);
            }
        }

        void Pass::PrintWarnings() const
        {
            if (PassValidation::IsEnabled())
            {
                PrintMessages(m_warningMessages);
            }
        }

        void Pass::PrintMessages(const AZStd::vector<AZStd::string>& messages) const
        {
            if (PassValidation::IsEnabled())
            {
                AZStd::string stringOutput;
                PrintPassName(stringOutput);

                for (const AZStd::string& message : messages)
                {
                    PrintIndent(stringOutput, 1);
                    stringOutput += message;
                    stringOutput += "\n";
                }
                AZ_Printf("PassSystem", stringOutput.c_str());
            }
        }

        void Pass::PrintBindingsWithoutAttachments(uint32_t slotTypeMask) const
        {            
            if (PassValidation::IsEnabled())
            {
                AZStd::string stringOutput;
                PrintPassName(stringOutput);

                for (const PassAttachmentBinding& binding : m_attachmentBindings)
                {
                    uint32_t bindingMask = (1 << uint32_t(binding.m_slotType));
                    if ((bindingMask & slotTypeMask) && (binding.m_attachment == nullptr))
                    {
                        // Print the name of the slot
                        PrintIndent(stringOutput, 1);
                        stringOutput += binding.m_name.GetStringView();
                        stringOutput += " has no valid attachment\n";
                    }
                }
                AZ_Printf("PassSystem", stringOutput.c_str());
            }
        }

        void Pass::DebugPrintBinding(AZStd::string& stringOutput, const PassAttachmentBinding& binding) const
        {
            if (PassValidation::IsEnabled())
            {
                // Print the name of the slot
                stringOutput += binding.m_name.GetStringView();

                // Print the attachment type and size, for example:
                // (Image, 1920, 1080)   or  (Buffer, 4096 bytes)
                if (binding.m_attachment != nullptr)
                {
                    stringOutput += " (";

                    // Images will have the format: AttachmentName (Image, 1920, 1080)
                    if (binding.m_attachment->m_descriptor.m_type == RHI::AttachmentType::Image)
                    {
                        stringOutput += "Image";
                        RHI::ImageDescriptor& desc = binding.m_attachment->m_descriptor.m_image;
                        uint32_t dimensions = static_cast<uint32_t>(desc.m_dimension);
                        for (uint32_t i = 0; i < dimensions; ++i)
                        {
                            stringOutput += ", ";
                            stringOutput += AZStd::to_string(desc.m_size[i]);
                        }
                        if (desc.m_multisampleState.m_samples > 1)
                        {
                            if (desc.m_multisampleState.m_customPositionsCount > 0)
                            {
                                stringOutput += ", Custom_MSAA_";
                            }
                            else
                            {
                                stringOutput += ", MSAA_";
                            }
                            stringOutput += AZStd::to_string(desc.m_multisampleState.m_samples);
                            stringOutput += "x";
                        }
                    }
                    // Buffers will have the format: AttachmentName (Buffer, 4092 bytes)
                    else if (binding.m_attachment->m_descriptor.m_type == RHI::AttachmentType::Buffer)
                    {
                        stringOutput += "Buffer, ";
                        stringOutput += AZStd::to_string(binding.m_attachment->m_descriptor.m_buffer.m_byteCount);
                        stringOutput += " bytes";
                    }

                    stringOutput += ")";
                }
            }
        }

        void Pass::DebugPrintBindingAndConnection(AZStd::string& stringOutput, uint8_t bindingIndex) const
        {
            if (PassValidation::IsEnabled())
            {
                PrintIndent(stringOutput, m_treeDepth + 2);

                // Print the Attachment
                const PassAttachmentBinding& binding = m_attachmentBindings[bindingIndex];
                DebugPrintBinding(stringOutput, binding);

                // Print the Attachment it's connected to
                if (binding.m_connectedBinding != nullptr)
                {
                    stringOutput += " connected to ";
                    DebugPrintBinding(stringOutput, *binding.m_connectedBinding);
                }

                stringOutput += "\n";
            }
        }

        void Pass::DebugPrint() const
        {
            if (PassValidation::IsEnabled())
            {
                AZStd::string stringOutput;
                PrintPassName(stringOutput, m_treeDepth);

                // Print inputs
                if (m_inputBindingIndices.size() > 0)
                {
                    PrintIndent(stringOutput, m_treeDepth + 1);
                    stringOutput += "Inputs:\n";
                    for (uint8_t inputIndex : m_inputBindingIndices)
                    {
                        DebugPrintBindingAndConnection(stringOutput, inputIndex);
                    }
                }

                // Print input/outputs
                if (m_inputOutputBindingIndices.size() > 0)
                {
                    PrintIndent(stringOutput, m_treeDepth + 1);
                    stringOutput += "Input/Outputs:\n";
                    for (uint8_t inputIndex : m_inputOutputBindingIndices)
                    {
                        DebugPrintBindingAndConnection(stringOutput, inputIndex);
                    }
                }

                // Print outputs
                if (m_outputBindingIndices.size() > 0)
                {
                    PrintIndent(stringOutput, m_treeDepth + 1);
                    stringOutput += "Outputs:\n";
                    for (uint8_t inputIndex : m_outputBindingIndices)
                    {
                        DebugPrintBindingAndConnection(stringOutput, inputIndex);
                    }
                }
                AZ_Printf("PassSystem", stringOutput.c_str());
            }
        }

        void PassValidationResults::PrintValidationIfError()
        {
            if (PassValidation::IsEnabled())
            {
                if (!IsValid())
                {
                    AZ_Printf("PassSystem", "\n--- PASS VALIDATION FAILURE ---"
                        "\n--Critical Errors--\n");

                    AZ_Printf("PassSystem", "\nThere are %d passes with errors:\n", m_passesWithErrors.size());
                    for (Pass* pass : m_passesWithErrors)
                    {
                        pass->PrintErrors();
                    }

                    AZ_Printf("PassSystem", "\nThere are %d passes with missing Inputs:\n", m_passesWithMissingInputs.size());
                    for (Pass* pass : m_passesWithMissingInputs)
                    {
                        pass->PrintBindingsWithoutAttachments(uint32_t(PassSlotMask::Input));
                    }

                    AZ_Printf("PassSystem", "\nThere are %d passes with missing Inputs/Outputs:\n", m_passesWithMissingInputOutputs.size());
                    for (Pass* pass : m_passesWithMissingInputOutputs)
                    {
                        pass->PrintBindingsWithoutAttachments(uint32_t(PassSlotMask::InputOutput));
                    }

                    AZ_Printf("PassSystem", "\n--Non-Critical Errors/Warnings--\n");

                    AZ_Printf("PassSystem", "\nThere are %d passes with warnings:\n", m_passesWithWarnings.size());
                    for (Pass* pass : m_passesWithWarnings)
                    {
                        pass->PrintWarnings();
                    }
                }
            }
        }

    }   // namespace RPI
}   // namespace AZ
