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

#include "ResourceCompilerScene_precompiled.h"
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneSerializationHandler.h>

namespace AZ
{
    namespace RC
    {
        void SceneSerializationHandler::Activate()
        {
            BusConnect();
        }

        void SceneSerializationHandler::Deactivate()
        {
            BusDisconnect();
        }

        void SceneSerializationHandler::Reflect(ReflectContext* context)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<SceneSerializationHandler>()->Version(1);
            }
        }

        AZStd::shared_ptr<SceneAPI::Containers::Scene> SceneSerializationHandler::LoadScene(
            const AZStd::string& filePath, Uuid sceneSourceGuid)
        {
            namespace Utilities = AZ::SceneAPI::Utilities;
            using AZ::SceneAPI::Events::AssetImportRequest;

            AZ_TraceContext("File", filePath);

            if (sceneSourceGuid.IsNull())
            {
                AZ_TracePrintf(Utilities::ErrorWindow, "Invalid source guid for the scene file.");
                return nullptr;
            }

            if (AZ::SceneAPI::Events::AssetImportRequest::IsManifestExtension(filePath.c_str()))
            {
                AZ_TracePrintf(Utilities::ErrorWindow, "Provided path contains the manifest path, not the path to the source file.");
                return nullptr;
            }
            if (!AZ::SceneAPI::Events::AssetImportRequest::IsSceneFileExtension(filePath.c_str()))
            {
                AZ_TracePrintf(Utilities::ErrorWindow, "Provided path doesn't contain an extension supported by the SceneAPI.");
                return nullptr;
            }
            if (AzFramework::StringFunc::Path::IsRelative(filePath.c_str()))
            {
                AZ_TracePrintf(Utilities::ErrorWindow, "Given file path is relative where an absolute path was expected.");
                return nullptr;
            }

            if (!AZ::IO::SystemFile::Exists(filePath.c_str()))
            {
                AZ_TracePrintf(Utilities::ErrorWindow, "No file exists at given source path.");
                return nullptr;
            }

            AZStd::shared_ptr<SceneAPI::Containers::Scene> scene =
                AssetImportRequest::LoadSceneFromVerifiedPath(filePath, sceneSourceGuid, AssetImportRequest::RequestingApplication::AssetProcessor);

            if (!scene)
            {
                AZ_TracePrintf(Utilities::ErrorWindow, "Failed to load the requested scene.");
                return nullptr;
            }

            return scene;
        }
    } // namespace RC
} // namespace AZ
