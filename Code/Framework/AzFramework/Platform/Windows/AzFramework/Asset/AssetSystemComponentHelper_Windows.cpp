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

#include <AzCore/PlatformIncl.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include <Psapi.h>

namespace AzFramework::AssetSystem::Platform
{
    void AllowAssetProcessorToForeground()
    {
        // Make sure that all of the asset processors can bring their window to the front
        // Hacky to put it here, but not really any better place.
        DWORD bytesReturned;

        // There's no straightforward way to get the exact number of running processes,
        // So we use 2^13 processes as a generous upper bound that shouldn't be hit.
        DWORD processIds[8 * 1024];

        if (EnumProcesses(processIds, sizeof(processIds), &bytesReturned))
        {
            const DWORD numProcesses = bytesReturned / sizeof(DWORD);
            for (DWORD processIndex = 0; processIndex < numProcesses; ++processIndex)
            {
                DWORD processId = processIds[processIndex];
                HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION |
                    PROCESS_VM_READ,
                    FALSE, processId);

                // Get the process name.
                if (processHandle)
                {
                    HMODULE moduleHandle;
                    DWORD bytesNeededForAllProcessModules;

                    // Get the first module, because that will be the executable
                    if (EnumProcessModules(processHandle, &moduleHandle, sizeof(moduleHandle), &bytesNeededForAllProcessModules))
                    {
                        char processName[4096] = TEXT("<unknown>");
                        if (GetModuleBaseNameA(processHandle, moduleHandle, processName, AZ_ARRAY_SIZE(processName)) > 0)
                        {
                            if (azstricmp(processName, "AssetProcessor") == 0)
                            {
                                AllowSetForegroundWindow(processId);
                            }
                        }
                    }
                }
            }
        }
    }

    bool LaunchAssetProcessor(AZStd::string_view executableDirectory, AZStd::string_view appRoot,
        AZStd::string_view gameProjectName)
    {
        AZ::IO::FixedMaxPath assetProcessorPath{ executableDirectory };
        assetProcessorPath /= "AssetProcessor.exe";

        auto fullLaunchCommand = AZ::IO::FixedMaxPathString::format(R"("%s" --start-hidden)", assetProcessorPath.c_str());
        // Add the app-root to the launch command if not empty
        if (!appRoot.empty())
        {
            fullLaunchCommand += R"( --app-root=")";
            fullLaunchCommand += appRoot;
            // Windows CreateProcess has issues with paths that end with a trailing backslash
            // so remove it if it exist
            if (fullLaunchCommand.ends_with(AZ::IO::WindowsPathSeparator))
            {
                fullLaunchCommand.pop_back();
            }
            fullLaunchCommand += '"';
        }

        // Add the active game project to the launch command if not empty 
        if (!gameProjectName.empty())
        {
            fullLaunchCommand += R"( --gameFolder=")";
            fullLaunchCommand += gameProjectName;
            fullLaunchCommand += '"';
        }

        STARTUPINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_MINIMIZE;
        PROCESS_INFORMATION pi;

        return ::CreateProcessA(nullptr, fullLaunchCommand.data(), nullptr, nullptr, FALSE, 0, nullptr, AZ::IO::FixedMaxPathString{ executableDirectory }.c_str(), &si, &pi) != 0;
    }
}
