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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Threading/ThreadSafeDeque.h>
#include <AzCore/std/string/string.h>
#include <AzNetworking/ConnectionLayer/IConnectionListener.h>
#include <Source/NetworkTime/NetworkTime.h>
#include <Source/AutoGen/Multiplayer.AutoPacketDispatcher.h>

namespace AzNetworking
{
    class INetworkInterface;
}

namespace Multiplayer
{
    //! Multiplayer system component wraps the bridging logic between the game and transport layer.
    class MultiplayerSystemComponent final
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public AzNetworking::IConnectionListener
    {
    public:
        AZ_COMPONENT(MultiplayerSystemComponent, "{7C99C4C1-1103-43F9-AD62-8B91CF7C1981}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        MultiplayerSystemComponent();
        ~MultiplayerSystemComponent() override = default;

        //! AZ::Component overrides.
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        //! AZ::TickBus::Handler overrides.
        //! @{
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;
        //! @}

        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, const MultiplayerPackets::Connect& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, const MultiplayerPackets::Accept& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, const MultiplayerPackets::SyncConsole& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, const MultiplayerPackets::ConsoleCommand& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, const MultiplayerPackets::SyncConnectionCvars& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, const MultiplayerPackets::EntityUpdates& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, const MultiplayerPackets::EntityRpcs& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, const MultiplayerPackets::ClientMigration& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, const MultiplayerPackets::NotifyClientMigration& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, const MultiplayerPackets::EntityMigration& packet);

        //! IConnectionListener interface
        //! @{
        AzNetworking::ConnectResult ValidateConnect(const AzNetworking::IpAddress& remoteAddress, const AzNetworking::IPacketHeader& packetHeader, AzNetworking::ISerializer& serializer) override;
        void OnConnect(AzNetworking::IConnection* connection) override;
        bool OnPacketReceived(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, AzNetworking::ISerializer& serializer) override;
        void OnPacketLost(AzNetworking::IConnection* connection, AzNetworking::PacketId packetId) override;
        void OnDisconnect(AzNetworking::IConnection* connection, AzNetworking::DisconnectReason reason, AzNetworking::TerminationEndpoint endpoint) override;
        //! @}

    private:

        void OnConsoleCommandInvoked(AZStd::string_view command, const AZ::ConsoleCommandContainer& args, AZ::ConsoleFunctorFlags flags, AZ::ConsoleInvokedFrom invokedFrom);
        void ExecuteConsoleCommandList(AzNetworking::IConnection* connection, const AZStd::fixed_vector<Multiplayer::LongNetworkString, 32>& commands);

        AzNetworking::INetworkInterface* m_networkInterface = nullptr;
        AZ::ConsoleCommandInvokedEvent::Handler m_consoleCommandHandler;
        AZ::ThreadSafeDeque<AZStd::string> m_cvarCommands;

        NetworkTime m_networkTime;
    };
}
