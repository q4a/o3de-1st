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
#include <dlfcn.h>
#include <iostream>
#include <AzTest/Platform.h>

class ModuleHandle
    : public AZ::Test::IModuleHandle
{
public:
    explicit ModuleHandle(const std::string& lib)
        : m_libHandle(nullptr)
    {
        std::string libext = lib;
        if (!AZ::Test::EndsWith(libext, ".so"))
        {
            libext += ".so";
        }
        m_libHandle = dlopen(libext.c_str(), RTLD_NOW);
        const char* error = dlerror();
        if (error)
        {
            std::cerr << "Fail to open shared library: " << libext << std::endl
                      << "Error from dlopen(): " << error << std::endl;
        }
    }

    ModuleHandle(const ModuleHandle&) = delete;
    ModuleHandle& operator=(const ModuleHandle&) = delete;

    ~ModuleHandle() override
    {
        if (m_libHandle)
        {
            dlclose(m_libHandle);
        }
    }

    bool IsValid() override { return m_libHandle != nullptr; }

    std::shared_ptr<AZ::Test::IFunctionHandle> GetFunction(const std::string& name) override;

private:
    friend class FunctionHandle;

    void* m_libHandle;
};

class FunctionHandle
    : public AZ::Test::IFunctionHandle
{
public:
    explicit FunctionHandle(ModuleHandle& module, const std::string& symbol)
        : m_fn(nullptr)
    {
        m_fn = dlsym(module.m_libHandle, symbol.c_str());
        const char* error = dlerror();
        if (error)
        {
            std::cerr << "Fail to open symbols file: " << symbol << std::endl
                      << "Error from dlsym(): " << error << std::endl;
        }
    }

    FunctionHandle(const FunctionHandle&) = delete;
    FunctionHandle& operator=(const FunctionHandle&) = delete;

    ~FunctionHandle() override = default;

    int operator()(int argc, char** argv) override
    {
        using Fn = int(int, char**);

        Fn* fn = reinterpret_cast<Fn*>(m_fn);
        return (*fn)(argc, argv);
    }

    int operator()() override
    {
        using Fn = int();

        Fn* fn = reinterpret_cast<Fn*>(m_fn);
        return (*fn)();
    }

    bool IsValid() override { return m_fn != nullptr; }

private:
    void* m_fn;
};

std::shared_ptr<AZ::Test::IFunctionHandle> ModuleHandle::GetFunction(const std::string& name)
{
    return std::make_shared<FunctionHandle>(*this, name);
}

namespace AZ
{
    namespace Test
    {

        Platform& GetPlatform()
        {
            static Platform s_platform;
            return s_platform;
        }

        bool Platform::SupportsWaitForDebugger()
        {
            return false;
        }

        std::shared_ptr<IModuleHandle> Platform::GetModule(const std::string& lib)
        {
            return std::make_shared<ModuleHandle>(lib);
        }

        void Platform::WaitForDebugger()
        {
            std::cerr << "Platform does not support waiting for debugger." << std::endl;
        }

        void Platform::SuppressPopupWindows()
        {
        }

        std::string Platform::GetModuleNameFromPath(const std::string& path)
        {
            size_t start = path.rfind('/');
            if (start == std::string::npos)
            {
                start = 0;
            }
            size_t end = path.rfind('.');

            return path.substr(start, end);
        }

        void Platform::Printf(const char* format, ...)
        {
            // Not currently supported
        }

        AZ::EnvironmentInstance Platform::GetTestRunnerEnvironment()
        {
            // Not currently supported
            return nullptr;
        }
    } // Test
} // AZ
