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
#include <PhysX_precompiled.h>
#include <Source/SystemComponent.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/Utils.h>
#include <AzFramework/Physics/Material.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <PhysX/MeshAsset.h>
#include <PhysX/HeightFieldAsset.h>
#include <Source/RigidBody.h>
#include <Source/RigidBodyStatic.h>
#include <Source/Utils.h>
#include <Source/Collision.h>
#include <Source/Shape.h>
#include <Source/Joint.h>
#include <Source/SphereColliderComponent.h>
#include <Source/BoxColliderComponent.h>
#include <Source/CapsuleColliderComponent.h>
#include <Source/Pipeline/MeshAssetHandler.h>
#include <Source/Pipeline/HeightFieldAssetHandler.h>
#include <Source/PhysXCharacters/API/CharacterUtils.h>
#include <Source/PhysXCharacters/API/CharacterController.h>
#include <Source/WindProvider.h>

#ifdef PHYSX_EDITOR
#include <Source/EditorColliderComponent.h>
#include <Editor/EditorWindow.h>
#include <Editor/PropertyTypes.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#endif

#include <PhysX/Debug/PhysXDebugInterface.h>
#include <System/PhysXSystem.h>

namespace PhysX
{
    static const char* const DefaultWorldName = "PhysX Default";

    bool SystemComponent::VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        using GlobalCollisionDebugState = Debug::DebugDisplayData::GlobalCollisionDebugState;

        if (classElement.GetVersion() <= 1)
        {
            const int pvdTransportTypeElemIndex = classElement.FindElement(AZ_CRC("PvdTransportType", 0x91e0b21e));

            if (pvdTransportTypeElemIndex >= 0)
            {
                Debug::PvdTransportType pvdTransportTypeValue;
                AZ::SerializeContext::DataElementNode& pvdTransportElement = classElement.GetSubElement(pvdTransportTypeElemIndex);
                pvdTransportElement.GetData<Debug::PvdTransportType>(pvdTransportTypeValue);

                if (pvdTransportTypeValue == static_cast<Debug::PvdTransportType>(2))
                {
                    // version 2 removes Disabled (2) value default to Network instead.
                    const bool success = pvdTransportElement.SetData<Debug::PvdTransportType>(context, Debug::PvdTransportType::Network);
                    if (!success)
                    {
                        return false;
                    }
                }
            }
        }

        if (classElement.GetVersion() <= 2)
        {
            const int globalColliderDebugDrawElemIndex = classElement.FindElement(AZ_CRC("GlobalColliderDebugDraw", 0xca73ed43));

            if (globalColliderDebugDrawElemIndex >= 0)
            {
                bool oldGlobalColliderDebugDrawElemDebug = false;
                AZ::SerializeContext::DataElementNode& globalColliderDebugDrawElem = classElement.GetSubElement(globalColliderDebugDrawElemIndex);
                // Previously globalColliderDebugDraw was a bool indicating whether to always draw debug or to manually set on the element
                if (!globalColliderDebugDrawElem.GetData<bool>(oldGlobalColliderDebugDrawElemDebug))
                {
                    return false;
                }
                classElement.RemoveElement(globalColliderDebugDrawElemIndex);
                const GlobalCollisionDebugState newValue = oldGlobalColliderDebugDrawElemDebug ? GlobalCollisionDebugState::AlwaysOn : GlobalCollisionDebugState::Manual;
                classElement.AddElementWithData(context, "GlobalColliderDebugDraw", newValue);
            }
        }

        if (classElement.GetVersion() <= 3)
        {
            classElement.AddElementWithData(context, "ShowJointHierarchy", true);
            classElement.AddElementWithData(context, "JointHierarchyLeadColor",
                Debug::DebugDisplayData::JointLeadColor::Aquamarine);
            classElement.AddElementWithData(context, "JointHierarchyFollowerColor",
                Debug::DebugDisplayData::JointFollowerColor::Magenta);
            classElement.AddElementWithData(context, "jointHierarchyDistanceThreshold", 1.0f);
        }

