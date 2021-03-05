#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(FILES
    AzNetworkingModule.cpp
    AzNetworkingModule.h
    AutoGen/AutoPacketDispatcher_Header.jinja
    AutoGen/AutoPacketDispatcher_Inline.jinja
    AutoGen/AutoPackets_Header.jinja
    AutoGen/AutoPackets_Inline.jinja
    AutoGen/AutoPackets_Source.jinja
    AutoGen/CorePackets.AutoPackets.xml
    ConnectionLayer/ConnectionEnums.h
    ConnectionLayer/ConnectionMetrics.cpp
    ConnectionLayer/ConnectionMetrics.h
    ConnectionLayer/ConnectionMetrics.inl
    ConnectionLayer/IConnection.h
    ConnectionLayer/IConnection.inl
    ConnectionLayer/IConnectionListener.h
    ConnectionLayer/IConnectionSet.h
    ConnectionLayer/SequenceGenerator.h
    ConnectionLayer/SequenceGenerator.inl
    DataStructures/ByteBuffer.h
    DataStructures/ByteBuffer.inl
    DataStructures/FixedSizeBitset.h
    DataStructures/FixedSizeBitset.inl
    DataStructures/FixedSizeBitsetView.h
    DataStructures/FixedSizeBitsetView.inl
    DataStructures/FixedSizeVectorBitset.h
    DataStructures/FixedSizeVectorBitset.inl
    DataStructures/IBitset.h
    DataStructures/RingBufferBitset.h
    DataStructures/RingBufferBitset.inl
    DataStructures/TimeoutQueue.cpp
    DataStructures/TimeoutQueue.h
    DataStructures/TimeoutQueue.inl
    Framework/ICompressor.h
    Framework/INetworking.h
    Framework/INetworkInterface.h
    Framework/NetworkingSystemComponent.cpp
    Framework/NetworkingSystemComponent.h
    Framework/NetworkInterfaceMetrics.h
    PacketLayer/IPacket.h
    PacketLayer/IPacketHeader.h
    Serialization/AbstractValue.h
    Serialization/AzContainerSerializers.h
    Serialization/DeltaSerializer.cpp
    Serialization/DeltaSerializer.h
    Serialization/DeltaSerializer.inl
    Serialization/HashSerializer.cpp
    Serialization/HashSerializer.h
    Serialization/ISerializer.h
    Serialization/ISerializer.inl
    Serialization/NetworkInputSerializer.cpp
    Serialization/NetworkInputSerializer.h
    Serialization/NetworkInputSerializer.inl
    Serialization/NetworkOutputSerializer.cpp
    Serialization/NetworkOutputSerializer.h
    Serialization/NetworkOutputSerializer.inl
    Serialization/TrackChangedSerializer.h
    Serialization/TrackChangedSerializer.inl
    TcpTransport/TcpConnection.cpp
    TcpTransport/TcpConnection.h
    TcpTransport/TcpConnection.inl
    TcpTransport/TcpConnectionSet.cpp
    TcpTransport/TcpConnectionSet.h
    TcpTransport/TcpPacketHeader.cpp
    TcpTransport/TcpPacketHeader.h
    TcpTransport/TcpPacketHeader.inl
    TcpTransport/TcpRingBuffer.h
    TcpTransport/TcpRingBuffer.inl
    TcpTransport/TcpRingBufferImpl.cpp
    TcpTransport/TcpRingBufferImpl.h
    TcpTransport/TcpRingBufferImpl.inl
    TcpTransport/TcpSocket.cpp
    TcpTransport/TcpSocket.h
    TcpTransport/TcpSocket.inl
    TcpTransport/TcpSocketManager.h
    TcpTransport/TcpSocketManager_Epoll.cpp
    TcpTransport/TcpSocketManager_None.cpp
    TcpTransport/TcpSocketManager_Select.cpp
    TcpTransport/TlsSocket.cpp
    TcpTransport/TlsSocket.h
    TcpTransport/TcpListenThread.cpp
    TcpTransport/TcpListenThread.h
    TcpTransport/TcpNetworkInterface.cpp
    TcpTransport/TcpNetworkInterface.h
    UdpTransport/DtlsEndpoint.cpp
    UdpTransport/DtlsEndpoint.h
    UdpTransport/DtlsSocket.cpp
    UdpTransport/DtlsSocket.h
    UdpTransport/UdpConnection.cpp
    UdpTransport/UdpConnection.h
    UdpTransport/UdpConnection.inl
    UdpTransport/UdpConnectionSet.cpp
    UdpTransport/UdpConnectionSet.h
    UdpTransport/UdpFragmentQueue.cpp
    UdpTransport/UdpFragmentQueue.h
    UdpTransport/UdpNetworkInterface.cpp
    UdpTransport/UdpNetworkInterface.h
    UdpTransport/UdpPacketHeader.cpp
    UdpTransport/UdpPacketHeader.h
    UdpTransport/UdpPacketHeader.inl
    UdpTransport/UdpPacketIdWindow.cpp
    UdpTransport/UdpPacketIdWindow.h
    UdpTransport/UdpPacketIdWindow.inl
    UdpTransport/UdpPacketTracker.cpp
    UdpTransport/UdpPacketTracker.h
    UdpTransport/UdpPacketTracker.inl
    UdpTransport/UdpReaderThread.cpp
    UdpTransport/UdpReaderThread.h
    UdpTransport/UdpReliableQueue.cpp
    UdpTransport/UdpReliableQueue.h
    UdpTransport/UdpSocket.cpp
    UdpTransport/UdpSocket.h
    UdpTransport/UdpSocket.inl
    Utilities/CidrAddress.cpp
    Utilities/CidrAddress.h
    Utilities/CompressionCommon.cpp
    Utilities/CompressionCommon.h
    Utilities/EncryptionCommon.cpp
    Utilities/EncryptionCommon.h
    Utilities/Endian.h
    Utilities/IpAddress.cpp
    Utilities/IpAddress.h
    Utilities/IpAddress.inl
    Utilities/NetworkCommon.cpp
    Utilities/NetworkCommon.h
    Utilities/NetworkCommon.inl
    Utilities/NetworkIncludes.h
    Utilities/QuantizedValues.h
    Utilities/QuantizedValues.inl
    Utilities/TimedThread.cpp
    Utilities/TimedThread.h
)