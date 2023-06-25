#include "server/SocketClient.hpp"
#include <cstdlib>
#include <cstring>
#include <google/protobuf/util/json_util.h>
#include <basis/seadNew.h>

#include "SocketBase.hpp"
#include "al/async/FunctorV0M.hpp"
#include "logger.hpp"
#include "nn/result.h"
#include "nn/socket.h"
#include "packets/Packet.hpp"
#include "server/Client.hpp"
#include "thread/seadMessageQueue.h"
#include "types.h"

namespace {
    std::string packetToJson(packets::Packet const& packet) {
        std::string jsonString;
        google::protobuf::util::JsonPrintOptions options;
        options.add_whitespace = false;
        options.always_print_primitive_fields = true;
        options.preserve_proto_field_names = true;
        MessageToJsonString(packet, &jsonString, options);
        return jsonString;
    }
}

SocketClient::SocketClient(const char* name, sead::Heap* heap, Client* client) : mHeap(heap), SocketBase(name) {

    this->client = client;

    mRecvThread = new al::AsyncFunctorThread("SocketRecvThread", al::FunctorV0M<SocketClient*, SocketThreadFunc>(this, &SocketClient::recvFunc), 0, 0x1000, {0});
    mSendThread = new al::AsyncFunctorThread("SocketSendThread", al::FunctorV0M<SocketClient*, SocketThreadFunc>(this, &SocketClient::sendFunc), 0, 0x1000, {0});
    
    mRecvQueue.allocate(maxBufSize, mHeap);
    mSendQueue.allocate(maxBufSize, mHeap);
};

nn::Result SocketClient::init(const char* ip, u16 port) {

    this->sock_ip = ip;
    this->port    = port;
    
    in_addr  hostAddress   = { 0 };
    sockaddr serverAddress = { 0 };

    Logger::log("SocketClient::init: %s:%d sock %s\n", ip, port, getStateChar());

    nn::nifm::Initialize();
    nn::nifm::SubmitNetworkRequest();

    while (nn::nifm::IsNetworkRequestOnHold()) { }

    // emulators (ryujinx) make this return false always, so skip it during init
    #if EMU == 0
    if (!nn::nifm::IsNetworkAvailable()) {
        Logger::log("Network Unavailable.\n");
        this->socket_log_state = SOCKET_LOG_UNAVAILABLE;
        this->socket_errno = nn::socket::GetLastErrno();
        return -1;
    }
    #endif

    if ((this->socket_log_socket = nn::socket::Socket(2, 1, 6)) < 0) {
        Logger::log("Socket Unavailable.\n");
        this->socket_errno = nn::socket::GetLastErrno();
        this->socket_log_state = SOCKET_LOG_UNAVAILABLE;
        return -1;
    }

    if (! this->stringToIPAddress(this->sock_ip, &hostAddress)) {
        Logger::log("IP address is invalid or hostname not resolveable.\n");
        this->socket_errno = nn::socket::GetLastErrno();
        this->socket_log_state = SOCKET_LOG_UNAVAILABLE;
        return -1;
    }

    serverAddress.address = hostAddress;
    serverAddress.port = nn::socket::InetHtons(this->port);
    serverAddress.family = 2;

    int sockOptValue = true;

    nn::socket::SetSockOpt(this->socket_log_socket, 6, TCP_NODELAY, &sockOptValue, sizeof(sockOptValue));

    nn::Result result;
    
    if((result = nn::socket::Connect(this->socket_log_socket, &serverAddress, sizeof(serverAddress))).isFailure()) {
        Logger::log("Socket Connection Failed!\n");
        this->socket_errno = nn::socket::GetLastErrno();
        this->socket_log_state = SOCKET_LOG_UNAVAILABLE;
        return result;
    }

    this->socket_log_state = SOCKET_LOG_CONNECTED;

    Logger::log("Socket fd: %d\n", socket_log_socket);

    startThreads();  // start recv and send threads after sucessful connection

    // send init packet to server once we connect (an issue with the server prevents this from working properly, waiting for a fix to implement)
    
    packets::system::PlayerConnect initPacket;
    initPacket.set_client_name(packets::convert(Client::getUsername()));

    if (mIsFirstConnect) {
        initPacket.set_con_type(packets::system::ConnectionTypes::INIT);
        mIsFirstConnect = false;
    } else {
        initPacket.set_con_type(packets::system::ConnectionTypes::RECONNECT);
    }

    sead::ScopedCurrentHeapSetter setter(mHeap);

    packets::system::SystemPacket systemPacket;
    *systemPacket.mutable_player_connect() = initPacket;
    packets::Packet* packet = new packets::Packet();
    *packet->mutable_system_packet() = systemPacket;
    *packet->mutable_metadata()->mutable_user_id() = packets::convert(Client::getClientId());


    send(packet);

    // on a reconnect, resend some maybe missing packets
    if (initPacket.con_type() == packets::system::ConnectionTypes::RECONNECT) {
      client->resendInitPackets();
    }

    return result;

}

