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

#include <AzCore/std/optional.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class Instance;
        namespace PrefabDomUtils
        {
            inline static const char* InstancesName = "Instances";
            inline static const char* PatchesName = "Patches";
            inline static const char* SourceName = "Source";
            inline static const char* LinkIdName = "LinkId";

            /**
            * Find Prefab value from given parent value and target value's name.
            * @param parentValue A parent value in Prefab DOM.
            * @param valueName A name of child value of parentValue.
            * @return Reference to the child value of parentValue with name valueName if the child value exists.
            */
            PrefabDomValueReference FindPrefabDomValue(PrefabDomValue& parentValue, const char* valueName);
            PrefabDomValueConstReference FindPrefabDomValue(const PrefabDomValue& parentValue, const char* valueName);

            /**
            * Stores a valid Prefab Instance within a Prefab Dom. Useful for generating Templates
            * @param instance The instance to store
            * @param prefabDom the prefabDom that will be used to store the Instance data
            * @return bool on whether the operation succeeded
            */
            bool StoreInstanceInPrefabDom(const Instance& instance, PrefabDom& prefabDom);

            /**
            * Loads a valid Prefab Instance from a Prefab Dom. Useful for generating Instances.
            * @param instance The Instance to load.
            * @param prefabDom the prefabDom that will be used to load the Instance data.
            * @param shouldClearContainers whether to clear containers in Instance while loading.
            * @return bool on whether the operation succeeded.
            */
            bool LoadInstanceFromPrefabDom(Instance& instance, const PrefabDom& prefabDom, bool shouldClearContainers);

            inline PrefabDomPath GetPrefabDomInstancePath(const char* instanceName)
            {
                return PrefabDomPath()
                    .Append(InstancesName)
                    .Append(instanceName);
            };

        } // namespace PrefabDomUtils
    } // namespace Prefab
} // namespace AzToolsFramework