        return true;
    }

    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        D6JointLimitConfiguration::Reflect(context);
        Pipeline::MeshAssetData::Reflect(context);

        PhysX::ReflectionUtils::ReflectPhysXOnlyApi(context);

        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SystemComponent, AZ::Component>()
                ->Version(1)
                ->Field("Enabled", &SystemComponent::m_enabled)
            ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<SystemComponent>("PhysX", "Global PhysX physics configuration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SystemComponent::m_enabled,
                    "Enabled", "Enables the PhysX system component.")
                ;
            }
        }
    }

    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("PhysXService", 0x75beae2d));
    }

    void SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("PhysXService", 0x75beae2d));
    }

    void SystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
    }

    void SystemComponent::GetDependentServices([[maybe_unused]]AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    SystemComponent::SystemComponent()
        : m_enabled(true)
        , m_onSystemInitializedHandler(
            [this](const AzPhysics::SystemConfiguration* config)
            {
                EnableAutoManagedPhysicsTick(config->m_autoManageSimulationUpdate);
            })
        , m_onSystemConfigChangedHandler(
            [this](const AzPhysics::SystemConfiguration* config)
            {
                EnableAutoManagedPhysicsTick(config->m_autoManageSimulationUpdate);
            })
    {
    }

    SystemComponent::~SystemComponent()
    {
        if (m_collisionRequests.Get() == this)
        {
            m_collisionRequests.Unregister(this);
        }
        if (m_physicsSystem.Get() == this)
        {
            m_physicsSystem.Unregister(this);
        }
    }

    // AZ::Component interface implementation
    void SystemComponent::Init()
    {
        //Old API interfaces
        if (m_collisionRequests.Get() == nullptr)
        {
            m_collisionRequests.Register(this);
        }
        if (m_physicsSystem.Get() == nullptr)
        {
            m_physicsSystem.Register(this);
        }
    }

    template<typename AssetHandlerT, typename AssetT>
    void RegisterAsset(AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>>& assetHandlers)
    {
        AssetHandlerT* handler = aznew AssetHandlerT();
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, AZ::AzTypeInfo<AssetT>::Uuid());
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, AssetHandlerT::s_assetFileExtension);
        assetHandlers.emplace_back(handler);
    }

    void SystemComponent::Activate()
    {
        if (!m_enabled)
        {
            return;
        }

        m_materialManager.Connect();
        m_defaultWorldComponent.Activate();

        // Assets related work
        auto* materialAsset = aznew AzFramework::GenericAssetHandler<Physics::MaterialLibraryAsset>("Physics Material", "Physics", "physmaterial");
        materialAsset->Register();
        m_assetHandlers.emplace_back(materialAsset);

        // Add asset types and extensions to AssetCatalog. Uses "AssetCatalogService".
        RegisterAsset<Pipeline::MeshAssetHandler, Pipeline::MeshAsset>(m_assetHandlers);
        RegisterAsset<Pipeline::HeightFieldAssetHandler, Pipeline::HeightFieldAsset>(m_assetHandlers);

        // Connect to relevant buses
        Physics::SystemRequestBus::Handler::BusConnect();
        PhysX::SystemRequestsBus::Handler::BusConnect();
        Physics::CollisionRequestBus::Handler::BusConnect();
        Physics::CharacterSystemRequestBus::Handler::BusConnect();

#ifdef PHYSX_EDITOR
        PhysX::Editor::RegisterPropertyTypes();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
#endif

        ActivatePhysXSystem();
    }

    void SystemComponent::Deactivate()
    {
#ifdef PHYSX_EDITOR
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
#endif
        AZ::TickBus::Handler::BusDisconnect();
        Physics::CharacterSystemRequestBus::Handler::BusDisconnect();
        Physics::CollisionRequestBus::Handler::BusDisconnect();
        PhysX::SystemRequestsBus::Handler::BusDisconnect();
        Physics::SystemRequestBus::Handler::BusDisconnect();

        // Reset material manager
        m_materialManager.ReleaseAllMaterials();

        m_defaultWorldComponent.Deactivate();
        m_materialManager.Disconnect();

        m_windProvider.reset();

        m_onSystemInitializedHandler.Disconnect();
        m_onSystemConfigChangedHandler.Disconnect();
        if (m_physXSystem != nullptr)
        {
            m_physXSystem->Shutdown();
            m_physXSystem = nullptr;
        }
        m_assetHandlers.clear(); //this need to be after m_physXSystem->Shutdown(); For it will drop the default material library reference.
    }