bool SocketClient::send(packets::Packet *packet) {

    if (this->socket_log_state != SOCKET_LOG_CONNECTED)
        return false;

    auto json = packetToJson(*packet);

    // if (packet->mType != PLAYERINF && packet->mType != HACKCAPINF)
    //     Logger::log("Sending packet: %s\n", json.c_str());

    int32_t packetLength = json.size();

    auto sendData = [this](char const* data, size_t size) {
        int valread = 0;
        if ((valread = nn::socket::Send(this->socket_log_socket, data, size, 0) > 0)) {
            return true;
        } else {
            Logger::log("Failed to Fully Send Packet! Result: %d Send Size: %d\n", valread, size);
            this->socket_errno = nn::socket::GetLastErrno();
            this->tryReconnect();
            return false;
        }
        return true;
    };

    auto res = sendData((char*)&packetLength, sizeof(packetLength));
    if (!res) {
        return false;
    }
    res = sendData(json.c_str(), json.size());
    return res;
}

bool SocketClient::recv() {

    if (this->socket_log_state != SOCKET_LOG_CONNECTED) {
        Logger::log("Unable To Receive! Socket Not Connected.\n");
        this->socket_errno = nn::socket::GetLastErrno();
        return this->tryReconnect();
    }
    
    int32_t packetSize = 0;
    constexpr int headerSize = sizeof(packetSize);
    int valread = 0;

    // read only the size of a header
    while(valread < headerSize) {
        int result = nn::socket::Recv(this->socket_log_socket, ((char*)&packetSize) + valread,
                                      headerSize - valread, this->sock_flags);

        this->socket_errno = nn::socket::GetLastErrno();
        
        if(result > 0) {
            valread += result;
        } else {
            if(this->socket_errno==11){
                return true;
            } else {
                Logger::log("Header Read Failed! Value: %d Total Read: %d\n", result, valread);
                return this->tryReconnect(); // if we sucessfully reconnect, we dont want 
            }
        }
    }

    if(valread <= 0) {  // if we error'd, close the socket
        Logger::log("valread was zero! Disconnecting.\n");
        this->socket_errno = nn::socket::GetLastErrno();
        return this->tryReconnect();
    }

    // if (header->mType != PLAYERINF && header->mType != HACKCAPINF) {
    //     Logger::log("Received packet (from %02X%02X):", header->mUserID.data[0],
    //                 header->mUserID.data[1]);
    //     Logger::disableName();
    //     Logger::log(" Size: %d", header->mPacketSize);
    //     Logger::log(" Type: %d", header->mType);
    //     if(packetNames[header->mType])
    //         Logger::log(" Type String: %s\n", packetNames[header->mType]);
    //     Logger::enableName();
    // }

    char* packetBuf = (char*)mHeap->alloc(packetSize);

    if (!packetBuf) {
        return true;
    }

    while (valread < packetSize) {
        int result = nn::socket::Recv(this->socket_log_socket, packetBuf + valread,
                                        packetSize - valread, this->sock_flags);

        this->socket_errno = nn::socket::GetLastErrno();

        if (result > 0) {
            valread += result;
        } else {
            mHeap->free(packetBuf);
            Logger::log("Packet Read Failed! Value: %d\nPacket Size: %d\n", result, packetSize);
            return this->tryReconnect();
        }
    }

    std::string packetJson(packetBuf, packetSize);
    mHeap->free(packetBuf);

    packets::Packet* packet = (packets::Packet*)mHeap->alloc(sizeof(packets::Packet));
    if (!packet) {
        return false;
    }
    *packet = packets::Packet();
    google::protobuf::util::JsonParseOptions jsonParseOptions;
    JsonStringToMessage(packetJson, packet, jsonParseOptions);

    if (!mRecvQueue.isFull()) {
        mRecvQueue.push((s64)packet, sead::MessageQueue::BlockType::NonBlocking);
    } else {
        mHeap->free(packet);
    }
    
    return true;
}

