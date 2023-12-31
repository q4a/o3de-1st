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

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <BuilderSettings/ImageProcessingDefines.h>
#include <Atom/ImageProcessing/PixelFormats.h>

namespace ImageProcessingAtom
{
    //! default settings for platform
    struct PlatformSetting
    {
        AZ_TYPE_INFO(PlatformSetting, "{95FBE763-C5CD-4C40-964F-9D34E3AB2138}");
        AZ_CLASS_ALLOCATOR(PlatformSetting, AZ::SystemAllocator, 0);

        //! Platform's name
        PlatformName m_name;

        //! pixel formats supported for the platform
        AZStd::list<EPixelFormat> m_availableFormat;
    };
} // namespace ImageProcessingAtom
