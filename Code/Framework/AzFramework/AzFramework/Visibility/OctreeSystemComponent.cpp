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

#include <AzFramework/Visibility/OctreeSystemComponent.h>
#include <AzCore/Math/ShapeIntersection.h>

namespace AzFramework
{
    AZ_CVAR(bool,     bg_octreeUseQuadtree,        false, nullptr, AZ::ConsoleFunctorFlags::ReadOnly, "If set to true, the octreeSystemComponent will degenerate to a quadtree split along the X/Y plane");
    AZ_CVAR(float,    bg_octreeMaxWorldExtents, 16384.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "Maximum supported world size by the world octreeSystemComponent");
    AZ_CVAR(uint32_t, bg_octreeNodeMaxEntries,       64, nullptr, AZ::ConsoleFunctorFlags::Null, "Maximum number of entries to allow in any node before forcing a split");
    AZ_CVAR(uint32_t, bg_octreeNodeMinEntries,       32, nullptr, AZ::ConsoleFunctorFlags::Null, "Minimum number of entries to allow in a node resulting from a merge operation");


    static uint32_t GetChildNodeCount()
    {
        constexpr uint32_t QuadtreeNodeChildCount = 4;
        constexpr uint32_t OctreeNodeChildCount   = 8;
        return (bg_octreeUseQuadtree) ? QuadtreeNodeChildCount : OctreeNodeChildCount;
    }


    OctreeNode::OctreeNode(const AZ::Aabb& bounds)
        : m_bounds(bounds)
    {
        ;
    }


    OctreeNode::OctreeNode(OctreeNode&& rhs)
        : m_bounds(rhs.m_bounds)
        , m_parent(rhs.m_parent)
        , m_children(rhs.m_children)
        , m_entries(AZStd::move(rhs.m_entries))
    {
        // Correct internal node pointers
        for (VisibilityEntry* entry : m_entries)
        {
            entry->m_internalNode = this;
        }
    }


    OctreeNode& OctreeNode::operator=(OctreeNode&& rhs)
    {
        m_bounds = rhs.m_bounds;
        m_parent = rhs.m_parent;
        m_children = rhs.m_children;
        m_entries = AZStd::move(rhs.m_entries);

        // Correct internal node pointers
        for (VisibilityEntry* entry : m_entries)
        {
            entry->m_internalNode = this;
        }

        return *this;
    }


    void OctreeNode::Insert(OctreeSystemComponent& octreeSystemComponent, VisibilityEntry* entry)
    {
        AZ_Assert(entry->m_internalNode == nullptr, "Double-insertion: Insert invoked for an entry already bound to the OctreeSystemComponent");

        // If this is not a leaf node, try to insert into the child nodes
        if (m_children != nullptr)
        {
            const AZ::Aabb boundingVolume = entry->m_boundingVolume;
            const uint32_t childCount = GetChildNodeCount();
            for (uint32_t child = 0; child < childCount; ++child)
            {
                if (AZ::ShapeIntersection::Contains(m_children[child].m_bounds, boundingVolume))
                {
                    return m_children[child].Insert(octreeSystemComponent, entry);
                }
            }
        }

        // If we reach here, either we don't have children or the entry overlaps multiple child nodes
        // Attempt to add the entry to the current nodes entry set, forcing a split if necessary
        if ((m_children == nullptr) && (m_entries.size() >= bg_octreeNodeMaxEntries))
        {
            // If we're not already split, and our entry list gets too large, split this node
            Split(octreeSystemComponent);
            Insert(octreeSystemComponent, entry);
        }
        else
        {
            m_entries.push_back(entry);
            entry->m_internalNode = this;
            entry->m_internalNodeIndex = aznumeric_cast<uint32_t>(m_entries.size() - 1);
        }
    }


    void OctreeNode::Update(OctreeSystemComponent& octreeSystemComponent, VisibilityEntry* entry)
    {
        AZ_Assert(entry->m_internalNode == this, "Update invoked for an entry bound to a different OctreeNode");

        const AZ::Aabb boundingVolume = entry->m_boundingVolume;
        if (IsLeaf() && AZ::ShapeIntersection::Contains(m_bounds, boundingVolume))
        {
            // Entry moved, but is still fully contained within the current node
            // We can only do this for leaf nodes, otherwise entries can get 'stuck' in non-leaf nodes
            // even when one of the child nodes would be an adequate fit, due to this early out check
            return;
        }

        // Remove the entry from our current node, since it is no longer contained
        Remove(octreeSystemComponent, entry);

        // Traverse up our ancestor nodes to find the first node that fully contains the entry
        // This strategy assumes an entry will typically move a small distance relative to the total world
        OctreeNode* insertCheck = this;
        while (insertCheck != nullptr)
        {
            if (AZ::ShapeIntersection::Contains(insertCheck->m_bounds, boundingVolume))
            {
                return insertCheck->Insert(octreeSystemComponent, entry);
            }
            insertCheck = insertCheck->m_parent;
        }
    }