// prints packet to debug logger
void SocketClient::printPacket(packets::Packet *packet) {
    Logger::log("Packet: %s\n", packetToJson(*packet).c_str());
}

bool SocketClient::tryReconnect() {

    Logger::log("Attempting to Reconnect.\n");

    if (closeSocket()) { // unfortunately we cannot use the same fd from the previous connection, so close the socket entirely and attempt a new connection.
        if (init(sock_ip, port).isSuccess()) { // call init again
            Logger::log("Reconnect Successful.\n");
            return true;
        }
    }

    return false;
}

bool SocketClient::closeSocket() {

    Logger::log("Closing Socket.\n");

    bool result = false;

    if (!(result = SocketBase::closeSocket())) {
        Logger::log("Failed to close socket!\n");
    }

    return result;
}

bool SocketClient::stringToIPAddress(const char* str, in_addr* out) {
    // string to IPv4
    if (nn::socket::InetAton(str, out)) {
        return true;
    }

    // get IPs via DNS
    struct hostent *he = nn::socket::GetHostByName(str);
    if (! he) { return false; }

    // might give us multiple IP addresses, so pick the first one
    struct in_addr **addr_list = (struct in_addr **) he->h_addr_list;
    for (int i = 0 ; addr_list[i] != NULL ; i++) {
        *out = *addr_list[i];
        return true;
    }

    return false;
}

/**
 * @brief starts client read thread
 * 
 * @return true if read thread was sucessfully started
 * @return false if read thread was unable to start, or thread was already started.
 */
bool SocketClient::startThreads() {

    Logger::log("Recv Thread isDone: %s\n", BTOC(this->mRecvThread->isDone()));
    Logger::log("Send Thread isDone: %s\n", BTOC(this->mSendThread->isDone()));

    if(this->mRecvThread->isDone() && this->mSendThread->isDone()) {
        this->mRecvThread->start();
        this->mSendThread->start();
        Logger::log("Socket threads sucessfully started.\n");
        return true;
    }else {
        Logger::log("Socket threads failed to start.\n");
        return false;
    }
}

void SocketClient::endThreads() {
    mRecvThread->mDelegateThread->destroy();
    mSendThread->mDelegateThread->destroy();
}

void SocketClient::sendFunc() {

    Logger::log("Starting Send Thread.\n");

    while (true) {
        trySendQueue();
    }

    Logger::log("Ending Send Thread.\n");
}

void SocketClient::recvFunc() {

    nn::socket::Recv(this->socket_log_socket, nullptr, 0, 0);

    Logger::log("Starting Recv Thread.\n");

    while (true) {
        if (!recv()) {
            Logger::log("Receiving Packet Failed!\n");
        }
    }

    Logger::log("Ending Recv Thread.\n");
}

bool SocketClient::queuePacket(packets::Packet* packet) {
    if (socket_log_state == SOCKET_LOG_CONNECTED && !mSendQueue.isFull()) {
        mSendQueue.push((s64)packet,
                        sead::MessageQueue::BlockType::NonBlocking);  // as this is non-blocking, it
                                                                      // will always return true.
        return true;
    } else {
        mHeap->free(packet);
        return false;
    }
}

void SocketClient::trySendQueue() {

    packets::Packet* curPacket = (packets::Packet*)mSendQueue.pop(sead::MessageQueue::BlockType::Blocking);

    send(curPacket);

    mHeap->free(curPacket);
}

packets::Packet* SocketClient::tryGetPacket(sead::MessageQueue::BlockType blockType) {
    return socket_log_state == SOCKET_LOG_CONNECTED ? (packets::Packet*)mRecvQueue.pop(blockType) : nullptr;
}
