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

#include <Atom/RPI.Public/View.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Culling.h>

#include <Atom/RHI/DrawListTagRegistry.h>

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        ViewPtr View::CreateView(const AZ::Name& name, UsageFlags usage)
        {
            View* view = aznew View(name, usage);
            return ViewPtr(view);
        }

        View::View(const AZ::Name& name, UsageFlags usage)
            : m_name(name)
            , m_usageFlags(usage)
        {
            AZ_Assert(!name.IsEmpty(), "invalid name");

            // Set default matrixes. 
            SetWorldToViewMatrix(AZ::Matrix4x4::CreateIdentity());
            AZ::Matrix4x4 viewToClipMatrix;
            AZ::MakePerspectiveFovMatrixRH(viewToClipMatrix, AZ::Constants::HalfPi, 1, 0.1f, 1000.f, true);
            SetViewToClipMatrix(viewToClipMatrix);

            Data::Asset<ShaderResourceGroupAsset> viewSrgAsset = RPISystemInterface::Get()->GetViewSrgAsset();

            if (viewSrgAsset.IsReady())
            {
                m_shaderResourceGroup = ShaderResourceGroup::Create(viewSrgAsset);

                if (m_shaderResourceGroup)
                {
                    m_viewProjectionMatrixConstantIndex    = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_viewProjectionMatrix" });
                    m_worldPositionConstantIndex           = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_worldPosition" });
                    m_viewMatrixConstantIndex              = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_viewMatrix" });
                    m_viewMatrixInverseConstantIndex       = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_viewMatrixInverse" });
                    m_projectionMatrixConstantIndex        = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_projectionMatrix" });
                    m_projectionMatrixInverseConstantIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_projectionMatrixInverse" });
                    m_clipToWorldMatrixConstantIndex       = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_viewProjectionInverseMatrix" });
                    m_worldToClipPrevMatrixConstantIndex   = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_viewProjectionPrevMatrix" });
                    m_zConstantsConstantIndex              = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_nearZ_farZ_farZTimesNearZ_farZMinusNearZ" });
                    m_unprojectionConstantsIndex           = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_unprojectionConstants" });
                }
            }
        }

        void View::SetDrawListMask(const RHI::DrawListMask& drawListMask)
        {
            m_drawListMask = drawListMask;
            m_drawListContext.Shutdown();
            m_drawListContext.Init(m_drawListMask);
        }

        void View::Reset()
        {
            m_drawListMask.reset();
            m_drawListContext.Shutdown();
            m_passesByDrawList = nullptr;
        }

        RHI::ShaderResourceGroup* View::GetRHIShaderResourceGroup() const
        {
            return m_shaderResourceGroup->GetRHIShaderResourceGroup();
        }

        Data::Instance<RPI::ShaderResourceGroup> View::GetShaderResourceGroup()
        {
            return m_shaderResourceGroup;
        }

        void View::AddDrawPacket(const RHI::DrawPacket* drawPacket, float depth)
        {
            // This function is thread safe since DrawListContent has storage per thread for draw item data.
            m_drawListContext.AddDrawPacket(drawPacket, depth);
        }        

        void View::AddDrawPacket(const RHI::DrawPacket* drawPacket, Vector3 worldPosition)
        {
            Vector3 cameraToObject = worldPosition - m_position;
            float depth = cameraToObject.Dot(-m_viewToWorldMatrix.GetBasisZAsVector3());
            AddDrawPacket(drawPacket, depth);
        }

        void View::AddDrawItem(RHI::DrawListTag drawListTag, const RHI::DrawItemKeyPair& drawItemKeyPair)
        {
            m_drawListContext.AddDrawItem(drawListTag, drawItemKeyPair);
        }

        void View::SetWorldToViewMatrix(const AZ::Matrix4x4& worldToView)
        {
            m_viewToWorldMatrix = worldToView.GetInverseFast();
            m_position = m_viewToWorldMatrix.GetTranslation();

            m_worldToViewMatrix = worldToView;
            m_worldToClipMatrix = m_viewToClipMatrix * m_worldToViewMatrix;
            m_worldToClipMatrixChanged = true;

            InvalidateSrg();
        }

        void View::SetCameraTransform(const AZ::Matrix3x4& cameraTransform)
        {
            m_position = cameraTransform.GetTranslation();

            // Before inverting the matrix we must first adjust from Z-up to Y-up. The camera world matrix
            // is in a Z-up world and an identity matrix means that it faces along the positive-Y axis and Z is up.
            // An identity view matrix on the other hand looks along the negative Z-axis.
            // So we adjust for this by rotating the camera world matrix by 90 degrees around the X axis.
            AZ::Matrix3x4 zUpToYUp = AZ::Matrix3x4::CreateRotationX(AZ::Constants::HalfPi);
            AZ::Matrix3x4 yUpWorld = cameraTransform * zUpToYUp;

            float viewToWorldMatrixRaw[16] = {
                        1,0,0,0,
                        0,1,0,0,
                        0,0,1,0,
                        0,0,0,1 };
            yUpWorld.StoreToRowMajorFloat12(viewToWorldMatrixRaw);
            m_viewToWorldMatrix = AZ::Matrix4x4::CreateFromRowMajorFloat16(viewToWorldMatrixRaw);

            m_worldToViewMatrix = m_viewToWorldMatrix.GetInverseFast();

            m_worldToClipMatrix = m_viewToClipMatrix * m_worldToViewMatrix;
            m_clipToWorldMatrix = m_viewToWorldMatrix * m_clipToViewMatrix;
            m_worldToClipMatrixChanged = true;

            InvalidateSrg();
        }        

        void View::SetViewToClipMatrix(const AZ::Matrix4x4& viewToClip)
        {
            m_viewToClipMatrix = viewToClip;
            m_clipToViewMatrix = viewToClip.GetInverseFull();

            m_worldToClipMatrix = m_viewToClipMatrix * m_worldToViewMatrix;
            m_worldToClipMatrixChanged = true;

            m_clipToWorldMatrix = m_viewToWorldMatrix * m_clipToViewMatrix;

            // Update z depth constant simultaneously
            // zNear -> n, zFar -> f
            // A = f / (n - f), B = nf / (n - f) 
            // the formula of A and B should be the same as projection matrix's definition 
            // currently defined in MakePerspectiveFovMatrixRH in MatrixUtil.cpp
            double A = m_viewToClipMatrix.GetElement(2, 2);
            double B = m_viewToClipMatrix.GetElement(2, 3);

            m_nearZ_farZ_farZTimesNearZ_farZMinusNearZ.SetX(float(B / A));
            m_nearZ_farZ_farZTimesNearZ_farZMinusNearZ.SetY(float(B / (A + 1.0)));
            m_nearZ_farZ_farZTimesNearZ_farZMinusNearZ.SetZ(float((B * B) / (A * (A + 1.0))));
            m_nearZ_farZ_farZTimesNearZ_farZMinusNearZ.SetW(float(-B / (A * (A + 1.0))));

            double tanHalfFovX = 1.0 / m_viewToClipMatrix.GetElement(0, 0);
            double tanHalfFovY = 1.0 / m_viewToClipMatrix.GetElement(1, 1);

            m_unprojectionConstants.SetX(float(2.0 * tanHalfFovX));
            m_unprojectionConstants.SetY(float(-2.0 * tanHalfFovY));
            m_unprojectionConstants.SetZ(float(-tanHalfFovX));
            m_unprojectionConstants.SetW(float(tanHalfFovY));

            InvalidateSrg();
        }

        const AZ::Matrix4x4& View::GetWorldToViewMatrix() const
        {
            return m_worldToViewMatrix;
        }

        const AZ::Matrix4x4& View::GetViewToWorldMatrix() const
        {
            return m_viewToWorldMatrix;
        }

        const AZ::Matrix4x4& View::GetViewToClipMatrix() const
        {
            return m_viewToClipMatrix;
        }

        const AZ::Matrix4x4& View::GetWorldToClipMatrix() const
        {
            return m_worldToClipMatrix;
        }

        bool View::HasDrawListTag(RHI::DrawListTag drawListTag)
        {
            return drawListTag.IsValid() && m_drawListMask[drawListTag.GetIndex()];
        }

        RHI::DrawListView View::GetDrawList(RHI::DrawListTag drawListTag)
        {
            return m_drawListContext.GetList(drawListTag);
        }

        void View::FinalizeDrawLists()
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);
            m_drawListContext.FinalizeLists();
            SortFinalizedDrawLists();
        }

        void View::SortFinalizedDrawLists()
        {
            RHI::DrawListsByTag& drawListsByTag = m_drawListContext.GetMergedDrawListsByTag();

            for (size_t idx = 0; idx < drawListsByTag.size(); ++idx)
            {
                if (drawListsByTag[idx].size() > 1)
                {
                    SortDrawList(drawListsByTag[idx], RHI::DrawListTag(idx));
                }
            }
        }

        void View::SortDrawList(RHI::DrawList& drawList, RHI::DrawListTag tag)
        {
            const Pass* passWithDrawListTag = (*m_passesByDrawList)[tag];
            passWithDrawListTag->SortDrawList(drawList);
        }

        // [GFX TODO] This function needs unit tests and might need to be reworked 
        RHI::DrawItemSortKey View::GetSortKeyForPosition(const Vector3& positionInWorld) const
        {
            // We are using fixed-point depth representation for the u64 sort key

            // Compute position in clip space
            const Vector4 worldPosition4 = Vector4::CreateFromVector3(positionInWorld);
            const Vector4 clipSpacePosition = m_worldToClipMatrix * worldPosition4;

            // Get a depth value guaranteed to be in the range 0 to 1
            float normalizedDepth = clipSpacePosition.GetZ() / clipSpacePosition.GetW();
            normalizedDepth = (normalizedDepth + 1.0f) * 0.5f;
            normalizedDepth = AZStd::clamp<float>(normalizedDepth, 0.f, 1.f);

            // Convert the depth into a uint64
            RHI::DrawItemSortKey sortKey = static_cast<RHI::DrawItemSortKey>(normalizedDepth * azlossy_cast<double>(std::numeric_limits<RHI::DrawItemSortKey>::max()));

            return sortKey;
        }

        float View::CalculateSphereAreaInClipSpace(const AZ::Vector3& sphereWorldPosition, float sphereRadius) const
        {
            // Projection of a sphere to clip space 
            // Derived from https://www.iquilezles.org/www/articles/sphereproj/sphereproj.htm

            if (sphereRadius <= 0.0f)
            {
                return 0.0f;
            }

            const AZ::Matrix4x4& worldToViewMatrix = GetWorldToViewMatrix();
            const AZ::Matrix4x4& viewToClipMatrix = GetViewToClipMatrix();

            // transform to camera space (eye space)
            const Vector4 worldPosition4 = Vector4::CreateFromVector3(sphereWorldPosition);
            const Vector4 viewSpacePosition = worldToViewMatrix * worldPosition4;

            float zDist = -viewSpacePosition.GetZ();    // in our view space Z is negative in front of the camera

            if (zDist < 0.0f)
            {
                // sphere center is behind camera.
                if (zDist < -sphereRadius)
                {
                    return 0.0f;    // whole of sphere is behind camera so zero coverage
                }
                else
                {
                    return 1.0f;    // camera is inside sphere so treat as covering whole view
                }
            }
            else
            {
                if (zDist < sphereRadius)
                {
                    return 1.0f;   // camera is inside sphere so treat as covering whole view
                }
            }

            // Element 1,1 of the projection matrix is equal to :  1 / tan(fovY/2) AKA cot(fovY/2)
            // See https://stackoverflow.com/questions/46182845/field-of-view-aspect-ratio-view-matrix-from-projection-matrix-hmd-ost-calib
            float cotHalfFovY = viewToClipMatrix.GetElement(1, 1);

            float radiusSq = sphereRadius * sphereRadius;
            float depthSq = zDist * zDist;
            float distanceSq = viewSpacePosition.GetAsVector3().GetLengthSq();
            float cotHalfFovYSq = cotHalfFovY * cotHalfFovY;

            float radiusSqSubDepthSq = radiusSq - depthSq;

            const float epsilon = 0.00001f;
            if (fabsf(radiusSqSubDepthSq) < epsilon)
            {
                // treat as covering entire view since we don't want to divide by zero
                return 1.0f;
            }

            // This will return 1.0f when an area equal in size to the viewport height squared is covered.
            // So to get actual pixels covered do : coverage * viewport-resolution-y * viewport-resolution-y
            // The actual math computes the area of an ellipse as a percentage of the view area, see the paper above for the steps
            // to simplify the equations into this calculation.
            return  -0.25f * cotHalfFovYSq * AZ::Constants::Pi * radiusSq * sqrt(fabsf((distanceSq - radiusSq)/radiusSqSubDepthSq))/radiusSqSubDepthSq;
        }

        void View::InvalidateSrg()
        {
            m_needBuildSrg = true;
        }

        void View::UpdateSrg()
        {
            if (m_worldToClipPrevMatrixNeedsUpdate)
            {
                m_shaderResourceGroup->SetConstant(m_worldToClipPrevMatrixConstantIndex, m_worldToClipPrevMatrix);
                m_worldToClipPrevMatrixNeedsUpdate = false;
            }

            if (m_worldToClipMatrixChanged)
            {
                m_worldToClipPrevMatrix = m_worldToClipMatrix;
                m_worldToClipPrevMatrixNeedsUpdate = true;
                m_worldToClipMatrixChanged = false;
            }

            if (!m_needBuildSrg)
            {
                return;
            }

            if (m_worldPositionConstantIndex.IsValid())
            {
                m_shaderResourceGroup->SetConstant(m_worldPositionConstantIndex, m_position);
            }
            if (m_viewProjectionMatrixConstantIndex.IsValid())
            {
                m_shaderResourceGroup->SetConstant(m_viewProjectionMatrixConstantIndex, m_worldToClipMatrix);
            }
            if (m_viewMatrixConstantIndex.IsValid())
            {
                m_shaderResourceGroup->SetConstant(m_viewMatrixConstantIndex, m_worldToViewMatrix);
            }
            if (m_viewMatrixInverseConstantIndex.IsValid())
            {
                m_shaderResourceGroup->SetConstant(m_viewMatrixInverseConstantIndex, m_worldToViewMatrix.GetInverseFull());
            }
            if (m_projectionMatrixConstantIndex.IsValid())
            {
                m_shaderResourceGroup->SetConstant(m_projectionMatrixConstantIndex, m_viewToClipMatrix);
            }
            if (m_projectionMatrixInverseConstantIndex.IsValid())
            {
                m_shaderResourceGroup->SetConstant(m_projectionMatrixInverseConstantIndex, m_viewToClipMatrix.GetInverseFull());
            }
            if (m_zConstantsConstantIndex.IsValid())
            {
                m_shaderResourceGroup->SetConstant(m_zConstantsConstantIndex, m_nearZ_farZ_farZTimesNearZ_farZMinusNearZ);
            }
            if (m_clipToWorldMatrixConstantIndex.IsValid())
            {
                m_shaderResourceGroup->SetConstant(m_clipToWorldMatrixConstantIndex, m_clipToWorldMatrix);
            }
            if (m_unprojectionConstantsIndex.IsValid())
            {
                m_shaderResourceGroup->SetConstant(m_unprojectionConstantsIndex, m_unprojectionConstants);
            }

            m_shaderResourceGroup->Compile();
            m_needBuildSrg = false;
        }
    } // namespace RPI
} // namespace AZ
