/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Asset/GenericAssetHandler.h>

namespace AZ
{
    class Vector3;
}

namespace Physics
{
    class World;
    class WorldBody;
    class RigidBody;
    class RigidBodyStatic;
    class Shape;
    class Material;
    class MaterialSelection;
    class MaterialConfiguration;
    class MaterialLibraryAsset;
    class WorldBodyConfiguration;
    class RigidBodyConfiguration;
    class ColliderConfiguration;
    class ShapeConfiguration;
    class JointLimitConfiguration;
    class Joint;
    struct RayCastRequest;
    struct RayCastResult;
    struct ShapeCastRequest;
    struct ShapeCastResult;
    class CharacterConfiguration;
    class Character;

    /// Represents a debug vertex (position & color).
    struct DebugDrawVertex
    {
        AZ::Vector3     m_position;
        AZ::Color       m_color;

        DebugDrawVertex(const AZ::Vector3& v, const AZ::Color& c)
            : m_position(v)
            , m_color(c)
        {}

        static AZ::Color GetGhostColor()        { return AZ::Color(1.0f, 0.7f, 0.0f, 1.0f); }
        static AZ::Color GetRigidBodyColor()    { return AZ::Color(0.0f, 1.0f, 0.0f, 1.0f); }
        static AZ::Color GetSleepingBodyColor() { return AZ::Color(0.5f, 0.5f, 0.5f, 1.0f); }
        static AZ::Color GetCharacterColor()    { return AZ::Color(0.0f, 1.0f, 1.0f, 1.0f); }
        static AZ::Color GetRayColor()          { return AZ::Color(0.8f, 0.4f, 0.2f, 1.0f); }
        static AZ::Color GetRed()               { return AZ::Color(1.0f, 0.0f, 0.0f, 1.0f); }
        static AZ::Color GetGreen()             { return AZ::Color(0.0f, 1.0f, 0.0f, 1.0f); }
        static AZ::Color GetBlue()              { return AZ::Color(0.0f, 0.0f, 1.0f, 1.0f); }
        static AZ::Color GetWhite()             { return AZ::Color(1.0f, 1.0f, 1.0f, 1.0f); }
    };

    /// Settings structure provided to DebugDrawPhysics to drive debug drawing behavior.
    struct DebugDrawSettings
    {
        using DebugDrawLineCallback = AZStd::function<void(const DebugDrawVertex& from, const DebugDrawVertex& to, const AZStd::shared_ptr<WorldBody>& body, float thickness, void* udata)>;
        using DebugDrawTriangleCallback = AZStd::function<void(const DebugDrawVertex& a, const DebugDrawVertex& b, const DebugDrawVertex& c, const AZStd::shared_ptr<WorldBody>& body, void* udata)>;
        using DebugDrawTriangleBatchCallback = AZStd::function<void(const DebugDrawVertex* verts, AZ::u32 numVerts, const AZ::u32* indices, AZ::u32 numIndices, const AZStd::shared_ptr<WorldBody>& body, void* udata)>;

        DebugDrawLineCallback           m_drawLineCB;                               ///< Required user callback for line drawing.
        DebugDrawTriangleBatchCallback  m_drawTriBatchCB;                           ///< User callback for triangle batch drawing. Required if \ref m_isWireframe is false.
        bool                            m_isWireframe = false;                      ///< Specifies whether or not physics shapes should be draw as wireframe (lines only) or solid triangles.
        AZ::u32                         m_objectLayers = static_cast<AZ::u32>(~0);  ///< Mask specifying which \ref AzFramework::Physics::StandardObjectLayers should be drawn.
        AZ::Vector3                     m_cameraPos = AZ::Vector3::CreateZero();    ///< Camera position, for limiting objects based on \ref m_drawDistance.
        float                           m_drawDistance = 500.f;                     ///< Distance from \ref m_cameraPos within which objects will be drawn.
        bool                            m_drawBodyTransforms = false;               ///< If enabled, draws transform axes for each body.
        void*                           m_udata = nullptr;                          ///< Platform specific and/or gem specific optional user data pointer.

        void DrawLine(const DebugDrawVertex& from, const DebugDrawVertex& to, const AZStd::shared_ptr<WorldBody>& body, float thickness = 1.0f) { m_drawLineCB(from, to, body, thickness, m_udata); }
        void DrawTriangleBatch(const DebugDrawVertex* verts, AZ::u32 numVerts, const AZ::u32* indices, AZ::u32 numIndices, const AZStd::shared_ptr<WorldBody>& body) { m_drawTriBatchCB(verts, numVerts, indices, numIndices, body, m_udata); }
    };

    /// An interface to get the default physics world for systems that do not support multiple worlds.
    class DefaultWorldRequests
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::mutex;

