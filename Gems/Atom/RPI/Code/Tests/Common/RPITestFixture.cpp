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

#include <Common/RPITestFixture.h>
#include <Common/RHI/Factory.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/AzCore_Traits_Platform.h>
#include <AzCore/IO/Path/Path.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzTest/Utils.h>

#include <Atom/RHI/Device.h>
#include <Atom/RHI.Reflect/ReflectSystemComponent.h>

#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAsset.h>
#include <Atom/RPI.Reflect/ResourcePoolAsset.h>
#include <Atom/RPI.Reflect/Asset/AssetHandler.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroupPool.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    // Expose AssetManagerComponent::Reflect function for testing
    class MyAssetManagerComponent : public AssetManagerComponent
    {
    public:
        static void Reflect(ReflectContext* reflection)
        {
            AssetManagerComponent::Reflect(reflection);
        }
    };

    JsonRegistrationContext* RPITestFixture::GetJsonRegistrationContext()
    {
        return m_jsonRegistrationContext.get();
    }

    void RPITestFixture::Reflect(ReflectContext* context)
    {
        MyAssetManagerComponent::Reflect(context);
        RHI::ReflectSystemComponent::Reflect(context);
        RPI::RPISystem::Reflect(context);
        Name::Reflect(context);
    }

    void RPITestFixture::SetUp()
    {
        AssetManagerTestFixture::SetUp();

        AZ::RPI::Validation::s_isEnabled = true;
        AZ::RHI::Validation::s_isEnabled = true;

        m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
        m_localFileIO.reset(aznew AZ::IO::LocalFileIO());
        AZ::IO::FileIOBase::SetInstance(m_localFileIO.get());

        AZ::IO::Path assetPath = AZ::Test::GetEngineRootPath();
        assetPath /= "Cache";
        AZ::IO::FileIOBase::GetInstance()->SetAlias("@assets@", assetPath.c_str());

        m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();
        m_jsonSystemComponent = AZStd::make_unique<AZ::JsonSystemComponent>();
        m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());

        Reflect(GetSerializeContext());
        Reflect(m_jsonRegistrationContext.get());

        NameDictionary::Create();

        m_rhiFactory.reset(aznew StubRHI::Factory());

        RPI::RPISystemDescriptor rpiSystemDescriptor;
        m_rpiSystem = AZStd::make_unique<RPI::RPISystem>();
        m_rpiSystem->Initialize(rpiSystemDescriptor);
        m_rpiSystem->InitializeSystemAssetsForTests();

        // Setup job context for job system
        JobManagerDesc desc;
        JobManagerThreadDesc threadDesc;
#if AZ_TRAIT_SET_JOB_PROCESSOR_ID
        threadDesc.m_cpuId = 0; // Don't set processors IDs on windows
#endif 

        uint32_t numWorkerThreads = AZStd::thread::hardware_concurrency();

        for (unsigned int i = 0; i < numWorkerThreads; ++i)
        {
            desc.m_workerThreads.push_back(threadDesc);
#if AZ_TRAIT_SET_JOB_PROCESSOR_ID
            threadDesc.m_cpuId++;
#endif 
        }

        m_jobManager = AZStd::make_unique<JobManager>(desc);
        m_jobContext = AZStd::make_unique<JobContext>(*m_jobManager);
        JobContext::SetGlobalContext(m_jobContext.get());

        m_assetSystemStub.Activate();
    }

    void RPITestFixture::TearDown()
    {
        // Flushing the tick bus queue since AZ::RHI::Factory:Register queues a function
        AZ::SystemTickBus::ClearQueuedEvents();

        m_assetSystemStub.Deactivate();

        JobContext::SetGlobalContext(nullptr);
        m_jobContext = nullptr;
        m_jobManager = nullptr;

        m_rpiSystem->Shutdown();
        m_rpiSystem = nullptr;
        m_rhiFactory = nullptr;

        NameDictionary::Destroy();

        m_jsonRegistrationContext->EnableRemoveReflection();
        m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());
        Reflect(m_jsonRegistrationContext.get());
        m_jsonRegistrationContext->DisableRemoveReflection();

        m_jsonRegistrationContext.reset();
        m_jsonSystemComponent.reset();

        AZ::IO::FileIOBase::SetInstance(m_priorFileIO);
        m_localFileIO.reset();

        AssetManagerTestFixture::TearDown();
    }

    AZ::RHI::Device* RPITestFixture::GetDevice()
    {
        return AZ::RHI::RHISystemInterface::Get()->GetDevice();
    }

    void RPITestFixture::ProcessQueuedSrgCompilations(Data::Asset<ShaderResourceGroupAsset> srgAsset)
    {
        Data::Instance<ShaderResourceGroupPool> srgPool = ShaderResourceGroupPool::FindOrCreate(srgAsset);
        srgPool->GetRHIPool()->CompileGroupsBegin();
        srgPool->GetRHIPool()->CompileGroupsForInterval(RHI::Interval(0, srgPool->GetRHIPool()->GetGroupsToCompileCount()));
        srgPool->GetRHIPool()->CompileGroupsEnd();
    }

}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