#ifdef PHYSX_EDITOR

    // AztoolsFramework::EditorEvents::Bus::Handler overrides
    void SystemComponent::PopulateEditorGlobalContextMenu([[maybe_unused]] QMenu* menu, [[maybe_unused]] const AZ::Vector2& point, [[maybe_unused]] int flags)
    {
    }

    void SystemComponent::NotifyRegisterViews()
    {
        PhysX::Editor::EditorWindow::RegisterViewClass();
    }
#endif

    // PhysXSystemComponentRequestBus interface implementation
    physx::PxScene* SystemComponent::CreateScene(physx::PxSceneDesc& sceneDesc)
    {
#ifdef PHYSX_ENABLE_MULTI_THREADING
        sceneDesc.flags |= physx::PxSceneFlag::eREQUIRE_RW_LOCK;
#endif
        //TEMP until this in moved over
        sceneDesc.cpuDispatcher = m_physXSystem->GetPxCpuDispathcher();
        return m_physXSystem->GetPxPhysics()->createScene(sceneDesc);
    }

    physx::PxConvexMesh* SystemComponent::CreateConvexMesh(const void* vertices, AZ::u32 vertexNum, AZ::u32 vertexStride)
    {
        physx::PxConvexMeshDesc desc;
        desc.points.data = vertices;
        desc.points.count = vertexNum;
        desc.points.stride = vertexStride;
        // we provide points only, therefore the PxConvexFlag::eCOMPUTE_CONVEX flag must be specified
        desc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

        //TEMP until this in moved over
        physx::PxConvexMesh* convex = m_physXSystem->GetPxCooking()->createConvexMesh(desc,
            m_physXSystem->GetPxPhysics()->getPhysicsInsertionCallback());
        AZ_Error("PhysX", convex, "Error. Unable to create convex mesh");

        return convex;
    }

    bool SystemComponent::CookConvexMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount)
    {
        AZStd::vector<AZ::u8> physxData;

        if (CookConvexMeshToMemory(vertices, vertexCount, physxData))
        {
            return Utils::WriteCookedMeshToFile(filePath, physxData, 
                Physics::CookedMeshShapeConfiguration::MeshType::Convex);
        }

        AZ_Error("PhysX", false, "CookConvexMeshToFile. Convex cooking failed for %s.", filePath.c_str());
        return false;
    }

    bool SystemComponent::CookTriangleMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount,
        const AZ::u32* indices, AZ::u32 indexCount)
    {   
        AZStd::vector<AZ::u8> physxData;

        if (CookTriangleMeshToMemory(vertices, vertexCount, indices, indexCount, physxData))
        {
            return Utils::WriteCookedMeshToFile(filePath, physxData,
                Physics::CookedMeshShapeConfiguration::MeshType::TriangleMesh);
        }

        AZ_Error("PhysX", false, "CookTriangleMeshToFile. Mesh cooking failed for %s.", filePath.c_str());
        return false;
    }

    bool SystemComponent::CookConvexMeshToMemory(const AZ::Vector3* vertices, AZ::u32 vertexCount, AZStd::vector<AZ::u8>& result)
    {
        physx::PxDefaultMemoryOutputStream memoryStream;
        
        bool cookingResult = Utils::CookConvexToPxOutputStream(vertices, vertexCount, memoryStream);
        
        if(cookingResult)
        {
            result.insert(result.end(), memoryStream.getData(), memoryStream.getData() + memoryStream.getSize());
        }
        
        return cookingResult;
    }

    bool SystemComponent::CookTriangleMeshToMemory(const AZ::Vector3* vertices, AZ::u32 vertexCount,
        const AZ::u32* indices, AZ::u32 indexCount, AZStd::vector<AZ::u8>& result)
    {
        physx::PxDefaultMemoryOutputStream memoryStream;
        bool cookingResult = Utils::CookTriangleMeshToToPxOutputStream(vertices, vertexCount, indices, indexCount, memoryStream);

        if (cookingResult)
        {
            result.insert(result.end(), memoryStream.getData(), memoryStream.getData() + memoryStream.getSize());
        }

        return cookingResult;
    }

    physx::PxConvexMesh* SystemComponent::CreateConvexMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize)
    {
        physx::PxDefaultMemoryInputData inpStream(reinterpret_cast<physx::PxU8*>(const_cast<void*>(cookedMeshData)), bufferSize);
        //TEMP until this in moved over
        return m_physXSystem->GetPxPhysics()->createConvexMesh(inpStream);
    }

    physx::PxTriangleMesh* SystemComponent::CreateTriangleMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize)
    {
        physx::PxDefaultMemoryInputData inpStream(reinterpret_cast<physx::PxU8*>(const_cast<void*>(cookedMeshData)), bufferSize);
        //TEMP until this in moved over
        return m_physXSystem->GetPxPhysics()->createTriangleMesh(inpStream);
    }

    AZStd::unique_ptr<Physics::RigidBodyStatic> SystemComponent::CreateStaticRigidBody(const Physics::WorldBodyConfiguration& configuration)
    {
        return AZStd::make_unique<PhysX::RigidBodyStatic>(configuration);
    }

    AZStd::unique_ptr<Physics::RigidBody> SystemComponent::CreateRigidBody(const Physics::RigidBodyConfiguration& configuration)
    {
        return AZStd::make_unique<RigidBody>(configuration);
    }

    AZStd::shared_ptr<Physics::Shape> SystemComponent::CreateShape(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& configuration)
    {
        auto shapePtr = AZStd::make_shared<PhysX::Shape>(colliderConfiguration, configuration);

        if (shapePtr->GetPxShape())
        {
            return shapePtr;
        }

        AZ_Error("PhysX", false, "SystemComponent::CreateShape error. Unable to create a shape from configuration.");

        return nullptr;
    }

    AZStd::shared_ptr<Physics::Material> SystemComponent::CreateMaterial(const Physics::MaterialConfiguration& materialConfiguration)
    {
        return AZStd::make_shared<PhysX::Material>(materialConfiguration);
    }

    AZStd::vector<AZStd::shared_ptr<Physics::Material>> SystemComponent::CreateMaterialsFromLibrary(const Physics::MaterialSelection& materialSelection)
    {
        AZStd::vector<physx::PxMaterial*> pxMaterials;
        m_materialManager.GetPxMaterials(materialSelection, pxMaterials);

        AZStd::vector<AZStd::shared_ptr<Physics::Material>> genericMaterials;
        genericMaterials.reserve(pxMaterials.size());

        for (physx::PxMaterial* pxMaterial : pxMaterials)
        {
            genericMaterials.push_back(static_cast<PhysX::Material*>(pxMaterial->userData)->shared_from_this());
        }

        return genericMaterials;
    }

    AZStd::shared_ptr<Physics::Material> SystemComponent::GetDefaultMaterial()
    {
        return m_materialManager.GetDefaultMaterial();
    }

    AZStd::vector<AZ::TypeId> SystemComponent::GetSupportedJointTypes()
    {
        return JointUtils::GetSupportedJointTypes();
    }

    AZStd::shared_ptr<Physics::JointLimitConfiguration> SystemComponent::CreateJointLimitConfiguration(AZ::TypeId jointType)
    {
        return JointUtils::CreateJointLimitConfiguration(jointType);
    }

    AZStd::shared_ptr<Physics::Joint> SystemComponent::CreateJoint(const AZStd::shared_ptr<Physics::JointLimitConfiguration>& configuration,
        Physics::WorldBody* parentBody, Physics::WorldBody* childBody)
    {
        return JointUtils::CreateJoint(configuration, parentBody, childBody);
    }

    void SystemComponent::GenerateJointLimitVisualizationData(
        const Physics::JointLimitConfiguration& configuration,
        const AZ::Quaternion& parentRotation,
        const AZ::Quaternion& childRotation,
        float scale,
        AZ::u32 angularSubdivisions,
        AZ::u32 radialSubdivisions,
        AZStd::vector<AZ::Vector3>& vertexBufferOut,
        AZStd::vector<AZ::u32>& indexBufferOut,
        AZStd::vector<AZ::Vector3>& lineBufferOut,
        AZStd::vector<bool>& lineValidityBufferOut)
    {
        JointUtils::GenerateJointLimitVisualizationData(configuration, parentRotation, childRotation, scale,
            angularSubdivisions, radialSubdivisions, vertexBufferOut, indexBufferOut, lineBufferOut, lineValidityBufferOut);
    }

    AZStd::unique_ptr<Physics::JointLimitConfiguration> SystemComponent::ComputeInitialJointLimitConfiguration(
        const AZ::TypeId& jointLimitTypeId,
        const AZ::Quaternion& parentWorldRotation,
        const AZ::Quaternion& childWorldRotation,
        const AZ::Vector3& axis,
        const AZStd::vector<AZ::Quaternion>& exampleLocalRotations)
    {
        return JointUtils::ComputeInitialJointLimitConfiguration(jointLimitTypeId, parentWorldRotation,
            childWorldRotation, axis, exampleLocalRotations);
    }

    void SystemComponent::ReleaseNativeMeshObject(void* nativeMeshObject)
    {
        if (nativeMeshObject)
        {
            static_cast<physx::PxBase*>(nativeMeshObject)->release();
        }
    }

    void SystemComponent::AddColliderComponentToEntity(AZ::Entity* entity, const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& shapeConfiguration, [[maybe_unused]] bool addEditorComponents)
    {
        Physics::ShapeType shapeType = shapeConfiguration.GetShapeType();

#ifdef PHYSX_EDITOR
        if (addEditorComponents)
        {
            entity->CreateComponent<EditorColliderComponent>(colliderConfiguration, shapeConfiguration);
        }
        else
#else
        {
            if (shapeType == Physics::ShapeType::Sphere)
            {
                const Physics::SphereShapeConfiguration& sphereConfiguration = static_cast<const Physics::SphereShapeConfiguration&>(shapeConfiguration);
                auto sphereColliderComponent = entity->CreateComponent<SphereColliderComponent>();
                sphereColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(
                    AZStd::make_shared<Physics::ColliderConfiguration>(colliderConfiguration),
                    AZStd::make_shared<Physics::SphereShapeConfiguration>(sphereConfiguration)) });
            }
            else if (shapeType == Physics::ShapeType::Box)
            {
                const Physics::BoxShapeConfiguration& boxConfiguration = static_cast<const Physics::BoxShapeConfiguration&>(shapeConfiguration);
                auto boxColliderComponent = entity->CreateComponent<BoxColliderComponent>();
                boxColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(
                    AZStd::make_shared<Physics::ColliderConfiguration>(colliderConfiguration),
                    AZStd::make_shared<Physics::BoxShapeConfiguration>(boxConfiguration)) });
            }
            else if (shapeType == Physics::ShapeType::Capsule)
            {
                const Physics::CapsuleShapeConfiguration& capsuleConfiguration = static_cast<const Physics::CapsuleShapeConfiguration&>(shapeConfiguration);
                auto capsuleColliderComponent = entity->CreateComponent<CapsuleColliderComponent>();
                capsuleColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(
                    AZStd::make_shared<Physics::ColliderConfiguration>(colliderConfiguration),
                    AZStd::make_shared<Physics::CapsuleShapeConfiguration>(capsuleConfiguration)) });
            }
        }

        AZ_Error("PhysX System", !addEditorComponents, "AddColliderComponentToEntity(): Trying to add an Editor collider component in a stand alone build.",
            static_cast<AZ::u8>(shapeType));

