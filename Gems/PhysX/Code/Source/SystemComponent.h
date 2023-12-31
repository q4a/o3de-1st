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

#include <PxPhysicsAPI.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/Material.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>

#include <PhysX/SystemComponentBus.h>
#include <PhysX/Configuration/PhysXConfiguration.h>
#include <Configuration/PhysXSettingsRegistryManager.h>
#include <DefaultWorldComponent.h>
#include <World.h>
#include <Material.h>

#ifdef PHYSX_EDITOR
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#endif

namespace PhysX
{
    class WindProvider;
    class PhysXSystem;

    /// System component for PhysX.
    /// The system component handles underlying tasks such as initialization and shutdown of PhysX, managing a
    /// Lumberyard memory allocator for PhysX allocations, scheduling for PhysX jobs, and connections to the PhysX
    /// Visual Debugger.  It also owns fundamental PhysX objects which manage worlds, rigid bodies, shapes, materials,
    /// constraints etc., and perform cooking (processing assets such as meshes and heightfields ready for use in PhysX).
    class SystemComponent
        : public AZ::Component
        , public Physics::SystemRequestBus::Handler
        , public PhysX::SystemRequestsBus::Handler
        , public Physics::CharacterSystemRequestBus::Handler
#ifdef PHYSX_EDITOR
        , public AzToolsFramework::EditorEntityContextNotificationBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
#endif
        , private Physics::CollisionRequestBus::Handler
        , private AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(SystemComponent, "{85F90819-4D9A-4A77-AB89-68035201F34B}");

        SystemComponent();
        ~SystemComponent();

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        SystemComponent(const SystemComponent&) = delete;

        // SystemRequestsBus
        physx::PxScene* CreateScene(physx::PxSceneDesc& sceneDesc) override;
        physx::PxConvexMesh* CreateConvexMesh(const void* vertices, AZ::u32 vertexNum, AZ::u32 vertexStride) override; // should we use AZ::Vector3* or physx::PxVec3 here?
        physx::PxConvexMesh* CreateConvexMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize) override;
        physx::PxTriangleMesh* CreateTriangleMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize) override;


        bool CookConvexMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount) override;
        
        bool CookConvexMeshToMemory(const AZ::Vector3* vertices, AZ::u32 vertexCount, AZStd::vector<AZ::u8>& result) override;

        bool CookTriangleMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount,
            const AZ::u32* indices, AZ::u32 indexCount) override;

        bool CookTriangleMeshToMemory(const AZ::Vector3* vertices, AZ::u32 vertexCount,
            const AZ::u32* indices, AZ::u32 indexCount, AZStd::vector<AZ::u8>& result) override;

        void AddColliderComponentToEntity(AZ::Entity* entity, const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& shapeConfiguration, bool addEditorComponents = false) override;

        physx::PxFilterData CreateFilterData(const AzPhysics::CollisionLayer& layer, const AzPhysics::CollisionGroup& group) override;
        physx::PxCooking* GetCooking() override;

        // Physics::CharacterSystemRequestBus
        virtual AZStd::unique_ptr<Physics::Character> CreateCharacter(const Physics::CharacterConfiguration& characterConfig,
            const Physics::ShapeConfiguration& shapeConfig, Physics::World& world) override;
        virtual void UpdateCharacters(Physics::World& world, float deltaTime) override;

        // CollisionRequestBus
        AzPhysics::CollisionLayer GetCollisionLayerByName(const AZStd::string& layerName) override;
        AZStd::string GetCollisionLayerName(const AzPhysics::CollisionLayer& layer) override;
        bool TryGetCollisionLayerByName(const AZStd::string& layerName, AzPhysics::CollisionLayer& layer) override;
        AzPhysics::CollisionGroup GetCollisionGroupByName(const AZStd::string& groupName) override;
        bool TryGetCollisionGroupByName(const AZStd::string& layerName, AzPhysics::CollisionGroup& group) override;
        AZStd::string GetCollisionGroupName(const AzPhysics::CollisionGroup& collisionGroup) override;
        AzPhysics::CollisionGroup GetCollisionGroupById(const AzPhysics::CollisionGroups::Id& groupId) override;
        void SetCollisionLayerName(int index, const AZStd::string& layerName) override;
        void CreateCollisionGroup(const AZStd::string& groupName, const AzPhysics::CollisionGroup& group) override;
        AzPhysics::CollisionConfiguration GetCollisionConfiguration() override;

        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;

