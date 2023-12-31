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

#include <AzQtComponents/Utilities/HandleDpiAwareness.h>

#include <AzCore/PlatformIncl.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/fixed_vector.h>

#include <AzFramework/API/AtomActiveInterface.h>
#include <AzCore/Interface/Interface.h>

namespace AzQtComponents
{
    namespace Utilities
    {
        namespace Platform
        {
            void HandleDpiAwareness(DpiAwareness dpiAwareness)
            {
                // We need to force the dpi awareness settings on Windows based on the version.
                // Qt doesn't currently expose a mechanism to do that other than via command line arguments

                char qpaArg[512] = { "QT_QPA_PLATFORM=windows:fontengine=freetype,"};

                switch (dpiAwareness)
                {
                    case Unaware:
                        azstrcat(qpaArg, AZ_ARRAY_SIZE(qpaArg), "dpiawareness=0");
                        break;
                    case SystemDpiAware:
                        azstrcat(qpaArg, AZ_ARRAY_SIZE(qpaArg), "dpiawareness=1");
                        break;
                    case PerScreenDpiAware:     // intentional fall-through (setting defaults to 2)
                    case Unset:                 // intentional fall-through
                    default:
                        azstrcat(qpaArg, AZ_ARRAY_SIZE(qpaArg), "dpiawareness=2");
                        break;
                }

                _putenv(qpaArg);
            }
        } // namespace Platform
    } // namespace Utilities
} // namespace AzQtComponents