    void OctreeNode::Remove(OctreeSystemComponent& octreeSystemComponent, VisibilityEntry* entry)
    {
        AZ_Assert(entry->m_internalNode == this, "Remove invoked for an entry bound to a different OctreeNode");
        AZ_Assert(m_entries[entry->m_internalNodeIndex] == entry, "Visibility entry data is corrupt");

        // Swap and pop the removed entry
        const uint32_t removeIndex = entry->m_internalNodeIndex;
        m_entries[removeIndex]->m_internalNode = nullptr;
        m_entries[removeIndex]->m_internalNodeIndex = 0;
        if (removeIndex < (m_entries.size() - 1))
        {
            AZStd::swap(m_entries[removeIndex], m_entries.back());
            m_entries[removeIndex]->m_internalNodeIndex = removeIndex;
        }
        m_entries.pop_back();

        if (m_parent != nullptr)
        {
            m_parent->TryMerge(octreeSystemComponent);
        }
    }


    void OctreeNode::Enumerate(const AZ::Aabb& aabb, const IVisibilitySystem::EnumerateCallback& callback) const
    {
        EnumerateHelper(aabb, callback);
    }


    void OctreeNode::Enumerate(const AZ::Sphere& sphere, const IVisibilitySystem::EnumerateCallback& callback) const
    {
        EnumerateHelper(sphere, callback);
    }


    void OctreeNode::Enumerate(const AZ::Frustum& frustum, const IVisibilitySystem::EnumerateCallback& callback) const
    {
        EnumerateHelper(frustum, callback);
    }

    void OctreeNode::EnumerateNoCull(const IVisibilitySystem::EnumerateCallback& callback) const
    {
        // Invoke the callback for the current node
        if (!m_entries.empty())
        {
            callback({m_bounds, m_entries});
        }

        if (m_children != nullptr)
        {
            // If this is not a leaf node, recurse into the children
            const uint32_t childCount = GetChildNodeCount();
            for (uint32_t child = 0; child < childCount; ++child)
            {
                m_children[child].EnumerateNoCull(callback);
            }
        }
    }

    const AZStd::vector<VisibilityEntry*>& OctreeNode::GetEntries() const
    {
        return m_entries;
    }


    OctreeNode* OctreeNode::GetChildren() const
    {
        return m_children;
    }


    bool OctreeNode::IsLeaf() const
    {
        return m_children == nullptr;
    }


    void OctreeNode::TryMerge(OctreeSystemComponent& octreeSystemComponent)
    {
        if (IsLeaf())
        {
            return;
        }

        uint32_t potentialNodeCount = aznumeric_cast<uint32_t>(m_entries.size());

        // Check ourselves and all our siblings for mergeability
        const uint32_t childCount = GetChildNodeCount();
        for (uint32_t child = 0; child < childCount; ++child)
        {
            m_children[child].TryMerge(octreeSystemComponent);
            if (!m_children[child].IsLeaf())
            {
                return;
            }
            potentialNodeCount += aznumeric_cast<uint32_t>(m_children[child].m_entries.size());
        }

        if (potentialNodeCount <= bg_octreeNodeMinEntries)
        {
            Merge(octreeSystemComponent);
        }
    }


    template <typename T>
    void OctreeNode::EnumerateHelper(const T& boundingVolume, const IVisibilitySystem::EnumerateCallback& callback) const
    {
        AZ_Assert(AZ::ShapeIntersection::Overlaps(boundingVolume, m_bounds), "EnumerateHelper invoked on an octreeSystemComponent node that is not within the bounding volume");

        // Invoke the callback for the current node
        if (!m_entries.empty())
        {
            callback({m_bounds, m_entries});
        }

        if (m_children != nullptr)
        {
            // If this is not a leaf node, recurse into the children
            const uint32_t childCount = GetChildNodeCount();
            for (uint32_t child = 0; child < childCount; ++child)
            {
                if (AZ::ShapeIntersection::Overlaps(boundingVolume, m_children[child].m_bounds))
                {
                    m_children[child].EnumerateHelper(boundingVolume, callback);
                }
            }
        }
    }


