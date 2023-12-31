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

#include <AzToolsFramework/ViewportUi/Button.h>
#include <AzToolsFramework/ViewportUi/Cluster.h>
#include <AzToolsFramework/ViewportUi/TextField.h>
#include <AzToolsFramework/ViewportUi/ViewportUiDisplay.h>

namespace AzToolsFramework::ViewportUi
{
    namespace Internal
    {
        class ViewportUiDisplay;
    }

    class ViewportUiManager : public ViewportUiRequestBus::Handler
    {
    public:
        ViewportUiManager() = default;
        ~ViewportUiManager() = default;
        
        // ViewportUiRequestBus ...
        const ClusterId CreateCluster() override;
        void SetClusterActiveButton(ClusterId clusterId, ButtonId buttonId) override;
        const ButtonId CreateClusterButton(ClusterId clusterId, const AZStd::string& icon) override;
        void RegisterClusterEventHandler(ClusterId clusterId, AZ::Event<ButtonId>::Handler& handler) override;
        void RemoveCluster(ClusterId clusterId) override;
        void SetClusterVisible(ClusterId clusterId, bool visible);
        void SetClusterGroupVisible(const AZStd::vector<ClusterId>& clusterGroup, bool visible) override;
        const TextFieldId CreateTextField(
            const AZStd::string& labelText, const AZStd::string& textFieldDefaultText,
            TextFieldValidationType validationType) override;
        void SetTextFieldText(TextFieldId textFieldId, const AZStd::string& text) override;
        void RegisterTextFieldCallback(
            TextFieldId textFieldId, AZ::Event<AZStd::string>::Handler& handler) override;
        void RemoveTextField(TextFieldId textFieldId) override;
        void SetTextFieldVisible(TextFieldId textFieldId, bool visible) override;
        void CreateComponentModeBorder(const AZStd::string& borderTitle) override;
        void RemoveComponentModeBorder() override;
        void PressButton(ClusterId clusterId, ButtonId buttonId) override;

        //! Connects to the correct viewportId bus address.
        void ConnectViewportUiBus(const int viewportId);
        //! Disconnects from the viewport request bus.
        void DisconnectViewportUiBus();
        //! Initializes the Viewport UI by attaching it to the given parent and render overlay.
        void InitializeViewportUi(QWidget* parent, QWidget* renderOverlay);
        //! Updates all registered elements to display up to date.
        void Update();

    protected:
        AZStd::unordered_map<ClusterId, AZStd::shared_ptr<Internal::Cluster>> m_clusters; //!< A map of all registered clusters.
        AZStd::unordered_map<TextFieldId, AZStd::shared_ptr<Internal::TextField>> m_textFields; //!< A map of all registered textFields.
        AZStd::unique_ptr<Internal::ViewportUiDisplay> m_viewportUi; //!< The lower level graphical API for Viewport UI.

    private:
        //! Register a new cluster and return its id.
        ClusterId RegisterNewCluster(AZStd::shared_ptr<Internal::Cluster>& cluster);
        //! Register a new text field and return its id.
        TextFieldId RegisterNewTextField(AZStd::shared_ptr<Internal::TextField>& textField);
        //! Update the corresponding ui element for the given cluster.
        void UpdateClusterUi(Internal::Cluster* cluster);
        //! Update the corresponding ui element for the given text field.
        void UpdateTextFieldUi(Internal::TextField* textField);
    };
} // namespace AzToolsFramework::ViewportUi
