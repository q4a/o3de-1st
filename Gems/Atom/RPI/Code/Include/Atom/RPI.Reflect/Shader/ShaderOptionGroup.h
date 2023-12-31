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

#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>

#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace RPI
    {
        class ShaderOptionDescriptor;

        class ShaderOptionGroup final
        {
            friend class ShaderOptionDescriptor;

        public:
            AZ_TYPE_INFO(ShaderOptionGroup, "{906F69F5-52F0-4095-9562-0E91DDDE6E2F}");
            AZ_CLASS_ALLOCATOR(ShaderOptionGroup, AZ::ThreadPoolAllocator, 0);

            ShaderOptionGroup() = default;
            ShaderOptionGroup(const ShaderOptionGroup& rhs);
            explicit ShaderOptionGroup(const ConstPtr<ShaderOptionGroupLayout>& shaderOptionGroupLayout);
            explicit ShaderOptionGroup(const ConstPtr<ShaderOptionGroupLayout>& shaderOptionGroupLayout, const ShaderVariantId& id);

            //! Clears all values in the group back to empty.
            void Clear();

            //! Resets all shader options to their default values
            void SetAllToDefaultValues();

            //! Resets unspecified shader options to their default values
            void SetUnspecifiedToDefaultValues();

            //! Returns whether all options have been specified
            bool IsFullySpecified() const;

            //! Returns the shader option index associated with the shader option id,
            //! or a null index if the id was not found.
            ShaderOptionIndex FindShaderOptionIndex(const Name& optionName) const;

            //! Helper method which assigns a value (valueId) to the shader option specified by optionName.
            //! For performance reasons consider caching the indices for both names and calling
            //!    void SetValue(ShaderOptionIndex optionIndex, ShaderOptionValue value) instead.
            bool SetValue(const Name& optionName, const Name& valueName);

            //! Helper method which assigns a value to the shader option specified by optionName.
            //! For performance reasons consider caching the index for optionName and calling
            //!    void SetValue(ShaderOptionIndex optionIndex, ShaderOptionValue value) instead.
            bool SetValue(const Name& optionName, ShaderOptionValue valueIndex);

            //! Helper method which gets the value for the shader option specified by optionName.
            //! For performance reasons consider caching the index for optionName and calling
            //!    void GetValue(ShaderOptionIndex optionIndex) instead.
            ShaderOptionValue GetValue(const Name& optionName) const;

            //! Helper method which assigns a value (valueId) to the shader option specified by optionIndex.
            //! For performance reasons consider caching the index for valueName and calling
            //!    void SetValue(ShaderOptionIndex optionIndex, ShaderOptionValue value) instead.
            bool SetValue(ShaderOptionIndex optionIndex, const Name& valueName);

            //! Helper method which assigns a value to the shader option specified by optionIndex
            //! If you have previously cached the shader option descriptor, you might want to use
            //! ShaderOptionDescriptor::Set(ShaderOptionGroup&, const ShaderOptionValue&) instead.
            bool SetValue(ShaderOptionIndex optionIndex, ShaderOptionValue valueIndex);

            //! Helper method which gets the value set for the shader option specified by optionIndex.
            //! If you have previously cached the shader option descriptor, you might want to use
            //! ShaderOptionDescriptor::Get(ShaderOptionGroup&) instead.
            //! Returns a Null ShaderOptionValue if the value is unspecified.
            ShaderOptionValue GetValue(ShaderOptionIndex optionIndex) const;

            //! Resets the shader option value to an uninitialized state.
            //! For performance reasons consider caching the index for optionName and calling
            //!    void ClearValue(ShaderOptionIndex optionIndex) instead.
            bool ClearValue(const Name& optionName);

            //! Resets the shader option value to an uninitialized state.
            bool ClearValue(ShaderOptionIndex optionIndex);

            //! Returns the constructed key.
            const ShaderVariantKey& GetShaderVariantKey() const;

            //! Returns the constructed mask.
            const ShaderVariantKey& GetShaderVariantMask() const;

            //! Returns the constructed id, which contains both the shader variant key and mask
            const ShaderVariantId& GetShaderVariantId() const;            

            //! Returns the shader option layout used to build the key.
            const ShaderOptionGroupLayout* GetShaderOptionLayout() const;

            //! The fallback value for this shader option group, to be used in ShaderResourceGroup::SetShaderVariantKeyFallbackValue.
            //! Any unspecified shader option values will be set to their defaults.
            ShaderVariantKey GetShaderVariantKeyFallbackValue() const;
            
            //! Dump the names and values of all shader options, used for debug output
            AZStd::string ToString() const;

        private:
            static const char* DebugCategory;

            //! Returns the constructed key.
            ShaderVariantKey& GetShaderVariantKey();

            //! Returns the constructed mask.
            ShaderVariantKey& GetShaderVariantMask();

            //! Gets a ShaderOptionIndex and reports errors if it can't be found.
            bool GetShaderOptionIndex(const Name& optionName, ShaderOptionIndex& optionIndex) const;

            //! True if the option index is valid. If RPI validation is disabled it will return true regardless.
            bool ValidateIndex(const ShaderOptionIndex& optionIndex) const;

            ConstPtr<ShaderOptionGroupLayout> m_layout;
            ShaderVariantId  m_id;
        };
    } // namespace RPI
} // namespace AZ