    void OctreeNode::Split(OctreeSystemComponent& octreeSystemComponent)
    {
        AZ_Assert(m_children == nullptr, "Split invoked on an octreeSystemComponent node that has already been split");
        m_childNodeIndex = octreeSystemComponent.AllocateChildNodes();
        m_children = octreeSystemComponent.GetChildNodesAtIndex(m_childNodeIndex);

        // Set child split planes and bounding volumes
        {
            const AZ::Vector3 childExtent = (m_bounds.GetMax() - m_bounds.GetMin()) * 0.5f;
            const AZ::Aabb childBound = AZ::Aabb::CreateFromMinMax(m_bounds.GetMin(), m_bounds.GetMin() + childExtent);
            const uint32_t childCount = GetChildNodeCount();

            for (uint32_t child = 0; child < childCount; ++child)
            {
                // Note that the ordering of these offsets is such that in QuadTree mode, we will split along the X/Y plane
                // This is because we use a slightly non-standard Z-up ground plane
                // If we ever change to an X/Z ground plane with a Y-up axis, we'll have to swap the Y and Z extent offsets
                AZ::Vector3 childOffset = AZ::Vector3::CreateZero();

                if (child & 0x01)
                {
                    childOffset.SetX(childExtent.GetX());
                }

                if (child & 0x02)
                {
                    childOffset.SetY(childExtent.GetY());
                }

                if (child & 0x04)
                {
                    childOffset.SetZ(childExtent.GetZ());
                }

                m_children[child].m_bounds = childBound.GetTranslated(childOffset);
                m_children[child].m_parent = this;
            }
        }

        // Re-partition our entry set across ourself and our child nodes
        AZStd::vector<VisibilityEntry*> entrySet(AZStd::move(m_entries));
        for (VisibilityEntry* entry : entrySet)
        {
            entry->m_internalNode = nullptr;
            entry->m_internalNodeIndex = 0;
            Insert(octreeSystemComponent, entry);
        }
    }


    void OctreeNode::Merge(OctreeSystemComponent& octreeSystemComponent)
    {
        AZ_Assert(m_children != nullptr, "Merge invoked on an octreeSystemComponent node that does not have children");

        // Move all child entries to our own entry set
        const uint32_t childCount = GetChildNodeCount();
        for (uint32_t child = 0; child < childCount; ++child)
        {
            for (VisibilityEntry* childEntry : m_children[child].m_entries)
            {
                childEntry->m_internalNode = this;
                childEntry->m_internalNodeIndex = aznumeric_cast<uint32_t>(m_entries.size());
                m_entries.push_back(childEntry);
            }
            m_children[child].m_entries.clear();
        }

        octreeSystemComponent.ReleaseChildNodes(m_childNodeIndex);
        m_childNodeIndex = InvalidChildNodeIndex;
        m_children = nullptr;
    }


