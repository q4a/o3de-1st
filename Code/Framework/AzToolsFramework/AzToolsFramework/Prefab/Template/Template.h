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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class Template;
        using TemplateReference = AZStd::optional<AZStd::reference_wrapper<Template>>;

        // A prefab template is the primary product of loading a prefab file from disk. 
        class Template
        {
        public:
            AZ_CLASS_ALLOCATOR(Template, AZ::SystemAllocator, 0);
            AZ_RTTI(Template, "{F6B7DC7B-386A-42DD-BA8B-919A4D024D7C}");

            using Links = AZStd::unordered_set<LinkId>;

            Template() = default;
            Template(const AZStd::string& filePath, PrefabDom prefabDom);
            Template(const Template& other);
            Template& operator=(const Template& other);

            Template(Template&& other) noexcept;
            Template& operator=(Template&& other) noexcept;

            virtual ~Template() noexcept = default;

            bool IsValid() const;
            bool IsLoadedWithErrors() const;

            void MarkAsLoadedWithErrors(bool loadedWithErrors);

            bool AddLink(LinkId newLinkId);
            bool RemoveLink(LinkId linkId);
            bool HasLink(LinkId linkId) const;
            const Links& GetLinks() const;

            PrefabDom& GetPrefabDom();
            const PrefabDom& GetPrefabDom() const;

            bool CopyTemplateIntoPrefabFileFormat(PrefabDom& output);

            PrefabDomValueReference GetInstancesValue();
            PrefabDomValueConstReference GetInstancesValue() const;

            const AZStd::string& GetFilePath() const;

        private:
            // Container for keeping links representing the Template's nested instances.
            Links m_links;

            // JSON Document contains parsed prefab file for users to access data in Prefab file easily.
            PrefabDom m_prefabDom;

            // File path of this Prefab Template.
            AZStd::string m_filePath;

            // Flag to tell if this Template and all its nested instances loaded with any error.
            bool m_isLoadedWithErrors = false;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
