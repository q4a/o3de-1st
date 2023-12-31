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

#include <AzFramework/Physics/Collision/CollisionGroups.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Physics/CollisionBus.h>

//This bit is defined in the TouchBending Gem wscript.
//Make sure the bit has a valid value.
#ifdef TOUCHBENDING_LAYER_BIT
#if (TOUCHBENDING_LAYER_BIT < 1) || (TOUCHBENDING_LAYER_BIT > 63)
#error Invalid Bit Definition For the TouchBending Layer Bit
#endif
#endif //#ifdef TOUCHBENDING_LAYER_BIT

namespace AzPhysics
{
    AZ_CLASS_ALLOCATOR_IMPL(CollisionGroup, AZ::SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(CollisionGroups, AZ::SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(CollisionGroups::Id, AZ::SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(CollisionGroups::Preset, AZ::SystemAllocator, 0);

    const CollisionGroup CollisionGroup::None = 0x0000000000000000ULL;
    const CollisionGroup CollisionGroup::All = 0xFFFFFFFFFFFFFFFFULL;

#ifdef TOUCHBENDING_LAYER_BIT
    const CollisionGroup CollisionGroup::All_NoTouchBend = CollisionGroup::All.GetMask() & ~CollisionLayer::TouchBend.GetMask();
#endif

    void CollisionGroup::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CollisionGroup>()
                ->Version(1)
                ->Field("Mask", &CollisionGroup::m_mask)
                ;
        }

        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AzPhysics::CollisionGroup>("CollisionGroup")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "AzPhysics")
                ->Constructor<const AZStd::string>()
                ;
        }
    }

    void CollisionGroups::Reflect(AZ::ReflectContext* context)
    {
        CollisionGroups::Id::Reflect(context);
        CollisionGroups::Preset::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CollisionGroups>()
                ->Version(1)
                ->Field("GroupPresets", &CollisionGroups::m_groups)
                ;
        }
    }

    void CollisionGroups::Id::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CollisionGroups::Id>()
                ->Version(1)
                ->Field("GroupId", &CollisionGroups::Id::m_id)
                ;
        }
    }

    void CollisionGroups::Preset::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CollisionGroups::Preset>()
                ->Version(1)
                ->Field("Id", &CollisionGroups::Preset::m_id)
                ->Field("Name", &CollisionGroups::Preset::m_name)
                ->Field("Group", &CollisionGroups::Preset::m_group)
                ->Field("ReadOnly", &CollisionGroups::Preset::m_readOnly)
                ;
        }
    }

    bool CollisionGroups::Preset::operator==(const Preset& other) const
    {
        return m_readOnly == other.m_readOnly &&
            m_group == other.m_group &&
            m_name == other.m_name &&
            m_id == other.m_id
            ;
    }

    bool CollisionGroups::Preset::operator!=(const Preset& other) const
    {
        return !(*this == other);
    }

    CollisionGroup::CollisionGroup(AZ::u64 mask) 
        : m_mask(mask)
    {
    }

    CollisionGroup::CollisionGroup(const AZStd::string& groupName)
    {
        CollisionGroup group;
        Physics::CollisionRequestBus::BroadcastResult(group, &Physics::CollisionRequests::GetCollisionGroupByName, groupName);
        m_mask = group.GetMask();
    }

    void CollisionGroup::SetLayer(CollisionLayer layer, bool set)
    {
        if (set)
        {
            m_mask |= 1ULL << layer.GetIndex();
        }
        else
        { 
            m_mask &= ~(1ULL << layer.GetIndex());
        }
    }

    bool CollisionGroup::IsSet(CollisionLayer layer) const
    {
        return (m_mask & layer.GetMask()) != 0;
    }

    AZ::u64 CollisionGroup::GetMask() const
    {
        return m_mask;
    }

    bool CollisionGroup::operator!=(const CollisionGroup& collisionGroup) const
    {
        return collisionGroup.m_mask != m_mask;
    }

    bool CollisionGroup::operator==(const CollisionGroup& collisionGroup) const
    {
        return collisionGroup.m_mask == m_mask;
    }

    CollisionGroups::Id CollisionGroups::CreateGroup(const AZStd::string& name, CollisionGroup group, Id id, bool readOnly)
    {
        Preset preset;
        preset.m_id = id;
        preset.m_name = name;
        preset.m_group = group;
        preset.m_readOnly = readOnly;
        m_groups.push_back(preset);
        return preset.m_id;
    }

    void CollisionGroups::DeleteGroup(Id id)
    {
        if (!id.m_id.IsNull())
        {
            auto last = AZStd::remove_if(m_groups.begin(), m_groups.end(), [id](const Preset& preset)
            {
                return preset.m_id == id;
            });
            m_groups.erase(last);
        }
    }

    CollisionGroup CollisionGroups::FindGroupById(CollisionGroups::Id id) const
    {
        auto found = AZStd::find_if(m_groups.begin(), m_groups.end(), [id](const Preset& preset) 
        {
            return preset.m_id == id;
        });

        if (found != m_groups.end())
        {
            return found->m_group;
        }
        return CollisionGroup::All;
    }

    CollisionGroup CollisionGroups::FindGroupByName(const AZStd::string& groupName) const
    {
        CollisionGroup group = CollisionGroup::All;
        TryFindGroupByName(groupName, group);
        return group;
    }

    bool CollisionGroups::TryFindGroupByName(const AZStd::string& groupName, CollisionGroup& group) const
    {
        auto found = AZStd::find_if(m_groups.begin(), m_groups.end(), [groupName](const Preset& preset)
        {
            return preset.m_name == groupName;
        });

        if (found != m_groups.end())
        {
            group = found->m_group;
            return true;
        }

        AZ_Warning("CollisionGroups", false, "Could not find collision group:%s. Does it exist in the physx configuration window?", groupName.c_str());
        return false;
    }

    CollisionGroups::Id CollisionGroups::FindGroupIdByName(const AZStd::string& groupName) const
    {
        auto found = AZStd::find_if(m_groups.begin(), m_groups.end(), [groupName](const Preset& preset)
        {
            return preset.m_name == groupName;
        });

        if (found != m_groups.end())
        {
            return found->m_id;
        }
        return CollisionGroups::Id();
    }

    AZStd::string CollisionGroups::FindGroupNameById(Id id) const
    {
        auto found = AZStd::find_if(m_groups.begin(), m_groups.end(), [id](const Preset& preset)
        {
            return preset.m_id.m_id == id.m_id;
        });

        if (found != m_groups.end())
        {
            return found->m_name;
        }
        return "";
    }

    void CollisionGroups::SetGroupName(CollisionGroups::Id id, const AZStd::string& groupName)
    {
        auto found = AZStd::find_if(m_groups.begin(), m_groups.end(), [id](const Preset& preset)
        {
            return preset.m_id.m_id == id.m_id;
        });

        if (found != m_groups.end())
        {
            found->m_name = groupName;
        }
    }

    void CollisionGroups::SetLayer(Id id, CollisionLayer layer, bool enabled)
    {
        auto found = AZStd::find_if(m_groups.begin(), m_groups.end(), [id](const Preset& preset)
        {
            return preset.m_id.m_id == id.m_id;
        });

        if (found != m_groups.end())
        {
            found->m_group.SetLayer(layer, enabled);
        }
    }

    const AZStd::vector<CollisionGroups::Preset>& CollisionGroups::GetPresets() const
    {
        return m_groups;
    }

    bool CollisionGroups::operator==(const CollisionGroups& other) const
    {
        return m_groups == other.m_groups;
    }

    bool CollisionGroups::operator!=(const CollisionGroups& other) const
    {
        return !(*this == other);
    }

    CollisionGroup operator|(CollisionLayer layer1, CollisionLayer layer2)
    {
        CollisionGroup group = CollisionGroup::None;
        group.SetLayer(layer1, true);
        group.SetLayer(layer2, true);
        return group;
    }

    CollisionGroup operator|(CollisionGroup otherGroup, CollisionLayer layer)
    {
        CollisionGroup group = otherGroup;
        group.SetLayer(layer, true);
        return group;
    }
}

