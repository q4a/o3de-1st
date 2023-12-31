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

namespace AZ
{
    AZ_MATH_INLINE Transform::Transform(const Vector3& translation, const Quaternion& rotation, const Vector3& scale)
        : m_translation(translation)
        , m_rotation(rotation)
        , m_scale(scale)
    {
        ;
    }


    AZ_MATH_INLINE Transform Transform::CreateIdentity()
    {
        Transform result;
        result.m_rotation = Quaternion::CreateIdentity();
        result.m_scale = Vector3::CreateOne();
        result.m_translation = Vector3::CreateZero();
        return result;
    }

    AZ_MATH_INLINE Transform Transform::CreateRotationX(float angle)
    {
        return CreateFromQuaternion(Quaternion::CreateRotationX(angle));
    }

    AZ_MATH_INLINE Transform Transform::CreateRotationY(float angle)
    {
        return CreateFromQuaternion(Quaternion::CreateRotationY(angle));
    }

    AZ_MATH_INLINE Transform Transform::CreateRotationZ(float angle)
    {
        return CreateFromQuaternion(Quaternion::CreateRotationZ(angle));
    }

    AZ_MATH_INLINE Transform Transform::CreateFromQuaternion(const Quaternion& q)
    {
        Transform result;
        result.m_rotation = q;
        result.m_scale = Vector3::CreateOne();
        result.m_translation = Vector3::CreateZero();
        return result;
    }

    AZ_MATH_INLINE Transform Transform::CreateFromQuaternionAndTranslation(const class Quaternion& q, const Vector3& p)
    {
        Transform result;
        result.m_rotation = q;
        result.m_scale = Vector3::CreateOne();
        result.m_translation = p;
        return result;
    }

    AZ_MATH_INLINE Transform Transform::CreateScale(const Vector3& scale)
    {
        Transform result;
        result.m_rotation = Quaternion::CreateIdentity();
        result.m_scale = scale;
        result.m_translation = Vector3::CreateZero();
        return result;
    }

    AZ_MATH_INLINE Transform Transform::CreateTranslation(const Vector3& translation)
    {
        Transform result;
        result.m_rotation = Quaternion::CreateIdentity();
        result.m_scale = Vector3::CreateOne();
        result.m_translation = translation;
        return result;
    }

    AZ_MATH_INLINE const Transform& Transform::Identity()
    {
        return g_transformIdentity;
    }

    AZ_MATH_INLINE Vector3 Transform::GetBasis(int32_t index) const
    {
        switch (index)
        {
        case 0:
            return GetBasisX();
        case 1:
            return GetBasisY();
        case 2:
            return GetBasisZ();
        default:
            AZ_MATH_ASSERT(false, "Invalid index for component access.\n");
            return Vector3::CreateZero();
        }
    }

    AZ_MATH_INLINE Vector3 Transform::GetBasisX() const
    {
        return m_rotation.TransformVector(Vector3::CreateAxisX(m_scale.GetX()));
    }

    AZ_MATH_INLINE Vector3 Transform::GetBasisY() const
    {
        return m_rotation.TransformVector(Vector3::CreateAxisY(m_scale.GetY()));
    }

    AZ_MATH_INLINE Vector3 Transform::GetBasisZ() const
    {
        return m_rotation.TransformVector(Vector3::CreateAxisZ(m_scale.GetZ()));
    }

    AZ_MATH_INLINE void Transform::GetBasisAndTranslation(Vector3* basisX, Vector3* basisY, Vector3* basisZ, Vector3* pos) const
    {
        *basisX = GetBasisX();
        *basisY = GetBasisY();
        *basisZ = GetBasisZ();
        *pos = GetTranslation();
    }

    AZ_MATH_INLINE const Vector3& Transform::GetTranslation() const
    {
        return m_translation;
    }

    AZ_MATH_INLINE void Transform::SetTranslation(float x, float y, float z)
    {
        SetTranslation(Vector3(x, y, z));
    }

    AZ_MATH_INLINE void Transform::SetTranslation(const Vector3& v)
    {
        m_translation = v;
    }

    AZ_MATH_INLINE const Quaternion& Transform::GetRotation() const
    {
        return m_rotation;
    }

    AZ_MATH_INLINE void Transform::SetRotation(const Quaternion& rotation)
    {
        m_rotation = rotation;
    }

    AZ_MATH_INLINE const Vector3& Transform::GetScale() const
    {
        return m_scale;
    }

    AZ_MATH_INLINE void Transform::SetScale(const Vector3& scale)
    {
        m_scale = scale;
    }

    AZ_MATH_INLINE Vector3 Transform::ExtractScale()
    {
        const Vector3 scale = m_scale;
        m_scale = Vector3::CreateOne();
        return scale;
    }

