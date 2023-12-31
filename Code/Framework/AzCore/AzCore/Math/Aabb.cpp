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

#include <AzCore/Math/Aabb.h>

#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/MathScriptHelpers.h>

namespace AZ
{
    //Script Wrappers for Axis Aligned Bounding Box
    void AabbDefaultConstructor(AZ::Aabb* thisPtr)
    {
        new (thisPtr) AZ::Aabb(AZ::Aabb::CreateNull());
    }


    void AabbSetGeneric(Aabb* thisPtr, ScriptDataContext& dc)
    {
        if (dc.GetNumArguments() == 2 && dc.IsClass<Vector3>(0) && dc.IsClass<Vector3>(1))
        {
            Vector3 min, max;
            dc.ReadArg<Vector3>(0, min);
            dc.ReadArg<Vector3>(1, max);
            thisPtr->Set(min, max);
        }
        else
        {
            AZ_Error("Script", false, "ScriptAabb Set only supports two arguments: Vector3 min, Vector3 max");
        }
    }


    void AabbGetAsSphereMultipleReturn(const Aabb* thisPtr, ScriptDataContext& dc)
    {
        Vector3 center;
        float radius;
        thisPtr->GetAsSphere(center, radius);
        dc.PushResult(center);
        dc.PushResult(radius);
    }


    void AabbContainsGeneric(const Aabb* thisPtr, ScriptDataContext& dc)
    {
        if (dc.GetNumArguments() == 1)
        {
            if (dc.IsClass<Vector3>(0))
            {
                Vector3 v = Vector3::CreateZero();
                dc.ReadArg<Vector3>(0, v);
                dc.PushResult<bool>(thisPtr->Contains(v));
            }
            else if (dc.IsClass<Aabb>(0))
            {
                Aabb aabb = Aabb::CreateNull();
                dc.ReadArg<Aabb>(0, aabb);
                dc.PushResult<bool>(thisPtr->Contains(aabb));
            }
        }

        if (dc.GetNumResults() == 0)
        {
            AZ_Error("Script", false, "ScriptAabb Contains expects one argument: Either Vector3 or Aabb");
        }
    }


    AZStd::string AabbToString(const Aabb& aabb)
    {
        return AZStd::string::format("Min %s Max %s", Vector3ToString(aabb.GetMin()).c_str(), Vector3ToString(aabb.GetMax()).c_str());
    }


    void Aabb::Reflect(ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Aabb>()
                ->Field("min", &Aabb::m_min)
                ->Field("max", &Aabb::m_max);
        }

