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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/TargetManagement/TargetManagementAPI.h>

#include <Atom/Document/MaterialDocumentNotificationBus.h>
#include <Atom/Document/MaterialDocumentSystemRequestBus.h>
#include <Atom/RPI.Public/WindowContext.h>
#include <Document/MaterialDocument.h>
#include <Document/MaterialEditorSettings.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QFileInfo>
#include <QString>
AZ_POP_DISABLE_WARNING

namespace MaterialEditor
{
    //! MaterialDocumentSystemComponent is the central component of the Material Editor Core gem
    class MaterialDocumentSystemComponent
        : public AZ::Component
        , private AZ::TickBus::Handler
        , private AzFramework::TmMsgBus::Handler
        , private MaterialDocumentNotificationBus::Handler
        , private MaterialDocumentSystemRequestBus::Handler
    {
    public:
        AZ_COMPONENT(MaterialDocumentSystemComponent, "{58ABE0AE-2710-41E2-ADFD-E2D67407427D}");

        MaterialDocumentSystemComponent();
        ~MaterialDocumentSystemComponent() = default;
        MaterialDocumentSystemComponent(const MaterialDocumentSystemComponent&) = delete;
        MaterialDocumentSystemComponent& operator =(const MaterialDocumentSystemComponent&) = delete;

        static void Reflect(AZ::ReflectContext* context);

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    private:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // MaterialDocumentNotificationBus::Handler overrides...
        void OnDocumentDependencyModified(const AZ::Uuid& documentId) override;
        void OnDocumentExternallyModified(const AZ::Uuid& documentId) override;
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::TickBus::Handler overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        ////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::TmMsgBus::Handler overrides...
        void OnReceivedMsg(AzFramework::TmMsgPtr msg) override;
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // MaterialDocumentSystemRequestBus::Handler overrides...
        AZ::Uuid CreateDocument() override;
        bool DestroyDocument(const AZ::Uuid& documentId) override;
        AZ::Uuid OpenDocument(AZStd::string_view sourcePath) override;
        AZ::Uuid CreateDocumentFromFile(AZStd::string_view sourcePath, AZStd::string_view targetPath) override;
        bool CloseDocument(const AZ::Uuid& documentId) override;
        bool CloseAllDocuments() override;
        bool CloseAllDocumentsExcept(const AZ::Uuid& documentId) override;
        bool SaveDocument(const AZ::Uuid& documentId) override;
        bool SaveDocumentAsCopy(const AZ::Uuid& documentId, AZStd::string_view targetPath) override;
        bool SaveDocumentAsChild(const AZ::Uuid& documentId, AZStd::string_view targetPath) override;
        bool SaveAllDocuments() override;
        ////////////////////////////////////////////////////////////////////////

        AZ::Uuid OpenDocumentImpl(AZStd::string_view sourcePath, bool checkIfAlreadyOpen);

        AZStd::unordered_map<AZ::Uuid, AZStd::shared_ptr<MaterialDocument>> m_documentMap;
        AZStd::unordered_set<AZ::Uuid> m_documentIdsToRebuild;
        AZStd::unordered_set<AZ::Uuid> m_documentIdsToReopen;
        AZStd::unique_ptr<MaterialEditorSettings> m_settings;
    };
}