        /// Returns the Default world managed by a relevant system.
        virtual AZStd::shared_ptr<World> GetDefaultWorld() = 0;
    };

    typedef AZ::EBus<DefaultWorldRequests> DefaultWorldBus;

    /// An interface to get the editor physics world for doing edit time physics queries
    class EditorWorldRequests
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::mutex;

        /// Returns the Editor world managed editor system component.
        virtual AZStd::shared_ptr<World> GetEditorWorld() = 0;
    };
    using EditorWorldBus = AZ::EBus<EditorWorldRequests>;

    class SystemRequestsTraits 
        : public AZ::EBusTraits 
    {
    public:
        using MutexType = AZStd::recursive_mutex;

        // EBusTraits
        // singleton pattern
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };

    /// Physics system global requests.
    class System
    {
    public:
        // Required for AZ::Interface
        AZ_TYPE_INFO(System, "{35965894-BFBC-437C-A4FB-E22F3DB09ACF}")

        System() = default;
        virtual ~System() = default;

        // AZ::Interface requires these to be deleted.
        System(System&&) = delete;
        System& operator=(System&&) = delete;

        //////////////////////////////////////////////////////////////////////////
        //// General Physics

        virtual AZStd::unique_ptr<RigidBodyStatic> CreateStaticRigidBody(const WorldBodyConfiguration& configuration) = 0;
        virtual AZStd::unique_ptr<RigidBody> CreateRigidBody(const RigidBodyConfiguration& configuration) = 0;
        virtual AZStd::shared_ptr<Shape> CreateShape(const ColliderConfiguration& colliderConfiguration, const ShapeConfiguration& configuration) = 0;

        /// Adds an appropriate collider component to the entity based on the provided shape configuration.
        /// @param entity Entity where the component should be added to.
        /// @param colliderConfiguration Configuration of the collider.
        /// @param shapeConfiguration Configuration of the shape of the collider.
        /// @param addEditorComponents Tells whether to add the Editor version of the collider component or the Game one.
        virtual void AddColliderComponentToEntity(AZ::Entity* entity, const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& shapeConfiguration, bool addEditorComponents = false) = 0;

        /// Releases the mesh object created by the physics backend.
        /// @param nativeMeshObject Pointer to the mesh object.
        virtual void ReleaseNativeMeshObject(void* nativeMeshObject) = 0;

        //////////////////////////////////////////////////////////////////////////
        //// Physics Materials

        virtual AZStd::shared_ptr<Material> CreateMaterial(const Physics::MaterialConfiguration& materialConfiguration) = 0;
        virtual AZStd::shared_ptr<Material> GetDefaultMaterial() = 0;
        virtual AZStd::vector<AZStd::shared_ptr<Material>> CreateMaterialsFromLibrary(const Physics::MaterialSelection& materialSelection) = 0;


        /// Updates the collider material selection from the physics asset or sets it to default if there's no asset provided.
        /// @param shapeConfiguration The shape information
        /// @param colliderConfiguration The collider information
        virtual bool UpdateMaterialSelection(const Physics::ShapeConfiguration& shapeConfiguration,
            Physics::ColliderConfiguration& colliderConfiguration) = 0;

        //////////////////////////////////////////////////////////////////////////
        //// Joints

        virtual AZStd::vector<AZ::TypeId> GetSupportedJointTypes() = 0;
        virtual AZStd::shared_ptr<JointLimitConfiguration> CreateJointLimitConfiguration(AZ::TypeId jointType) = 0;
        virtual AZStd::shared_ptr<Joint> CreateJoint(const AZStd::shared_ptr<JointLimitConfiguration>& configuration,
            Physics::WorldBody* parentBody, Physics::WorldBody* childBody) = 0;
        /// Generates joint limit visualization data in appropriate format to pass to DebugDisplayRequests draw functions.
        /// @param configuration The joint configuration to generate visualization data for.
        /// @param parentRotation The rotation of the joint's parent body (in the same frame as childRotation).
        /// @param childRotation The rotation of the joint's child body (in the same frame as parentRotation).
        /// @param scale Scale factor for the output display data.
        /// @param angularSubdivisions Level of detail in the angular direction (may be clamped in the implementation).
        /// @param radialSubdivisions Level of detail in the radial direction (may be clamped in the implementation).
        /// @param[out] vertexBufferOut Used with indexBufferOut to define triangles to be displayed.
        /// @param[out] indexBufferOut Used with vertexBufferOut to define triangles to be displayed.
        /// @param[out] lineBufferOut Used to define lines to be displayed.
        /// @param[out] lineValidityBufferOut Whether each line in the line buffer is part of a valid or violated limit.
        virtual void GenerateJointLimitVisualizationData(
            const JointLimitConfiguration& configuration,
            const AZ::Quaternion& parentRotation,
            const AZ::Quaternion& childRotation,
            float scale,
            AZ::u32 angularSubdivisions,
            AZ::u32 radialSubdivisions,
            AZStd::vector<AZ::Vector3>& vertexBufferOut,
            AZStd::vector<AZ::u32>& indexBufferOut,
            AZStd::vector<AZ::Vector3>& lineBufferOut,
            AZStd::vector<bool>& lineValidityBufferOut) = 0;

        /// Computes parameters such as joint limit local rotations to give the desired initial joint limit orientation.
        /// @param jointLimitTypeId The type ID used to identify the particular kind of joint limit configuration to be created.
        /// @param parentWorldRotation The rotation in world space of the parent world body associated with the joint.
        /// @param childWorldRotation The rotation in world space of the child world body associated with the joint.
        /// @param axis Axis used to define the centre for limiting angular degrees of freedom.
        /// @param exampleLocalRotations A vector (which may be empty) containing example valid rotations in the local space
        /// of the child world body relative to the parent world body, which may optionally be used to help estimate the extents
        /// of the joint limit.
        virtual AZStd::unique_ptr<JointLimitConfiguration> ComputeInitialJointLimitConfiguration(
            const AZ::TypeId& jointLimitTypeId,
            const AZ::Quaternion& parentWorldRotation,
            const AZ::Quaternion& childWorldRotation,
            const AZ::Vector3& axis,
            const AZStd::vector<AZ::Quaternion>& exampleLocalRotations) = 0;

        //////////////////////////////////////////////////////////////////////////
        //// Cooking

        /// Cooks a convex mesh and writes it to a file.
        /// @param filePath Path to the output file.
        /// @param vertices Pointer to beginning of vertex data.
        /// @param vertexCount Number of vertices in the mesh.
        /// @return Succeeded cooking.
        virtual bool CookConvexMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount) = 0;

        /// Cooks a convex mesh to a memory buffer.
        /// @param vertices Pointer to beginning of vertex data.
        /// @param vertexCount Number of vertices in the mesh.
        /// @param result The resulting memory buffer.
        /// @return Succeeded cooking.
        virtual bool CookConvexMeshToMemory(const AZ::Vector3* vertices, AZ::u32 vertexCount, AZStd::vector<AZ::u8>& result) = 0;

        /// Cooks a triangular mesh and writes it to a file.
        /// @param filePath Path to the output file.
        /// @param vertices Pointer to beginning of vertex data.
        /// @param vertexCount Number of vertices in the mesh.
        /// @param indices Pointer to beginning of index data.
        /// @param indexCount Number of indices in the mesh.
        /// @return Succeeded cooking.
        virtual bool CookTriangleMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount,
            const AZ::u32* indices, AZ::u32 indexCount) = 0;

        /// Cook a triangular mesh to a memory buffer.
        /// @param vertices Pointer to beginning of vertex data.
        /// @param vertexCount Number of vertices in the mesh.
        /// @param indices Pointer to beginning of index data.
        /// @param indexCount Number of indices in the mesh.
        /// @param result The resulting memory buffer.
        /// @return Succeeded cooking.
        virtual bool CookTriangleMeshToMemory(const AZ::Vector3* vertices, AZ::u32 vertexCount,
            const AZ::u32* indices, AZ::u32 indexCount, AZStd::vector<AZ::u8>& result) = 0;
    };

    using SystemRequests = System;
    using SystemRequestBus = AZ::EBus<SystemRequests, SystemRequestsTraits>;

    /// Physics character system global requests.
    class CharacterSystemRequests
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits
        // singleton pattern
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual ~CharacterSystemRequests() = default;

        /// Creates the physics representation used to handle basic character interactions (also known as a character
        /// controller).
        virtual AZStd::unique_ptr<Character> CreateCharacter(const CharacterConfiguration& characterConfig,
            const ShapeConfiguration& shapeConfig, World& world) = 0;

        /// Performs any updates related to character controllers which are per-world and not per-character, such as
        /// computing character-character interactions.
        virtual void UpdateCharacters(World& world, float deltaTime) = 0;
    };

    typedef AZ::EBus<CharacterSystemRequests> CharacterSystemRequestBus;

    /// Physics system global debug requests.
    class SystemDebugRequests
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits
        // singleton pattern
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        /// Draw physics system state.
        /// \param settings see \ref DebugDrawSettings.
        virtual void DebugDrawPhysics(const DebugDrawSettings& settings) { (void)settings; }

        /// Exports an entity's physics body(ies) to the specified filename, if supported by the physics backend.
        virtual void ExportEntityPhysics(const AZStd::vector<AZ::EntityId>& ids, const AZStd::string& filename) { (void)ids; (void)filename; }
    };

    typedef AZ::EBus<SystemDebugRequests> SystemDebugRequestBus;

    class SystemNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~SystemNotifications() {}

        virtual void OnWorldCreated(World* /*world*/) {};
        virtual void OnPreWorldDestroy(World* /*world*/) {};
    };

    using SystemNotificationBus = AZ::EBus<SystemNotifications>;

} // namespace Physics
