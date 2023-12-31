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

#include <AtomImGuiToolsSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <AzFramework/Components/ConsoleBus.h>

namespace AtomImGuiTools
{
    void AtomImGuiToolsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AtomImGuiToolsSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AtomImGuiToolsSystemComponent>("AtomImGuiTools", "[Manager of various Atom ImGui tools.]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AtomImGuiToolsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("AtomImGuiToolsService"));
    }

    void AtomImGuiToolsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("AtomImGuiToolsService"));
    }

    void AtomImGuiToolsSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void AtomImGuiToolsSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void AtomImGuiToolsSystemComponent::Activate()
    {
#if defined(IMGUI_ENABLED)
        ImGui::ImGuiUpdateListenerBus::Handler::BusConnect();
#endif
        CrySystemEventBus::Handler::BusConnect();
    }

    void AtomImGuiToolsSystemComponent::Deactivate()
    {
#if defined(IMGUI_ENABLED)
        m_imguiPassTree.Reset();
        ImGui::ImGuiUpdateListenerBus::Handler::BusDisconnect();
#endif

        CrySystemEventBus::Handler::BusDisconnect();
    }

#if defined(IMGUI_ENABLED)
    void AtomImGuiToolsSystemComponent::OnImGuiUpdate()
    {
        if (m_showPassTree)
        {
            m_imguiPassTree.Draw(m_showPassTree, AZ::RPI::PassSystemInterface::Get()->GetRootPass().get());
        }
        if (m_showGpuProfiler)
        {
            m_imguiGpuProfiler.Draw(m_showGpuProfiler, AZ::RPI::PassSystemInterface::Get()->GetRootPass().get());
        }
    }

    void AtomImGuiToolsSystemComponent::OnImGuiMainMenuUpdate()
    {
        if (ImGui::BeginMenu("Atom Tools"))
        {
            if (ImGui::MenuItem("Pass Viewer", "", &m_showPassTree))
            {
            }

            if (ImGui::MenuItem("Gpu Profiler", "", &m_showGpuProfiler))
            {
            }
            ImGui::EndMenu();
        }
    }
#endif

    void AtomImGuiToolsSystemComponent::OnCryEditorInitialized()
    {
        AzFramework::ConsoleRequestBus::Broadcast(&AzFramework::ConsoleRequestBus::Events::ExecuteConsoleCommand, "imgui_DiscreteInputMode 1");
    }
} // namespace AtomImGuiTools
