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

#include <AzNetworking/DataStructures/IBitset.h>
#include <AzCore/base.h>

namespace AzNetworking
{
    //! @class FixedSizeBitsetView
    //! @brief Creates a view into a subset of an IBitset.
    class FixedSizeBitsetView
        : public IBitset
    {
    public:

        //! Construct a bitset view.
        //! @param bitset a bitset to create the view from
        //! @param startOffset starting bit
        //! @param count total number of bits to use
        FixedSizeBitsetView(IBitset& bitset, uint32_t startOffset, uint32_t count);

        //! Sets the specified bit to the provided value.
        //! @param index index of the bit to set
        //! @param value value to set the bit to
        virtual void SetBit(uint32_t index, bool value) override;

        //! Gets the current value of the specified bit.
        //! @param index index of the bit to retrieve the value of
        //! @return boolean true if the bit is set, false otherwise
        virtual bool GetBit(uint32_t index) const override;

        //! Gets the current value of the specified bit.
        //! @param index index of the bit to retrieve the value of
        //! @return boolean true if the bit is set, false otherwise
        virtual bool AnySet() const override;

        //! Returns the number of bits that are represented in this fixed size bitset.
        //! @return the number of bits that are represented in this fixed size bitset
        virtual uint32_t GetValidBitCount() const override;

    private:

        IBitset& m_bitset;
        const uint32_t m_startOffset;
        const uint32_t m_count;
    };
}

#include <AzNetworking/DataStructures/FixedSizeBitsetView.inl>