#endif
        {
            AZ_Error("PhysX System", shapeType == Physics::ShapeType::Sphere || shapeType == Physics::ShapeType::Box || shapeType == Physics::ShapeType::Capsule,
                "AddColliderComponentToEntity(): Using Shape of type %d is not implemented.", static_cast<AZ::u8>(shapeType));
        }
    }

    // Physics::CharacterSystemRequestBus
    AZStd::unique_ptr<Physics::Character> SystemComponent::CreateCharacter(const Physics::CharacterConfiguration&
        characterConfig, const Physics::ShapeConfiguration& shapeConfig, Physics::World& world)
    {
        return Utils::Characters::CreateCharacterController(characterConfig, shapeConfig, world);
    }

    void SystemComponent::UpdateCharacters(Physics::World& world, float deltaTime)
    {
        if (auto physxWorld = azrtti_cast<PhysX::World*>(&world))
        {
            auto manager = physxWorld->GetOrCreateControllerManager();
            if (manager)
            {
                manager->computeInteractions(deltaTime);
            }
        }
    }

    AzPhysics::CollisionLayer SystemComponent::GetCollisionLayerByName(const AZStd::string& layerName)
    {
        return m_physXSystem->GetPhysXConfiguration().m_collisionConfig.m_collisionLayers.GetLayer(layerName);
    }

    AZStd::string SystemComponent::GetCollisionLayerName(const AzPhysics::CollisionLayer& layer)
    {
        return m_physXSystem->GetPhysXConfiguration().m_collisionConfig.m_collisionLayers.GetName(layer);
    }

    bool SystemComponent::TryGetCollisionLayerByName(const AZStd::string& layerName, AzPhysics::CollisionLayer& layer)
    {
        return m_physXSystem->GetPhysXConfiguration().m_collisionConfig.m_collisionLayers.TryGetLayer(layerName, layer);
    }

    AzPhysics::CollisionGroup SystemComponent::GetCollisionGroupByName(const AZStd::string& groupName)
    {
        return m_physXSystem->GetPhysXConfiguration().m_collisionConfig.m_collisionGroups.FindGroupByName(groupName);
    }

    bool SystemComponent::TryGetCollisionGroupByName(const AZStd::string& groupName, AzPhysics::CollisionGroup& group)
    {
        return m_physXSystem->GetPhysXConfiguration().m_collisionConfig.m_collisionGroups.TryFindGroupByName(groupName, group);
    }

    AZStd::string SystemComponent::GetCollisionGroupName(const AzPhysics::CollisionGroup& collisionGroup)
    {
        AZStd::string groupName;
        for (const auto& group : m_physXSystem->GetPhysXConfiguration().m_collisionConfig.m_collisionGroups.GetPresets())
        {
            if (group.m_group.GetMask() == collisionGroup.GetMask())
            {
                groupName = group.m_name;
                break;
            }
        }
        return groupName;
    }

    AzPhysics::CollisionGroup SystemComponent::GetCollisionGroupById(const AzPhysics::CollisionGroups::Id& groupId)
    {
        return m_physXSystem->GetPhysXConfiguration().m_collisionConfig.m_collisionGroups.FindGroupById(groupId);
    }

    void SystemComponent::SetCollisionLayerName(int index, const AZStd::string& layerName)
    {
        m_physXSystem->SetCollisionLayerName(aznumeric_cast<AZ::u64>(index), layerName);
    }

    void SystemComponent::CreateCollisionGroup(const AZStd::string& groupName, const AzPhysics::CollisionGroup& group)
    {
        m_physXSystem->CreateCollisionGroup(groupName, group);
    }

    AzPhysics::CollisionConfiguration SystemComponent::GetCollisionConfiguration()
    {
        return m_physXSystem->GetPhysXConfiguration().m_collisionConfig;
    }

    physx::PxFilterData SystemComponent::CreateFilterData(const AzPhysics::CollisionLayer& layer, const AzPhysics::CollisionGroup& group)
    {
        return PhysX::Collision::CreateFilterData(layer, group);
    }

    physx::PxCooking* SystemComponent::GetCooking()
    {
        //TEMP until this in moved over
        return m_physXSystem->GetPxCooking();
    }

    bool SystemComponent::UpdateMaterialSelection(const Physics::ShapeConfiguration& shapeConfiguration,
        Physics::ColliderConfiguration& colliderConfiguration)
    {
        Physics::MaterialSelection& materialSelection = colliderConfiguration.m_materialSelection;

        // If the material library is still not set, we can't update the material selection
        if (!materialSelection.IsMaterialLibraryValid())
        {
            AZ_Warning("PhysX", false,
                "UpdateMaterialSelection: Material Selection tried to use an invalid/non-existing Physics material library: \"%s\". "
                "Please make sure the file exists or re-assign another library", materialSelection.GetMaterialLibraryAssetHint().c_str());
            return false;
        }

        // If there's no material library data loaded, try to load it
        if (materialSelection.GetMaterialLibraryAssetData() == nullptr)
        {
            AZ::Data::AssetId materialLibraryAssetId = materialSelection.GetMaterialLibraryAssetId();
            materialSelection.SetMaterialLibrary(materialLibraryAssetId);
        }

        // If there's still not material library data, we can't update the material selection 
        if (materialSelection.GetMaterialLibraryAssetData() == nullptr)
        {
            AZ::Data::AssetId materialLibraryAssetId = materialSelection.GetMaterialLibraryAssetId();

            auto materialLibraryAsset =
                AZ::Data::AssetManager::Instance().GetAsset<Physics::MaterialLibraryAsset>(materialLibraryAssetId, AZ::Data::AssetLoadBehavior::Default);

            materialLibraryAsset.BlockUntilLoadComplete();

            // Log the asset path to help find out the incorrect library reference
            AZStd::string assetPath = materialLibraryAsset.GetHint();
            AZ_Warning("PhysX", false,
                "UpdateMaterialSelection: Unable to load the material library for a material selection."
                " Please check if the asset %s exists in the asset cache.", assetPath.c_str());

            return false;
        }

        if (shapeConfiguration.GetShapeType() == Physics::ShapeType::PhysicsAsset)
        {
            const Physics::PhysicsAssetShapeConfiguration& assetConfiguration =
                static_cast<const Physics::PhysicsAssetShapeConfiguration&>(shapeConfiguration);

            // Use the materials data from the asset to update the collider data
            return UpdateMaterialSelectionFromPhysicsAsset(assetConfiguration, colliderConfiguration);
        }

        return true;
    }

    void SystemComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_physXSystem)
        {
            m_physXSystem->Simulate(deltaTime);
        }
    }

    int SystemComponent::GetTickOrder()
    {
        return AZ::ComponentTickBus::TICK_PHYSICS_SYSTEM;
    }

    void SystemComponent::EnableAutoManagedPhysicsTick(bool shouldTick)
    {
        if (shouldTick && !m_isTickingPhysics)
        {
            AZ::TickBus::Handler::BusConnect();
        }
        else if (!shouldTick && m_isTickingPhysics)
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
        m_isTickingPhysics = shouldTick;
    }

    void SystemComponent::ActivatePhysXSystem()
    {
        // Set Physics API constants to PhysX-friendly values
        Physics::DefaultRigidBodyConfiguration::m_computeInertiaTensor = true;
        Physics::DefaultRigidBodyConfiguration::m_sleepMinEnergy = 0.005f;

        if (m_physXSystem = GetPhysXSystem())
        {
            m_physXSystem->RegisterSystemInitializedEvent(m_onSystemInitializedHandler);
            m_physXSystem->RegisterSystemConfigurationChangedEvent(m_onSystemConfigChangedHandler);
            const PhysXSettingsRegistryManager& registryManager = m_physXSystem->GetSettingsRegistryManager();
            if (AZStd::optional<PhysXSystemConfiguration> config = registryManager.LoadSystemConfiguration();
                config.has_value())
            {
                m_physXSystem->Initialize(&(*config));
            }
            else //load defaults if there is no config
            {
                const PhysXSystemConfiguration defaultConfig = PhysXSystemConfiguration::CreateDefault();
                m_physXSystem->Initialize(&defaultConfig);
                registryManager.SaveSystemConfiguration(defaultConfig, {});
            }

            //Load the DefaultSceneConfig
            if (AZStd::optional<AzPhysics::SceneConfiguration> config = registryManager.LoadDefaultSceneConfiguration();
                config.has_value())
            {
                m_physXSystem->UpdateDefaultSceneConfiguration((*config));
            }
            else //load defaults if there is no config
            {
                const AzPhysics::SceneConfiguration defaultConfig = AzPhysics::SceneConfiguration::CreateDefault();
                m_physXSystem->UpdateDefaultSceneConfiguration(defaultConfig);
                registryManager.SaveDefaultSceneConfiguration(defaultConfig, {});
            }

            //load the debug configuration and initialize the PhysX debug interface
            if (auto* debug = AZ::Interface<Debug::PhysXDebugInterface>::Get())
            {
                if (AZStd::optional<Debug::DebugConfiguration> config = registryManager.LoadDebugConfiguration();
                    config.has_value())
                {
                    debug->Initialize(*config);
                }
                else //load defaults if there is no config
                {
                    const Debug::DebugConfiguration defaultConfig = Debug::DebugConfiguration::CreateDefault();
                    debug->Initialize(defaultConfig);
                    registryManager.SaveDebugConfiguration(defaultConfig, {});
                }
            }
        }

        m_windProvider = AZStd::make_unique<WindProvider>();
    }

    bool SystemComponent::UpdateMaterialSelectionFromPhysicsAsset(
        const Physics::PhysicsAssetShapeConfiguration& assetConfiguration,
        Physics::ColliderConfiguration& colliderConfiguration)
    {
        Physics::MaterialSelection& materialSelection = colliderConfiguration.m_materialSelection;

        if (!assetConfiguration.m_asset.GetId().IsValid())
        {
            // Set the default selection if there's no physics asset.
            materialSelection.SetMaterialSlots(Physics::MaterialSelection::SlotsArray());
            return false;
        }

        if (!assetConfiguration.m_asset.IsReady())
        {
            // The asset is valid but is still loading, 
            // Do not set the empty slots in this case to avoid the entity being in invalid state
            return false;
        }

        Pipeline::MeshAsset* meshAsset = assetConfiguration.m_asset.GetAs<Pipeline::MeshAsset>();
        if (!meshAsset)
        {
            materialSelection.SetMaterialSlots(Physics::MaterialSelection::SlotsArray());
            AZ_Warning("PhysX", false, "UpdateMaterialSelectionFromPhysicsAsset: MeshAsset is invalid");
            return false;
        }

        // Set the slots from the mesh asset
        materialSelection.SetMaterialSlots(meshAsset->m_assetData.m_surfaceNames);

        if (!assetConfiguration.m_useMaterialsFromAsset)
        {
            return false;
        }

        const Physics::MaterialLibraryAsset* materialLibrary = materialSelection.GetMaterialLibraryAssetData();
        const AZStd::vector<AZStd::string>& meshMaterialNames = meshAsset->m_assetData.m_materialNames;

        // Update material IDs in the selection for each slot
        int slotIndex = 0;
        for (const AZStd::string& meshMaterialName : meshMaterialNames)
        {
            Physics::MaterialFromAssetConfiguration materialData;
            bool found = materialLibrary->GetDataForMaterialName(meshMaterialName, materialData);

            AZ_Warning("PhysX", found, 
                "UpdateMaterialSelectionFromPhysicsAsset: No material found for surfaceType (%s) in the collider material library", 
                meshMaterialName.c_str());

            if (found)
            {
                materialSelection.SetMaterialId(materialData.m_id, slotIndex);
            }

            slotIndex++;
        }

        return true;
    }
} // namespace PhysX
