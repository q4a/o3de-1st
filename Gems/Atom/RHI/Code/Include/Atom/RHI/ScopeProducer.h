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

#include <Atom/RHI/Scope.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>
#include <Atom/RHI/FrameGraphExecuteContext.h>

namespace AZ
{
    namespace RHI
    {
        /*
            ScopeProducer is the base class for systems which produce scopes on the frame scheduler.
            The user is expected to inherit from this class and implement three virtual methods:
                - SetupFrameGraphDependencies
                - CompileResources
                - BuildCommandList
            It can then be registered with the frame scheduler each frame. Internally, this process
            generates a Scope which is inserted to the frame graph internally.

            EXAMPLE:

            class MyScope : public RHI::ScopeProducer
            {
            public:
                MyScope()
                    : RHI::ScopeProducer(RHI::ScopeId{"MyScopeId"})
                {}

            private:
                void SetupFrameGraphDependencies(FrameGraphInterface frameGraph) override
                {
                    // Create attachments on the builder, use them.
                }

                void CompileResources(const FrameGraphCompileContext& context) override
                {
                    // Use the provided context to access image / buffer views and
                    // build ShaderResourceGroups.
                }

                void BuildCommandList(const FrameGraphExecuteContext& context) override
                {
                    // A context is provided which allows you to access the command
                    // list for execution.
                }
            };

            MyScope scope;

            // Register with the scheduler each frame. Callbacks will be
            // invoked.
            frameScheduler.AddScope(scope);
        */

        class ScopeProducer
        {
            friend class FrameScheduler;
        public:
            virtual ~ScopeProducer() = default;
            ScopeProducer(const ScopeId& scopeId);

            /**
             * Returns the scope id associated with this scope producer.
             */
            const ScopeId& GetScopeId() const;

            /**
             * Returns the scope associated with this scope producer.
             */
            const Scope* GetScope() const;

        protected:

            /** 
             *  Protected default constructor for classes that inherit from
             *  ScopeProducer but that can't supply a ScopeId at construction.
             */
            ScopeProducer();

            /**
             *  Sets ID of the scope producer. Used by class that inherit from
             *  ScopeProducer but that can't supply a ScopeId at construction.
             */
            void SetScopeId(const ScopeId& scopeId);

        private:
            //////////////////////////////////////////////////////////////////////////
            // User Overrides - Derived classes should override from these methods.

            /**
             * This function is called during the schedule setup phase. The client is expected to declare
             * attachments using the provided \param frameGraph.
             */
            virtual void SetupFrameGraphDependencies(FrameGraphInterface frameGraph) = 0;

            /**
             * This function is called after compilation of the frame graph, but before execution. The provided
             * FrameGraphAttachmentContext allows you to access RHI views associated with attachment
             * ids. This is the method to build ShaderResourceGroups from transient attachment views.
             */
            virtual void CompileResources(const FrameGraphCompileContext& context) { AZ_UNUSED(context); }

            /**
             * This function is called at command list recording time and may be called multiple times
             * if the schedule decides to split work items across command lists. In this case, each invocation
             * will provide a command list and invocation index.
             */
            virtual void BuildCommandList(const FrameGraphExecuteContext& context) { AZ_UNUSED(context); }

            //////////////////////////////////////////////////////////////////////////

            Scope* GetScope();

            ScopeId m_scopeId;
            Ptr<Scope> m_scope;
        };
    }
}