#ifdef PHYSX_EDITOR

        // AztoolsFramework::EditorEvents::Bus::Handler overrides
        void PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags) override;
        void NotifyRegisterViews() override;
#endif

        // Physics::SystemRequestBus::Handler
        AZStd::unique_ptr<Physics::RigidBodyStatic> CreateStaticRigidBody(const Physics::WorldBodyConfiguration& configuration) override;
        AZStd::unique_ptr<Physics::RigidBody> CreateRigidBody(const Physics::RigidBodyConfiguration& configuration) override;
        AZStd::shared_ptr<Physics::Shape> CreateShape(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& configuration) override;
        AZStd::shared_ptr<Physics::Material> CreateMaterial(const Physics::MaterialConfiguration& materialConfiguration) override;
        AZStd::shared_ptr<Physics::Material> GetDefaultMaterial() override;
        AZStd::vector<AZStd::shared_ptr<Physics::Material>> CreateMaterialsFromLibrary(const Physics::MaterialSelection& materialSelection) override;

        AZStd::vector<AZ::TypeId> GetSupportedJointTypes() override;
        AZStd::shared_ptr<Physics::JointLimitConfiguration> CreateJointLimitConfiguration(AZ::TypeId jointType) override;
        AZStd::shared_ptr<Physics::Joint> CreateJoint(const AZStd::shared_ptr<Physics::JointLimitConfiguration>& configuration,
            Physics::WorldBody* parentBody, Physics::WorldBody* childBody) override;
        void GenerateJointLimitVisualizationData(
            const Physics::JointLimitConfiguration& configuration,
            const AZ::Quaternion& parentRotation,
            const AZ::Quaternion& childRotation,
            float scale,
            AZ::u32 angularSubdivisions,
            AZ::u32 radialSubdivisions,
            AZStd::vector<AZ::Vector3>& vertexBufferOut,
            AZStd::vector<AZ::u32>& indexBufferOut,
            AZStd::vector<AZ::Vector3>& lineBufferOut,
            AZStd::vector<bool>& lineValidityBufferOut) override;
        AZStd::unique_ptr<Physics::JointLimitConfiguration> ComputeInitialJointLimitConfiguration(
            const AZ::TypeId& jointLimitTypeId,
            const AZ::Quaternion& parentWorldRotation,
            const AZ::Quaternion& childWorldRotation,
            const AZ::Vector3& axis,
            const AZStd::vector<AZ::Quaternion>& exampleLocalRotations) override;

        void ReleaseNativeMeshObject(void* nativeMeshObject) override;

        // Assets related data
        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;
        PhysX::MaterialsManager m_materialManager;

        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

        bool UpdateMaterialSelection(const Physics::ShapeConfiguration& shapeConfiguration,
            Physics::ColliderConfiguration& colliderConfiguration) override;
    private:
        // AZ::TickBus::Handler ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;

        void EnableAutoManagedPhysicsTick(bool shouldTick);

        void ActivatePhysXSystem();
        bool UpdateMaterialSelectionFromPhysicsAsset(
            const Physics::PhysicsAssetShapeConfiguration& assetConfiguration,
            Physics::ColliderConfiguration& colliderConfiguration);

        bool m_enabled; ///< If false, this component will not activate itself in the Activate() function.

        AZStd::unique_ptr<WindProvider> m_windProvider;
        DefaultWorldComponent m_defaultWorldComponent;
        AZ::Interface<Physics::CollisionRequests> m_collisionRequests;
        AZ::Interface<Physics::System> m_physicsSystem;

        PhysXSystem* m_physXSystem = nullptr;
        bool m_isTickingPhysics = false;
        AzPhysics::SystemEvents::OnInitializedEvent::Handler m_onSystemInitializedHandler;
        AzPhysics::SystemEvents::OnConfigurationChangedEvent::Handler m_onSystemConfigChangedHandler;
    };
} // namespace PhysX
