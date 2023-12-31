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

// LY Editor Crashpad Hook

#include <CrashReporting_precompiled.h>

#include <CrashReporting/GameCrashHandler.h>
#include <CrashSupport.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>

namespace CrashHandler
{

    void GameCrashHandler::InitCrashHandler(const std::string& moduleTag, const std::string& devRoot, const std::string& crashUrl, const std::string& crashToken, const std::string& handlerFolder, const CrashHandlerAnnotations& baseAnnotations, const CrashHandlerArguments& argumentVec)
    {
        GameCrashHandler crashHandler;
        crashHandler.Initialize( moduleTag,  devRoot,  crashUrl,  crashToken, handlerFolder, baseAnnotations, argumentVec);
    }

    std::string GameCrashHandler::GetCrashSubmissionURL() const
    {
#if defined(CRASH_HANDLER_URL)
        return MAKE_DEFINE_STRING(CRASH_HANDLER_URL);
#else
        return "https://lumberyard.sp.backtrace.io:8443/";
#endif
    }

    std::string GameCrashHandler::GetCrashSubmissionToken() const
    {
        std::string configToken = GetConfigSubmissionToken();
        if (configToken.length())
        {
            return configToken;
        }
#if defined(CRASH_HANDLER_TOKEN)
        return MAKE_DEFINE_STRING(CRASH_HANDLER_TOKEN);
#else
        // Add your crash upload token here
        return "";
#endif
    }

    std::string GameCrashHandler::DetermineAppPath() const
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (fileIO)
        {
            // If our user alias is available, use that
            const char* userAlias = fileIO->GetAlias("@user@");
            if (userAlias)
            {
                return userAlias;
            }
        }
        return GetAppRootFromCWD();
    }

    std::string GameCrashHandler::GetCrashHandlerPath(const std::string& basePath) const
    {
        std::string returnPath;

        if (basePath.length())
        {
            returnPath = basePath.c_str();
        }
        else
        {
            AZStd::string enginePath;
            EBUS_EVENT_RESULT(enginePath, AZ::ComponentApplicationBus, GetExecutableFolder);

            if (enginePath.length())
            {
                returnPath = enginePath.c_str();
            }
            else
            {
                GetExecutableFolder(returnPath);
            }
        }

        returnPath += GetCrashHandlerExecutableName();
        return returnPath;
    }
}