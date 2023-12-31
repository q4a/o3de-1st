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
#include <Launcher.h>

#include <CryCommon/CryLibrary.h>

int APIENTRY WinMain([[maybe_unused]] HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance, [[maybe_unused]] LPSTR lpCmdLine, [[maybe_unused]] int nCmdShow)
{
    InitRootDir();

    using namespace LumberyardLauncher;

    PlatformMainInfo mainInfo;

    mainInfo.m_instance = GetModuleHandle(0);

    mainInfo.CopyCommandLine(__argc, __argv);

    // Prevent allocator from growing in small chunks
    // Pre-create our system allocator and configure it to ask for larger chunks from the OS
    // Creating this here to be consistent with other platforms
    AZ::SystemAllocator::Descriptor sysHeapDesc;
    sysHeapDesc.m_heap.m_systemChunkSize = 64 * 1024 * 1024;
    AZ::AllocatorInstance<AZ::SystemAllocator>::Create(sysHeapDesc);

    ReturnCode status = Run(mainInfo);

#if !defined(_RELEASE)
    bool noPrompt = (strstr(mainInfo.m_commandLine, "-noprompt") != nullptr);
#else
    bool noPrompt = false;
#endif // !defined(_RELEASE)

    if (!noPrompt && status != ReturnCode::Success)
    {
        MessageBoxA(0, GetReturnCodeString(status), "Error", MB_OK | MB_DEFAULT_DESKTOP_ONLY | MB_ICONERROR);
    }

#if !defined(AZ_MONOLITHIC_BUILD)

    {
        // HACK HACK HACK - is this still needed?!?!
        // CrySystem module can get loaded multiple times (even from within CrySystem itself)
        // and currently there is no way to track them (\ref _CryMemoryManagerPoolHelper::Init() in CryMemoryManager_impl.h)
        // so we will release it as many times as it takes until it actually unloads.
        void* hModule = CryLoadLibraryDefName("CrySystem");
        if (hModule)
        {
            // loop until we fail (aka unload the DLL)
            while (CryFreeLibrary(hModule))
            {
                ;
            }
        }
    }
#endif // !defined(AZ_MONOLITHIC_BUILD)

    // there is no way to transfer ownership of the allocator to the component application
    // without altering the app descriptor, so it must be destroyed here
    AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();

    return static_cast<int>(status);
}