        auto behaviorContext = azrtti_cast<BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<Aabb>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "math")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Attribute(AZ::Script::Attributes::GenericConstructorOverride, &AabbDefaultConstructor)
                ->Property("min", &Aabb::GetMin, &Aabb::SetMin)
                ->Property("max", &Aabb::GetMax, &Aabb::SetMax)
                ->Method("ToString", &AabbToString)
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ->Method("CreateNull", &Aabb::CreateNull)
                ->Method("IsValid", &Aabb::IsValid)
                ->Method("CreateFromPoint", &Aabb::CreateFromPoint)
                ->Method("CreateFromMinMax", &Aabb::CreateFromMinMax)
                ->Method("CreateFromMinMaxValues", &Aabb::CreateFromMinMaxValues)
                ->Method("CreateCenterHalfExtents", &Aabb::CreateCenterHalfExtents)
                ->Method("CreateCenterRadius", &Aabb::CreateCenterRadius)
                ->Method("GetExtents", &Aabb::GetExtents)
                ->Method("GetCenter", &Aabb::GetCenter)
                ->Method("Set", &Aabb::Set)
                ->Attribute(AZ::Script::Attributes::MethodOverride, &AabbSetGeneric)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("CreateFromObb", &Aabb::CreateFromObb)
                ->Method("GetXExtent", &Aabb::GetXExtent)
                ->Method("GetYExtent", &Aabb::GetYExtent)
                ->Method("GetZExtent", &Aabb::GetZExtent)
                ->Method("GetAsSphere", &Aabb::GetAsSphere, nullptr, "() -> Vector3(center) and float(radius)")
                ->Attribute(AZ::Script::Attributes::MethodOverride, &AabbGetAsSphereMultipleReturn)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method<bool (Aabb::*)(const Aabb&) const>("Contains", &Aabb::Contains, nullptr, "const Vector3& or const Aabb&")
                ->Attribute(AZ::Script::Attributes::MethodOverride, &AabbContainsGeneric)
                ->Method<bool (Aabb::*)(const Vector3&) const>("ContainsVector3", &Aabb::Contains, nullptr, "const Vector3&")
                ->Attribute(AZ::Script::Attributes::Ignore, 0) // ignore for script since we already got the generic contains above
                ->Method("Overlaps", &Aabb::Overlaps)
                ->Method("Expand", &Aabb::Expand)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("GetExpanded", &Aabb::GetExpanded)
                ->Method("AddPoint", &Aabb::AddPoint)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("AddAabb", &Aabb::AddAabb)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("GetDistance", &Aabb::GetDistance)
                ->Method("GetClamped", &Aabb::GetClamped)
                ->Method("Clamp", &Aabb::Clamp)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("SetNull", &Aabb::SetNull)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("Translate", &Aabb::Translate)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("GetTranslated", &Aabb::GetTranslated)
                ->Method("GetSurfaceArea", &Aabb::GetSurfaceArea)
                ->Method("GetTransformedObb", &Aabb::GetTransformedObb)
                ->Method("GetTransformedAabb", &Aabb::GetTransformedAabb)
                ->Method("ApplyTransform", &Aabb::ApplyTransform)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("Clone", [](const Aabb& rhs) -> Aabb { return rhs; })
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("IsFinite", &Aabb::IsFinite)
                ->Method("Equal", &Aabb::operator==)
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All);
        }
    }


    Aabb Aabb::CreateFromObb(const Obb& obb)
    {
        Vector3 tmp[8];
        tmp[0] = obb.GetPosition() + obb.GetAxisX() * obb.GetHalfLengthX()
            + obb.GetAxisY() * obb.GetHalfLengthY()
            + obb.GetAxisZ() * obb.GetHalfLengthZ();
        tmp[1] = tmp[0] - obb.GetAxisZ() * (2.0f * obb.GetHalfLengthZ());
        tmp[2] = tmp[0] - obb.GetAxisX() * (2.0f * obb.GetHalfLengthX());
        tmp[3] = tmp[1] - obb.GetAxisX() * (2.0f * obb.GetHalfLengthX());
        tmp[4] = tmp[0] - obb.GetAxisY() * (2.0f * obb.GetHalfLengthY());
        tmp[5] = tmp[1] - obb.GetAxisY() * (2.0f * obb.GetHalfLengthY());
        tmp[6] = tmp[2] - obb.GetAxisY() * (2.0f * obb.GetHalfLengthY());
        tmp[7] = tmp[3] - obb.GetAxisY() * (2.0f * obb.GetHalfLengthY());

        Vector3 min = tmp[0];
        Vector3 max = tmp[0];

        for (int i = 1; i < 8; ++i)
        {
            min = min.GetMin(tmp[i]);
            max = max.GetMax(tmp[i]);
        }

        return Aabb::CreateFromMinMax(min, max);
    }


    Obb Aabb::GetTransformedObb(const Transform& transform) const
    {
        /// \todo This can be optimized, by transforming directly from the Aabb without converting to a Obb first
        Obb temp = Obb::CreateFromAabb(*this);
        return transform * temp;
    }


    void Aabb::ApplyTransform(const Transform& transform)
    {
        Vector3 a, b, axisCoeffs;

        Vector3 newMin, newMax;
        newMin = newMax = transform.GetTranslation();

        // Find extreme points for each axis.
        for (int j = 0; j < 3; j++)
        {
            Vector3 axis = Vector3::CreateZero();
            axis.SetElement(j, 1.0f);
            // The extreme values in each direction must be attained by transforming one of the 8 vertices of the original box.
            // Each co-ordinate of a transformed vertex is made up of three parts, corresponding to the components of the original
            // x, y and z co-ordinates which are mapped onto the new axis. Those three parts are independent, so we can take
            // the min and max of each part and sum them to get the min and max co-ordinate of the transformed box. For a given new axis,
            // the coefficients for what proportion of each original axis is rotated onto that new axis are the same as the components we
            // would get by performing the inverse rotation on the new axis, so we need to take the conjugate to get the inverse rotation.
            axisCoeffs = transform.GetScale() * (transform.GetRotation().GetConjugate().TransformVector(axis));
            a = axisCoeffs * m_min;
            b = axisCoeffs * m_max;

            newMin.SetElement(j, newMin(j) + a.GetMin(b).Dot(Vector3::CreateOne()));
            newMax.SetElement(j, newMax(j) + a.GetMax(b).Dot(Vector3::CreateOne()));
        }

        m_min = newMin;
        m_max = newMax;
    }
}
