/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Asset/AssetManager.h>

#include <Atom/ImageProcessing/ImageProcessingBus.h>

namespace ImageProcessingAtom
{
    //! Builder to process images
    class ImageBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_RTTI(ImageBuilderWorker, "{7F1FA09D-77F3-4118-A7D5-4906BED59C19}");

        ImageBuilderWorker() = default;
        ~ImageBuilderWorker() = default;

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        //////////////////////////////////////////////////////////////////////////
        //!AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override; // if you get this you must fail all existing jobs and return.
        //////////////////////////////////////////////////////////////////////////

    private:
        bool m_isShuttingDown = false;
    };

    //! BuilderPluginComponent is to handle the lifecycle of ImageBuilder module.
    class BuilderPluginComponent
        : public AZ::Component
        , protected ImageProcessingRequestBus::Handler
    {
    public:
        AZ_COMPONENT(BuilderPluginComponent, "{A227F803-D2E4-406E-93EC-121EF45A64A1}")
        static void Reflect(AZ::ReflectContext* context);

        BuilderPluginComponent(); // avoid initialization here.
        ~BuilderPluginComponent() override; // free memory an uninitialize yourself.

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override; // create objects, allocate memory and initialize yourself without reaching out to the outside world
        void Activate() override; // reach out to the outside world and connect up to what you need to, register things, etc.
        void Deactivate() override; // unregister things, disconnect from the outside world

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        //////////////////////////////////////////////////////////////////////////

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AtomImageProcessingRequestBus interface implementation
        IImageObject* LoadImage(const AZStd::string& filePath) override;
        ////////////////////////////////////////////////////////////////////////

    private:
        BuilderPluginComponent(const BuilderPluginComponent&) = delete;

        ImageBuilderWorker m_imageBuilder;

        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;
    };
}// namespace ImageProcessingAtom