    AZ_MATH_INLINE void Transform::MultiplyByScale(const Vector3& scale)
    {
        m_scale *= scale;
    }

    AZ_MATH_INLINE Transform Transform::operator*(const Transform& rhs) const
    {
        Transform result;
        result.m_rotation = m_rotation * rhs.m_rotation;
        result.m_scale = m_scale * rhs.m_scale;
        result.m_translation = TransformPoint(rhs.m_translation);
        return result;
    }

    AZ_MATH_INLINE Transform& Transform::operator*=(const Transform& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    AZ_MATH_INLINE Vector3 Transform::TransformPoint(const Vector3& rhs) const
    {
        return m_rotation.TransformVector((m_scale * rhs)) + m_translation;
    }

    AZ_MATH_INLINE Vector4 Transform::TransformPoint(const Vector4& rhs) const
    {
        return Vector4::CreateFromVector3AndFloat(m_rotation.TransformVector((m_scale * rhs.GetAsVector3())) + m_translation * rhs(3), rhs(3));
    }

    AZ_MATH_INLINE Vector3 Transform::TransformVector(const Vector3& rhs) const
    {
        return m_rotation.TransformVector((m_scale * rhs));
    }

    AZ_MATH_INLINE Transform Transform::GetInverse() const
    {
        // note - need to be careful about how to calculate inverse when there is non-uniform scale
        Transform out;
        out.m_rotation = m_rotation.GetConjugate();
        out.m_scale = m_scale.GetReciprocal();
        out.m_translation = -out.m_scale * (out.m_rotation.TransformVector(m_translation));
        return out;
    }

    AZ_MATH_INLINE void Transform::Invert()
    {
        *this = GetInverse();
    }

    AZ_MATH_INLINE bool Transform::IsOrthogonal(float tolerance) const
    {
        return m_scale.IsClose(Vector3::CreateOne(), tolerance);
    }

    AZ_MATH_INLINE Transform Transform::GetOrthogonalized() const
    {
        Transform result;
        result.m_rotation = m_rotation;
        result.m_scale = Vector3::CreateOne();
        result.m_translation = m_translation;
        return result;
    }

    AZ_MATH_INLINE void Transform::Orthogonalize()
    {
        *this = GetOrthogonalized();
    }

    AZ_MATH_INLINE bool Transform::IsClose(const Transform& rhs, float tolerance) const
    {
        return m_rotation.IsClose(rhs.m_rotation, tolerance)
            && m_scale.IsClose(rhs.m_scale, tolerance)
            && m_translation.IsClose(rhs.m_translation, tolerance);
    }

    AZ_MATH_INLINE bool Transform::operator==(const Transform& rhs) const
    {
        return m_rotation == rhs.m_rotation
            && m_scale == rhs.m_scale
            && m_translation == rhs.m_translation;
    }

    AZ_MATH_INLINE bool Transform::operator!=(const Transform& rhs) const
    {
        return !operator==(rhs);
    }

    AZ_MATH_INLINE Vector3 Transform::GetEulerDegrees() const
    {
        return m_rotation.GetEulerDegrees();
    }

    AZ_MATH_INLINE Vector3 Transform::GetEulerRadians() const
    {
        return m_rotation.GetEulerRadians();
    }

    AZ_MATH_INLINE void Transform::SetFromEulerDegrees(const Vector3& eulerDegrees)
    {
        m_translation = Vector3::CreateZero();
        m_scale = Vector3::CreateOne();
        m_rotation.SetFromEulerDegrees(eulerDegrees);
    }

    AZ_MATH_INLINE void Transform::SetFromEulerRadians(const Vector3& eulerRadians)
    {
        m_translation = Vector3::CreateZero();
        m_scale = Vector3::CreateOne();
        m_rotation.SetFromEulerRadians(eulerRadians);
    }

    AZ_MATH_INLINE bool Transform::IsFinite() const
    {
        return m_rotation.IsFinite()
            && m_scale.IsFinite()
            && m_translation.IsFinite();
    }

    // Non-member functionality belonging to the AZ namespace
    AZ_MATH_INLINE Vector3 ConvertTransformToEulerDegrees(const Transform& transform)
    {
        return transform.GetEulerDegrees();
    }

    AZ_MATH_INLINE Vector3 ConvertTransformToEulerRadians(const Transform& transform)
    {
        return transform.GetEulerRadians();
    }

    AZ_MATH_INLINE Transform ConvertEulerDegreesToTransform(const Vector3& eulerDegrees)
    {
        Transform finalRotation;
        finalRotation.SetFromEulerDegrees(eulerDegrees);
        return finalRotation;
    }

    AZ_MATH_INLINE Transform ConvertEulerRadiansToTransform(const Vector3& eulerRadians)
    {
        Transform finalRotation;
        finalRotation.SetFromEulerRadians(eulerRadians);
        return finalRotation;
    }
}
