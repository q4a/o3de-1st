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

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/string/conversions.h>

namespace AZ
{
    SettingsRegistryInterface::Specializations::Specializations(AZStd::initializer_list<AZStd::string_view> specializations)
    {
        for (AZStd::string_view specialization : specializations)
        {
            Append(specialization);
        }
    }

    SettingsRegistryInterface::Specializations& SettingsRegistryInterface::Specializations::operator=(
        AZStd::initializer_list<AZStd::string_view> specializations)
    {
        for (AZStd::string_view specialization : specializations)
        {
            Append(specialization);
        }
        return *this;
    }

    bool SettingsRegistryInterface::Specializations::Append(AZStd::string_view specialization)
    {
        if (m_hashes.size() < m_hashes.max_size())
        {
            m_names.push_back(TagName{ specialization });
            TagName& tag = m_names.back();
            AZStd::to_lower(tag.begin(), tag.end());
            m_hashes.push_back(Hash(tag));
            return true;
        }
        else
        {
            AZ_Error("Settings Registry", false,
                "Too many specialization for the Settings Registry. The maximum is %zu.", m_hashes.max_size());
            return false;
        }
    }

    bool SettingsRegistryInterface::Specializations::Contains(AZStd::string_view specialization) const
    {
        return Contains(Hash(specialization));
    }

    bool SettingsRegistryInterface::Specializations::Contains(size_t hash) const
    {
        return AZStd::find(m_hashes.begin(), m_hashes.end(), hash) != m_hashes.end();
    }

    size_t SettingsRegistryInterface::Specializations::GetPriority(AZStd::string_view specialization) const
    {
        return GetPriority(Hash(specialization));
    }

    size_t SettingsRegistryInterface::Specializations::GetPriority(size_t hash) const
    {
        auto it = AZStd::find(m_hashes.begin(), m_hashes.end(), hash);
        return it != m_hashes.end() ? AZStd::distance(m_hashes.begin(), it) : NotFound;
    }

    size_t SettingsRegistryInterface::Specializations::Hash(AZStd::string_view specialization)
    {
        FixedValueString lowercaseSpecialization{ specialization };
        AZStd::to_lower(lowercaseSpecialization.begin(), lowercaseSpecialization.end());
        return AZStd::hash<FixedValueString>{}(lowercaseSpecialization);
    }

    size_t SettingsRegistryInterface::Specializations::GetCount() const
    {
        return m_names.size();
    }

    AZStd::string_view SettingsRegistryInterface::Specializations::GetSpecialization(size_t index) const
    {
        return index < m_names.size() ? m_names[index] : AZStd::string_view();
    }
} // namespace AZ