    void OctreeSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<OctreeSystemComponent, AZ::Component>()
                ->Version(1);
        }
    }


    void OctreeSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("VisibilityService"));
    }


    void OctreeSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("VisibilityService"));
    }


    OctreeSystemComponent::OctreeSystemComponent()
        : m_root(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-bg_octreeMaxWorldExtents), AZ::Vector3(bg_octreeMaxWorldExtents)))
    {
        AZ::Interface<IVisibilitySystem>::Register(this);
        IVisibilitySystemRequestBus::Handler::BusConnect();
    }


    OctreeSystemComponent::~OctreeSystemComponent()
    {
        IVisibilitySystemRequestBus::Handler::BusDisconnect();
        AZ::Interface<IVisibilitySystem>::Unregister(this);
        for (auto page : m_nodeCache)
        {
            delete page;
        }
        m_nodeCache.reserve(0);
        m_nodeCache.shrink_to_fit();
    }


    void OctreeSystemComponent::Activate()
    {
        ;
    }


    void OctreeSystemComponent::Deactivate()
    {
        ;
    }


    void OctreeSystemComponent::InsertOrUpdateEntry(VisibilityEntry& entry)
    {
        if (entry.m_internalNode != nullptr)
        {
            static_cast<OctreeNode*>(entry.m_internalNode)->Update(*this, &entry);
        }
        else
        {
            m_root.Insert(*this, &entry);
            ++m_entryCount;
        }
    }


    void OctreeSystemComponent::RemoveEntry(VisibilityEntry& entry)
    {
        if (entry.m_internalNode)
        {
            static_cast<OctreeNode*>(entry.m_internalNode)->Remove(*this, &entry);
            --m_entryCount;
        }
    }


    void OctreeSystemComponent::Enumerate(const AZ::Aabb& aabb, const IVisibilitySystem::EnumerateCallback& callback) const
    {
        m_root.Enumerate(aabb, callback);
    }


    void OctreeSystemComponent::Enumerate(const AZ::Sphere& sphere, const IVisibilitySystem::EnumerateCallback& callback) const
    {
        m_root.Enumerate(sphere, callback);
    }


    void OctreeSystemComponent::Enumerate(const AZ::Frustum& frustum, const IVisibilitySystem::EnumerateCallback& callback) const
    {
        m_root.Enumerate(frustum, callback);
    }

    void OctreeSystemComponent::EnumerateNoCull(const IVisibilitySystem::EnumerateCallback& callback) const
    {
        m_root.EnumerateNoCull(callback);
    }

    uint32_t OctreeSystemComponent::GetEntryCount() const
    {
        return m_entryCount;
    }

    OctreeNode& OctreeSystemComponent::GetRoot()
    {
        return m_root;
    }

    uint32_t OctreeSystemComponent::GetNodeCount() const
    {
        return m_nodeCount;
    }


    uint32_t OctreeSystemComponent::GetFreeNodeCount() const
    {
        // Each entry represents GetChildNodeCount() nodes
        return aznumeric_cast<uint32_t>(m_freeOctreeNodes.size() * GetChildNodeCount());
    }


    uint32_t OctreeSystemComponent::GetPageCount() const
    {
        return aznumeric_cast<uint32_t>(m_nodeCache.size());
    }


    uint32_t OctreeSystemComponent::GetChildNodeCount() const
    {
        return AzFramework::GetChildNodeCount();
    }


    void OctreeSystemComponent::DumpStats([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        AZ_TracePrintf("Console", "OctreeNode::EntryCount = %u", GetEntryCount());
        AZ_TracePrintf("Console", "OctreeNode::NodeCount = %u", GetNodeCount());
        AZ_TracePrintf("Console", "OctreeNode::FreeNodeCount = %u", GetFreeNodeCount());
        AZ_TracePrintf("Console", "OctreeNode::PageCount = %u", GetPageCount());
        AZ_TracePrintf("Console", "OctreeNode::ChildNodeCount = %u", GetChildNodeCount());
    }


    static inline uint32_t CreateNodeIndex(uint32_t page, uint32_t offset)
    {
        AZ_Assert(page <= 0xFFFF && offset <= 0xFFFF, "Out of range values passed to CreateNodeIndex");
        return (page << 16) | offset;
    }


    static inline void ExtractPageAndOffsetFromIndex(uint32_t index, uint32_t& page, uint32_t& offset)
    {
        offset = index & 0x0000FFFF;
        page = index >> 16;
    }


    uint32_t OctreeSystemComponent::AllocateChildNodes()
    {
        const uint32_t childCount = GetChildNodeCount();
        m_nodeCount += childCount;

        if (m_nodeCache.empty())
        {
            m_nodeCache.push_back(new OctreeNodePage);
        }

        uint32_t nextChildPage = aznumeric_cast<uint32_t>(m_nodeCache.size() - 1);
        uint32_t nextChildOffset = aznumeric_cast<uint32_t>(m_nodeCache[nextChildPage]->size());

        if (!m_freeOctreeNodes.empty())
        {
            // Take a free block of child nodes from our free list
            ExtractPageAndOffsetFromIndex(m_freeOctreeNodes.back(), nextChildPage, nextChildOffset);
            m_freeOctreeNodes.pop();
        }
        else
        {
            if (nextChildOffset >= BlockSize)
            {
                // Our last page is already full, so we need to allocate a new page
                m_nodeCache.push_back(new OctreeNodePage);
                ++nextChildPage;
                nextChildOffset = 0;
            }

            // Our (potentially new) last page has unused capacity
            m_nodeCache[nextChildPage]->resize_no_construct(nextChildOffset + GetChildNodeCount());

            // We resize_no_construct to prevent fixed_vector from using copy or assignment operators, but this means we have to explicitly construct nodes ourselves
            OctreeNode* childNodes = &(*m_nodeCache[nextChildPage])[nextChildOffset];
            for (uint32_t child = 0; child < childCount; ++child)
            {
                new (&childNodes[child]) OctreeNode;
            }
        }

        return CreateNodeIndex(nextChildPage, nextChildOffset);
    }


    void OctreeSystemComponent::ReleaseChildNodes(uint32_t nodeIndex)
    {
        m_nodeCount -= GetChildNodeCount();
        m_freeOctreeNodes.push(nodeIndex);
    }


    OctreeNode* OctreeSystemComponent::GetChildNodesAtIndex(uint32_t nodeIndex) const
    {
        uint32_t childPage;
        uint32_t childOffset;
        ExtractPageAndOffsetFromIndex(nodeIndex, childPage, childOffset);
        return &(*m_nodeCache[childPage])[childOffset];
    }
}