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
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>

inline physx::PxVec3 PxMathConvert(const AZ::Vector3& lyVec)
{
    return physx::PxVec3(lyVec.GetX(), lyVec.GetY(), lyVec.GetZ());
}

inline AZ::Vector3 PxMathConvert(const physx::PxVec3& pxVec)
{
    return AZ::Vector3(pxVec.x, pxVec.y, pxVec.z);
}

inline AZ::Vector4 PxMathConvert(const physx::PxVec4& pxVec)
{
    return AZ::Vector4(pxVec.x, pxVec.y, pxVec.z, pxVec.w);
}

inline physx::PxQuat PxMathConvert(const AZ::Quaternion& lyQuat)
{
    return physx::PxQuat(lyQuat.GetX(), lyQuat.GetY(), lyQuat.GetZ(), lyQuat.GetW());
}

inline AZ::Quaternion PxMathConvert(const physx::PxQuat& pxQuat)
{
    return AZ::Quaternion(pxQuat.x, pxQuat.y, pxQuat.z, pxQuat.w);
}

inline AZ::Aabb PxMathConvert(const physx::PxBounds3& bounds)
{
    return AZ::Aabb::CreateFromMinMax(PxMathConvert(bounds.minimum), PxMathConvert(bounds.maximum));
}

inline physx::PxTransform PxMathConvert(const AZ::Transform& lyTransform)
{
    AZ::Quaternion lyQuat = lyTransform.GetRotation();
    AZ::Vector3 lyVec3 = lyTransform.GetTranslation();

    return physx::PxTransform(PxMathConvert(lyVec3), 
        PxMathConvert(lyQuat).getNormalized());
}

inline AZ::Transform PxMathConvert(const physx::PxTransform& pxTransform)
{
    return AZ::Transform::CreateFromQuaternionAndTranslation(PxMathConvert(pxTransform.q), PxMathConvert(pxTransform.p));
}

inline physx::PxTransform PxMathConvert(const AZ::Vector3& position, const AZ::Quaternion& rotation)
{
    return physx::PxTransform(PxMathConvert(position), PxMathConvert(rotation));
}

 /// Conversion for PhysX extended (double precision) vector.
 /// This is used by PhysX for example in character controller position to deal with large co-ordinates.
 /// Note this converts to the lower precision AZ::Vector3.
inline AZ::Vector3 PxMathConvertExtended(const physx::PxExtendedVec3& pxVec)
{
    return AZ::Vector3(static_cast<float>(pxVec.x), static_cast<float>(pxVec.y), static_cast<float>(pxVec.z));
}

/// Conversion for PhysX extended (double precision) vector.
/// This is used by PhysX for example in character controller position to deal with large co-ordinates.
/// Note this converts from the lower precision AZ::Vector3.
inline physx::PxExtendedVec3 PxMathConvertExtended(const AZ::Vector3& lyVec)
{
    return physx::PxExtendedVec3(lyVec.GetX(), lyVec.GetY(), lyVec.GetZ());
}
