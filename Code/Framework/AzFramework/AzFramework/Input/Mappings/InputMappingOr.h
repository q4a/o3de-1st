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

#include <AzFramework/Input/Mappings/InputMapping.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class that maps multiple different sources to a single output using 'OR' logic.
    //! Example: "gamepad_button_a" OR "keyboard_key_edit_space" -> "gameplay_jump"
    class InputMappingOr : public InputMapping
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputMappingOr, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputMappingOr, "{521D2450-2877-4F9F-A320-9989A4F3E781}", InputMapping);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputChannelId Id of the input mapping
        //! \param[in] inputContext Input context that owns the input mapping
        InputMappingOr(const InputChannelId& inputChannelId,
                       const InputContext& inputContext);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputMappingOr);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default Destructor
        ~InputMappingOr() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Add an input source to this input mapping
        //! \param[in] inputSourceId The id of the input source to add to this input mapping
        //! \return True if the input source was added to this input mapping, false otherwise
        bool AddSourceInput(const InputChannelId& inputSourceId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Remove an input source from this input mapping
        //! \param[in] inputSourceId The id of the input source to remove from this input mapping
        //! \return True if the input source was removed from this input mapping, false otherwise
        bool RemoveSourceInput(const InputChannelId& inputSourceId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the value of the source input channel currently activating this input mapping
        //! \return The value of the source input channel currently activating this input mapping
        float GetValue() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the delta of the source input channel currently activating this input mapping
        //! \return Difference between the current and last reported values of this input mapping
        float GetDelta() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelRequests::ResetState
        void ResetState() override;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputMapping::IsSourceInput
        bool IsSourceInput(const InputChannel& inputChannel) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputMapping::OnSourceInputEvent
        bool OnSourceInputEvent(const InputChannel& inputChannel) override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        SourceInputChannelIds  m_sourceInputChannelIds; //!< All potential source input channel ids
        InputChannel::Snapshot m_currentlyActiveSource; //!< A snapshot of the active source input
    };
} // namespace AzFramework