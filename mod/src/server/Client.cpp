#include "server/Client.hpp"

#include <google/protobuf/util/message_differencer.h>

#include "al/layout/SimpleLayoutAppearWaitEnd.h"
#include "al/util/LiveActorUtil.h"
#include "game/SaveData/SaveDataAccessFunction.h"
#include "heap/seadHeapMgr.h"
#include "logger.hpp"
#include "packets/Packet.hpp"
#include "server/hns/HideAndSeekMode.hpp"
#include "server/capturetheflag/CaptureTheFlagMode.hpp"
#include "server/capturetheflag/CaptureTheFlagConfigMenu.hpp"
#include "actors/FlagActor.h"

SEAD_SINGLETON_DISPOSER_IMPL(Client)

typedef void (Client::*ClientThreadFunc)(void);

/**
 * @brief Construct a new Client:: Client object
 * 
 * @param bufferSize defines the maximum amount of puppets the client can handle
 */
Client::Client() {

    mHeap = sead::ExpHeap::create(0x50000, "ClientHeap", sead::HeapMgr::instance()->getCurrentHeap(), 8, sead::Heap::cHeapDirection_Forward, false);

    sead::ScopedCurrentHeapSetter heapSetter(
        mHeap);  // every new call after this will use ClientHeap instead of SequenceHeap

    mReadThread = new al::AsyncFunctorThread("ClientReadThread", al::FunctorV0M<Client*, ClientThreadFunc>(this, &Client::readFunc), 0, 0x1000, {0});

    mKeyboard = new Keyboard(nn::swkbd::GetRequiredStringBufferSize());

    mSocket = new SocketClient("SocketClient", mHeap, this);
    
    mPuppetHolder = new PuppetHolder(maxPuppets);

    for (size_t i = 0; i < MAXPUPINDEX; i++)
    {
        mPuppetInfoArr[i] = new PuppetInfo();

        sprintf(mPuppetInfoArr[i]->puppetName, "Puppet%zu", i);
    }

    strcpy(mDebugPuppetInfo.puppetName, "PuppetDebug");

    mConnectCount = 0;

    curCollectedShines.fill(-1);

    collectedShineCount = 0;

    mShineArray.allocBuffer(100, nullptr); // max of 100 shine actors in buffer

    nn::account::GetLastOpenedUser(&mUserID);

    nn::account::Nickname playerName;
    nn::account::GetNickname(&playerName, mUserID);
    Logger::setLogName(playerName.name);  // set Debug logger name to player name

    mUsername = playerName.name;
    
    mUserID.print();

    Logger::log("Player Name: %s\n", playerName.name);

    Logger::log("%s Build Number: %s\n", playerName.name, TOSTRING(BUILDVERSTR));

}

/**
 * @brief initializes client class using initInfo obtained from StageScene::init
 * 
 * @param initInfo init info used to create layouts used by client
 */
void Client::init(al::LayoutInitInfo const &initInfo, GameDataHolderAccessor holder) {

    mConnectionWait = new (mHeap) al::WindowConfirmWait("ServerWaitConnect", "WindowConfirmWait", initInfo);
    
    mConnectStatus = new (mHeap) al::SimpleLayoutAppearWaitEnd("", "SaveMessage", initInfo, 0, false);

    mConnectionWait->setTxtMessage(u"Connecting to Server.");
    mConnectionWait->setTxtMessageConfirm(u"Failed to Connect!");

    al::setPaneString(mConnectStatus, "TxtSave", u"Connecting to Server.", 0);
    al::setPaneString(mConnectStatus, "TxtSaveSh", u"Connecting to Server.", 0);

    mHolder = holder;

    startThread();

    Logger::log("Heap Free Size: %f/%f\n", mHeap->getFreeSize() * 0.001f, mHeap->getSize() * 0.001f);
}

/**
 * @brief starts client read thread
 * 
 * @return true if read thread was sucessfully started
 * @return false if read thread was unable to start, or thread was already started.
 */
bool Client::startThread() {
    if(mReadThread->isDone() ) {
        mReadThread->start();
        Logger::log("Read Thread Sucessfully Started.\n");
        return true;
    }else {
        Logger::log("Read Thread has already started! Or other unknown reason.\n");
        return false;
    }
}

/**
 * @brief restarts currently active connection to server
 * 
 */
void Client::restartConnection() {

    if (!sInstance) {
        Logger::log("Static Instance is null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    Logger::log("Sending Disconnect.\n");

    packets::system::SystemPacket systemPacket;
    *systemPacket.mutable_player_dc() = packets::system::PlayerDC();
    packets::Packet* packet = new packets::Packet();
    *packet->mutable_metadata()->mutable_user_id() = packets::convert(sInstance->mUserID);
    *packet->mutable_system_packet() = systemPacket;

    sInstance->mSocket->queuePacket(packet);

    if (sInstance->mSocket->closeSocket()) {
        Logger::log("Sucessfully Closed Socket.\n");
    }

    sInstance->mConnectCount = 0;

    sInstance->mIsConnectionActive = sInstance->mSocket->init(sInstance->mServerIP.cstr(), sInstance->mServerPort).isSuccess();

    if(sInstance->mSocket->getLogState() == SOCKET_LOG_CONNECTED) {

        Logger::log("Reconnect Sucessful!\n");

    } else {
        Logger::log("Reconnect Unsuccessful.\n");
    }
}

/**
 * @brief starts a connection using client's TCP socket class, pulling up the software keyboard for user inputted IP if save file does not have one saved.
 * 
 * @return true if successful connection to server
 * @return false if connection was unable to establish
 */
bool Client::startConnection() {

    bool isNeedSave = false;

    bool isOverride = al::isPadHoldZL(-1);

    if (mServerIP.isEmpty() || isOverride) {
        mKeyboard->setHeaderText(u"Save File does not contain an IP!");
        mKeyboard->setSubText(u"Please set a Server IP Below.");
        mServerIP = "127.0.0.1";
        Client::openKeyboardIP();
        isNeedSave = true;
    }

    if (!mServerPort || isOverride) {
        mKeyboard->setHeaderText(u"Save File does not contain a port!");
        mKeyboard->setSubText(u"Please set a Server Port Below.");
        mServerPort = 1027;
        Client::openKeyboardPort();
        isNeedSave = true;
    }

    if (isNeedSave) {
        SaveDataAccessFunction::startSaveDataWrite(mHolder.mData);
    }

    mIsConnectionActive = mSocket->init(mServerIP.cstr(), mServerPort).isSuccess();

    if (mIsConnectionActive) {

        Logger::log("Sucessful Connection. Waiting to recieve init packet.\n");

        bool waitingForInitPacket = true;
        // wait for client init packet

        while (waitingForInitPacket) {

            packets::Packet *curPacket = mSocket->tryGetPacket();

            if (curPacket) {
                
                if (curPacket->has_system_packet() && curPacket->system_packet().has_init()) {
                    packets::system::Init const& initPacket = curPacket->system_packet().init();

                    Logger::log("Server Max Player Size: %d\n", initPacket.max_players());

                    maxPuppets = initPacket.max_players() - 1;

                    waitingForInitPacket = false;
                }

                mHeap->free(curPacket);

            } else {
                Logger::log("Recieve failed! Stopping Connection.\n");
                mIsConnectionActive = false;
                waitingForInitPacket = false;
            }
        }
    }

    return mIsConnectionActive;
}

/**
 * @brief Opens up OS's software keyboard in order to change the currently used server IP.
 * @returns whether or not a new IP has been defined and needs to be saved.
 */
bool Client::openKeyboardIP() {

    if (!sInstance) {
        Logger::log("Static Instance is null!\n");
        return false;
    }

    // opens swkbd with the initial text set to the last saved IP
    sInstance->mKeyboard->openKeyboard(
        sInstance->mServerIP.cstr(), [](nn::swkbd::KeyboardConfig& config) {
            config.keyboardMode = nn::swkbd::KeyboardMode::ModeASCII;
            config.textMaxLength = MAX_HOSTNAME_LENGTH;
            config.textMinLength = 1;
            config.isUseUtf8 = true;
            config.inputFormMode = nn::swkbd::InputFormMode::OneLine;
        });

    hostname prevIp = sInstance->mServerIP;

    while (true) {
        if (sInstance->mKeyboard->isThreadDone()) {
            if(!sInstance->mKeyboard->isKeyboardCancelled())
                sInstance->mServerIP = sInstance->mKeyboard->getResult();
            break;
        }
        nn::os::YieldThread(); // allow other threads to run
    }

    bool isFirstConnect = prevIp != sInstance->mServerIP;

    sInstance->mSocket->setIsFirstConn(isFirstConnect);

    return isFirstConnect;
}

/**
 * @brief Opens up OS's software keyboard in order to change the currently used server port.
 * @returns whether or not a new port has been defined and needs to be saved.
 */
bool Client::openKeyboardPort() {

    if (!sInstance) {
        Logger::log("Static Instance is null!\n");
        return false;
    }

    // opens swkbd with the initial text set to the last saved port
    char buf[6];
    nn::util::SNPrintf(buf, 6, "%u", sInstance->mServerPort);

    sInstance->mKeyboard->openKeyboard(buf, [](nn::swkbd::KeyboardConfig& config) {
        config.keyboardMode = nn::swkbd::KeyboardMode::ModeNumeric;
        config.textMaxLength = 5;
        config.textMinLength = 2;
        config.isUseUtf8 = true;
        config.inputFormMode = nn::swkbd::InputFormMode::OneLine;
    });

    int prevPort = sInstance->mServerPort;

    while (true) {
        if (sInstance->mKeyboard->isThreadDone()) {
            if(!sInstance->mKeyboard->isKeyboardCancelled())
                sInstance->mServerPort = ::atoi(sInstance->mKeyboard->getResult());
            break;
        }
        nn::os::YieldThread(); // allow other threads to run
    }

    bool isFirstConnect = prevPort != sInstance->mServerPort;

    sInstance->mSocket->setIsFirstConn(isFirstConnect);

    return isFirstConnect;
}

/**
 * @brief main thread function for read thread, responsible for processing packets from server
 * 
 */
void Client::readFunc() {

    if (waitForGameInit) {
        nn::os::YieldThread(); // sleep the thread for the first thing we do so that game init can finish
        nn::os::SleepThread(nn::TimeSpan::FromSeconds(2));
        waitForGameInit = false;
    }

    mConnectStatus->appear();

    al::startAction(mConnectStatus, "Loop", "Loop");

    if (!startConnection()) {

        Logger::log("Failed to Connect to Server.\n");

        nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(250000000)); // sleep active thread for 0.25 seconds

        mConnectStatus->end();
                
        return;
    }

    nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(500000000)); // sleep for 0.5 seconds to let connection layout fully show (probably should find a better way to do this)

    mConnectStatus->end();

    while(mIsConnectionActive) {

        packets::Packet *curPacket = mSocket->tryGetPacket();  // will block until a packet has been recieved, or socket disconnected

        if (!curPacket) { // if false, socket has errored or disconnected, so close the socket and end this thread.
            Logger::log("Client Socket Encountered an Error! Errno: 0x%x\n", mSocket->socket_errno);
        }

        auto const& packetMetadata = curPacket->metadata();

        if (curPacket->has_system_packet()) {
            auto const& systemPacket = curPacket->system_packet();
            if (systemPacket.has_player_inf()) {
                updatePlayerInfo(packetMetadata, systemPacket.player_inf());
            }
            else if (systemPacket.has_game_inf()) {
                updateGameInfo(packetMetadata, systemPacket.game_inf());
            }
            else if (systemPacket.has_player_inf()) {
                updatePlayerInfo(packetMetadata, systemPacket.player_inf());
            }
            else if (systemPacket.has_hack_cap_inf()) {
                updateHackCapInfo(packetMetadata, systemPacket.hack_cap_inf());
            }
            else if (systemPacket.has_capture_inf()) {
                updateCaptureInfo(packetMetadata, systemPacket.capture_inf());
            }
            else if (systemPacket.has_player_connect()) {
                updatePlayerConnect(packetMetadata, systemPacket.player_connect());

                // Send relevant info packets when another client is connected

                if (!google::protobuf::util::MessageDifferencer::Equals(lastGameInfPacket, emptyGameInfPacket)) {
                    // Assume game packets are empty from first connection
                    if (packets::convert(lastGameInfPacket.metadata().user_id()) != mUserID) {
                        *lastGameInfPacket.mutable_metadata()->mutable_user_id() = packets::convert(mUserID);
                    }
                    mSocket->send(&lastGameInfPacket);
                }

                // No need to send player/costume packets if they're empty
                if (packets::convert(lastPlayerInfPacket.metadata().user_id()) == mUserID) {
                    mSocket->send(&lastPlayerInfPacket);
                }
                if (packets::convert(lastCostumeInfPacket.metadata().user_id()) == mUserID) {
                    mSocket->send(&lastCostumeInfPacket);
                }
            }
            else if (systemPacket.has_costume_inf()) {
                updateCostumeInfo(packetMetadata, systemPacket.costume_inf());
            }
            else if (systemPacket.has_shine_collect()) {
                updateShineInfo(packetMetadata, systemPacket.shine_collect());
            }
            else if (systemPacket.has_player_dc()) {
                Logger::log("Received Player Disconnect!\n");
                packets::convert(packetMetadata.user_id()).print();
                disconnectPlayer(packetMetadata, systemPacket.player_dc());
            }
            else if (systemPacket.has_tag_inf()) {
                updateTagInfo(packetMetadata, systemPacket.tag_inf());
            }
            else if (systemPacket.has_change_stage()) {
                sendToStage(packetMetadata, systemPacket.change_stage());
            }
            else if (systemPacket.has_init()) {
                packets::system::Init const& initPacket = systemPacket.init();
                Logger::log("Server Max Player Size: %d\n", initPacket.max_players());
                maxPuppets = initPacket.max_players() - 1;
            }
            else {
                Logger::log("Discarding Unknown Packet Type.\n");
            }
        }
        else if (curPacket->has_ctf_packet()) {
            packets::ctf::CTFPacket const& ctfPacket = curPacket->ctf_packet();
            if (ctfPacket.has_player_team_assignment()) {
                updateCTFPlayerTeamAssignment(packetMetadata, ctfPacket.player_team_assignment());
            }
            else if (ctfPacket.has_game_state()) {
                updateCTFGameState(packetMetadata, ctfPacket.game_state());
            }
            else if (ctfPacket.has_flag_hold_state()) {
                updateCTFFlagHoldState(packetMetadata, ctfPacket.flag_hold_state());
            }
            else if (ctfPacket.has_flag_dropped_position()) {
                updateCTFFlagDroppedPosition(packetMetadata, ctfPacket.flag_dropped_position());
            }
            else if (ctfPacket.has_win()) {
                updateCTFWinState(packetMetadata, ctfPacket.win());
            }
            else {
                Logger::log("Discarding Unknown Packet Type.\n");
            }
        }
        else {
            Logger::log("Discarding Unknown Packet Type.\n");
        }

        mHeap->free(curPacket);
    }

    Logger::log("Client Read Thread ending.\n");
}

/**
 * @brief sends player info packet to current server
 * 
 * @param player pointer to current player class, used to get translation, animation, and capture data
 */
void Client::sendPlayerInfPacket(const PlayerActorBase *playerBase, bool isYukimaru) {

    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }
    
    if(!playerBase) {
        Logger::log("Error: Null Player Reference\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    packets::system::PlayerInf playerInfPacket;

    *playerInfPacket.mutable_player_pos() = packets::convert(al::getTrans(playerBase));

    sead::Quatf playerRot;
    al::calcQuat(&playerRot,
                 playerBase);  // calculate rotation based off pose instead of using quat rotation
    *playerInfPacket.mutable_player_rot() = packets::convert(playerRot);

    constexpr uint32_t ANIM_BLEND_WEIGHT_COUNT = 6;
    if (!isYukimaru) { 
        
        PlayerActorHakoniwa* player = (PlayerActorHakoniwa*)playerBase;

        playerInfPacket.mutable_anim_blend_weights()->Resize(ANIM_BLEND_WEIGHT_COUNT, 0.0f);
        for (size_t i = 0; i < ANIM_BLEND_WEIGHT_COUNT; i++)
        {
            playerInfPacket.mutable_anim_blend_weights()->at(i) = player->mPlayerAnimator->getBlendWeight(i);
        }

        const char *hackName = player->mHackKeeper->getCurrentHackName();

        if (hackName != nullptr) {

            sInstance->isClientCaptured = true;

            const char* actName = al::getActionName(player->mHackKeeper->currentHackActor);

            if (actName) {
                playerInfPacket.set_act_name(packets::convert(PlayerAnims::FindType(actName)));
                playerInfPacket.set_sub_act_name(packets::convert(PlayerAnims::Type::Unknown));
                //strcpy(packet.actName, actName); 
            } else {
                playerInfPacket.set_act_name(packets::convert(PlayerAnims::Type::Unknown));
                playerInfPacket.set_sub_act_name(packets::convert(PlayerAnims::Type::Unknown));
            }
        } else {
            playerInfPacket.set_act_name(packets::convert(PlayerAnims::FindType(player->mPlayerAnimator->mAnimFrameCtrl->getActionName())));
            playerInfPacket.set_sub_act_name(packets::convert(PlayerAnims::FindType(player->mPlayerAnimator->curSubAnim.cstr())));

            sInstance->isClientCaptured = false;
        }

    } else {

        // TODO: implement YukimaruRacePlayer syncing
        playerInfPacket.mutable_anim_blend_weights()->Resize(ANIM_BLEND_WEIGHT_COUNT, 0.0f);

        sInstance->isClientCaptured = false;

        playerInfPacket.set_act_name(packets::convert(PlayerAnims::Type::Unknown));
        playerInfPacket.set_sub_act_name(packets::convert(PlayerAnims::Type::Unknown));
    }

    packets::system::SystemPacket systemPacket;
    *systemPacket.mutable_player_inf() = playerInfPacket;
    packets::Packet* packet = new packets::Packet();
    *packet->mutable_system_packet() = systemPacket;
    *packet->mutable_metadata()->mutable_user_id() = packets::convert(sInstance->mUserID);

    
    if(!google::protobuf::util::MessageDifferencer::Equals(sInstance->lastPlayerInfPacket, *packet)) {
        sInstance->lastPlayerInfPacket = *packet; // deref packet and store in client memory
        sInstance->mSocket->queuePacket(packet);
    } else {
        sInstance->mHeap->free(packet); // free packet if we're not using it
    }

}

/**
 * @brief sends info related to player's cap actor to server
 * 
 * @param hackCap pointer to cap actor, used to get translation, animation, and state info
 */
void Client::sendHackCapInfPacket(const HackCap* hackCap) {

    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);
    
    bool isFlying = hackCap->isFlying();

    // if cap is in flying state, send packet as often as this function is called
    if (isFlying) {
        packets::system::HackCapInf hackCapInfPacket;

        *hackCapInfPacket.mutable_cap_pos() = packets::convert(al::getTrans(hackCap));

        hackCapInfPacket.mutable_is_cap_visible()->set_value(isFlying);

        hackCapInfPacket.mutable_cap_quat()->set_x(hackCap->mJointKeeper->mJointRot.x);
        hackCapInfPacket.mutable_cap_quat()->set_y(hackCap->mJointKeeper->mJointRot.y);
        hackCapInfPacket.mutable_cap_quat()->set_z(hackCap->mJointKeeper->mJointRot.z);
        hackCapInfPacket.mutable_cap_quat()->set_w(hackCap->mJointKeeper->mSkew);

        hackCapInfPacket.set_cap_anim(std::string(al::getActionName(hackCap)));

        packets::system::SystemPacket systemPacket;
        *systemPacket.mutable_hack_cap_inf() = hackCapInfPacket;
        packets::Packet* packet = new packets::Packet();
        *packet->mutable_system_packet() = systemPacket;
        *packet->mutable_metadata()->mutable_user_id() = packets::convert(sInstance->mUserID);

        sInstance->mSocket->queuePacket(packet);

        sInstance->isSentHackInf = true;

    } else if (sInstance->isSentHackInf) { // if cap is not flying, check to see if previous function call sent a packet, and if so, send one final packet resetting cap data.
        packets::system::HackCapInf hackCapInfPacket;
        hackCapInfPacket.mutable_is_cap_visible()->set_value(false);
        *hackCapInfPacket.mutable_cap_pos() = packets::convert(sead::Vector3f::zero);
        *hackCapInfPacket.mutable_cap_quat() = packets::convert(sead::Quatf::unit);

        packets::system::SystemPacket systemPacket;
        *systemPacket.mutable_hack_cap_inf() = hackCapInfPacket;
        packets::Packet* packet = new packets::Packet();
        *packet->mutable_system_packet() = systemPacket;
        *packet->mutable_metadata()->mutable_user_id() = packets::convert(sInstance->mUserID);


        sInstance->mSocket->queuePacket(packet);
        sInstance->isSentHackInf = false;
    }
}

/**
 * @brief 
 * Sends both stage info and player 2D info to the server.
 * @param player 
 * @param holder 
 */
void Client::sendGameInfPacket(const PlayerActorHakoniwa* player, GameDataHolderAccessor holder) {

    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);
    
    packets::system::GameInf gameInfPacket;

    if (player) {
        gameInfPacket.mutable_is_2d()->set_value(player->mDimKeeper->is2DModel);
    } else {
        gameInfPacket.mutable_is_2d()->set_value(false);
    }

    gameInfPacket.set_scenario_num(holder.mData->mGameDataFile->getScenarioNo());

    gameInfPacket.set_stage_name(std::string(GameDataFunction::getCurrentStageName(holder)));

    packets::system::SystemPacket systemPacket;
    *systemPacket.mutable_game_inf() = gameInfPacket;
    packets::Packet* packet = new packets::Packet();
    *packet->mutable_system_packet() = systemPacket;
    *packet->mutable_metadata()->mutable_user_id() = packets::convert(sInstance->mUserID);

    if (!google::protobuf::util::MessageDifferencer::Equals(*packet, sInstance->lastGameInfPacket) 
        && !google::protobuf::util::MessageDifferencer::Equals(*packet, sInstance->emptyGameInfPacket)) {
        sInstance->lastGameInfPacket = *packet;
        sInstance->mSocket->queuePacket(packet);
    } else {
        sInstance->mHeap->free(packet); // free packet if we're not using it
    }
}

/**
 * @brief 
 * Sends only stage info to the server.
 * @param holder 
 */
void Client::sendGameInfPacket(GameDataHolderAccessor holder) {

    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);
    
    packets::system::GameInf gameInfPacket;

    gameInfPacket.mutable_is_2d()->set_value(false);

    gameInfPacket.set_scenario_num(holder.mData->mGameDataFile->getScenarioNo());

    gameInfPacket.set_stage_name(std::string(GameDataFunction::getCurrentStageName(holder)));

    packets::system::SystemPacket systemPacket;
    *systemPacket.mutable_game_inf() = gameInfPacket;
    packets::Packet* packet = new packets::Packet();
    *packet->mutable_system_packet() = systemPacket;
    *packet->mutable_metadata()->mutable_user_id() = packets::convert(sInstance->mUserID);

    if (!google::protobuf::util::MessageDifferencer::Equals(*packet, sInstance->emptyGameInfPacket)) {
        sInstance->lastGameInfPacket = *packet;
        sInstance->mSocket->queuePacket(packet);
    } else {
        sInstance->mHeap->free(packet); // free packet if we're not using it
    }
}

/**
 * @brief 
 * 
 */
void Client::sendTagInfPacket() {

    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    HideAndSeekMode* hsMode = GameModeManager::instance()->getMode<HideAndSeekMode>();

    if (!GameModeManager::instance()->isMode(GameMode::HIDEANDSEEK)) {
        Logger::log("State is not Hide and Seek!\n");
        return;
    }

    HideAndSeekInfo* curInfo = GameModeManager::instance()->getInfo<HideAndSeekInfo>();

    packets::system::TagInf tagInfPacket;

    tagInfPacket.mutable_is_it()->set_value(hsMode->isPlayerIt());

    tagInfPacket.set_minutes(curInfo->mHidingTime.mMinutes);
    tagInfPacket.set_seconds(curInfo->mHidingTime.mSeconds);
    tagInfPacket.set_update_type(packets::system::TagUpdateType::STATE | packets::system::TagUpdateType::TIME);

    packets::system::SystemPacket systemPacket;
    *systemPacket.mutable_tag_inf() = tagInfPacket;
    packets::Packet* packet = new packets::Packet();
    *packet->mutable_system_packet() = systemPacket;
    *packet->mutable_metadata()->mutable_user_id() = packets::convert(sInstance->mUserID);

    sInstance->mSocket->queuePacket(packet);
}

/**
 * @brief 
 * 
 * @param body 
 * @param cap 
 */
void Client::sendCostumeInfPacket(const char* body, const char* cap) {

    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    if (!strcmp(body, "") && !strcmp(cap, "")) { return; }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    packets::system::CostumeInf costumeInfPacket;

    costumeInfPacket.set_body_model(body);
    costumeInfPacket.set_cap_model(cap);

    packets::system::SystemPacket systemPacket;
    *systemPacket.mutable_costume_inf() = costumeInfPacket;
    packets::Packet* packet = new packets::Packet();
    *packet->mutable_system_packet() = systemPacket;
    *packet->mutable_metadata()->mutable_user_id() = packets::convert(sInstance->mUserID);

    sInstance->lastCostumeInfPacket = *packet;
    sInstance->mSocket->queuePacket(packet);
}

void Client::sendFlagHoldStatePacket(packets::ctf::FlagHoldState const& flagHoldStatePacket) {
    packets::ctf::CTFPacket ctfPacket;
    *ctfPacket.mutable_flag_hold_state() = flagHoldStatePacket;

    sInstance->lastFlagHoldStatePacket = packets::Packet();
    *sInstance->lastFlagHoldStatePacket.mutable_ctf_packet() = ctfPacket;
    *sInstance->lastFlagHoldStatePacket.mutable_metadata()->mutable_user_id() = packets::convert(sInstance->mUserID);
    
    sInstance->mSocket->queuePacket(&sInstance->lastFlagHoldStatePacket);
}

void Client::sendCTFWinPacket(packets::ctf::Win const& winPacket) {
    packets::ctf::CTFPacket ctfPacket;
    *ctfPacket.mutable_win() = winPacket;

    sInstance->lastCTFWinPacket = packets::Packet();
    *sInstance->lastCTFWinPacket.mutable_ctf_packet() = ctfPacket;
    *sInstance->lastCTFWinPacket.mutable_metadata()->mutable_user_id() = packets::convert(sInstance->mUserID);
    
    sInstance->mSocket->queuePacket(&sInstance->lastCTFWinPacket);
}


/**
 * @brief 
 * 
 * @param player 
 */
void Client::sendCaptureInfPacket(const PlayerActorHakoniwa* player) {

    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);
    
    if (sInstance->isClientCaptured && !sInstance->isSentCaptureInf) {
        packets::system::CaptureInf captureInfPacket;

        captureInfPacket.set_hack_name(tryConvertName(player->mHackKeeper->getCurrentHackName()));

        packets::system::SystemPacket systemPacket;
        *systemPacket.mutable_capture_inf() = captureInfPacket;
        packets::Packet* packet = new packets::Packet();
        *packet->mutable_system_packet() = systemPacket;
        *packet->mutable_metadata()->mutable_user_id() = packets::convert(sInstance->mUserID);

        sInstance->mSocket->queuePacket(packet);
        sInstance->isSentCaptureInf = true;
    } else if (!sInstance->isClientCaptured && sInstance->isSentCaptureInf) {
        packets::system::CaptureInf captureInfPacket;

        captureInfPacket.set_hack_name("");

        packets::system::SystemPacket systemPacket;
        *systemPacket.mutable_capture_inf() = captureInfPacket;
        packets::Packet* packet = new packets::Packet();
        *packet->mutable_system_packet() = systemPacket;
        *packet->mutable_metadata()->mutable_user_id() = packets::convert(sInstance->mUserID);

        sInstance->mSocket->queuePacket(packet);
        sInstance->isSentCaptureInf = false;
    }
}

/**
 * @brief
 */
void Client::resendInitPackets() {
    // CostumeInfPacket
    if (packets::convert(lastCostumeInfPacket.metadata().user_id()) == mUserID) {
        mSocket->queuePacket(&lastCostumeInfPacket);
    }

    // GameInfPacket
    if (!google::protobuf::util::MessageDifferencer::Equals(lastGameInfPacket, emptyGameInfPacket)) {
        mSocket->queuePacket(&lastGameInfPacket);
    }
}

/**
 * @brief 
 * 
 * @param shineID 
 */
void Client::sendShineCollectPacket(int shineID) {

    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    if(sInstance->lastCollectedShine != shineID) {
        packets::system::ShineCollect shineCollectPacket;

        shineCollectPacket.set_shine_id(shineID);

        packets::system::SystemPacket systemPacket;
        *systemPacket.mutable_shine_collect() = shineCollectPacket;
        packets::Packet* packet = new packets::Packet();
        *packet->mutable_system_packet() = systemPacket;
        *packet->mutable_metadata()->mutable_user_id() = packets::convert(sInstance->mUserID);

        sInstance->lastCollectedShine = shineID;
        sInstance->mSocket->queuePacket(packet);
    }
}

/**
 * @brief 
 * 
 * @param packet 
 */
void Client::updatePlayerInfo(packets::PacketMetadata const& packetMetadata, packets::system::PlayerInf const& packet) {
    PuppetInfo* curInfo = findPuppetInfo(packets::convert(packetMetadata.user_id()), false);

    if (!curInfo) {
        return;
    }

    if(!curInfo->isConnected) {
        curInfo->isConnected = true;
    }

    auto const playerPos = packets::convert(packet.player_pos());
    curInfo->playerPos = playerPos;

    auto const playerRot = packets::convert(packet.player_rot());

    // check if rotation is larger than zero and less than or equal to 1
    if(abs(playerRot.x) > 0.f || abs(playerRot.y) > 0.f || abs(playerRot.z) > 0.f || abs(playerRot.w) > 0.f) {
        if(abs(playerRot.x) <= 1.f || abs(playerRot.y) <= 1.f || abs(playerRot.z) <= 1.f || abs(playerRot.w) <= 1.f) {
            curInfo->playerRot = playerRot;
        }
    }

        if (packets::convert(packet.act_name()) != PlayerAnims::Type::Unknown) {
            strcpy(curInfo->curAnimStr, PlayerAnims::FindStr(packets::convert(packet.act_name())));
            if (curInfo->curAnimStr[0] == '\0')
                Logger::log("[ERROR] %s: actName was out of bounds: %d\n", __func__, packet.act_name());
        } else {
            strcpy(curInfo->curAnimStr, "Wait");
        }

        if(packets::convert(packet.sub_act_name()) != PlayerAnims::Type::Unknown) {
            strcpy(curInfo->curSubAnimStr, PlayerAnims::FindStr(packets::convert(packet.sub_act_name())));
            if (curInfo->curSubAnimStr[0] == '\0')
                Logger::log("[ERROR] %s: subActName was out of bounds: %d\n", __func__, packet.sub_act_name());
        } else {
            strcpy(curInfo->curSubAnimStr, "");
        }

    curInfo->curAnim = packets::convert(packet.act_name());
    curInfo->curSubAnim = packets::convert(packet.sub_act_name());

    for (size_t i = 0; i < 6 && i < packet.anim_blend_weights().size(); i++)
    {
        // weights can only be between 0 and 1
        if(packet.anim_blend_weights().at(i) >= 0.f && packet.anim_blend_weights().at(i) <= 1.f) {
            curInfo->blendWeights[i] = packet.anim_blend_weights().at(i);
        }
    }

    //TEMP

    if(!curInfo->isCapThrow) {
        curInfo->capPos = packets::convert(packet.player_pos());
    }

}

/**
 * @brief 
 * 
 * @param packet 
 */
void Client::updateHackCapInfo(packets::PacketMetadata const& packetMetadata, packets::system::HackCapInf const& packet) {

    PuppetInfo* curInfo = findPuppetInfo(packets::convert(packetMetadata.user_id()), false);

    if (curInfo) {
        curInfo->capPos = packets::convert(packet.cap_pos());
        curInfo->capRot = packets::convert(packet.cap_quat());

        curInfo->isCapThrow = packet.is_cap_visible().value();

        strcpy(curInfo->capAnim, packet.cap_anim().c_str());
    }
}

/**
 * @brief 
 * 
 * @param packet 
 */
void Client::updateCaptureInfo(packets::PacketMetadata const& packetMetadata, packets::system::CaptureInf const& packet) {
    
    PuppetInfo* curInfo = findPuppetInfo(packets::convert(packetMetadata.user_id()), false);
        
    if (!curInfo) {
        return;
    }

    curInfo->isCaptured = packet.hack_name().size() > 0;

    if (curInfo->isCaptured) {
        strcpy(curInfo->curHack, packet.hack_name().c_str());
    }
}

/**
 * @brief 
 * 
 * @param packet 
 */
void Client::updateCostumeInfo(packets::PacketMetadata const& packetMetadata, packets::system::CostumeInf const& packet) {

    PuppetInfo* curInfo = findPuppetInfo(packets::convert(packetMetadata.user_id()), false);

    if (!curInfo) {
        return;
    }

    strcpy(curInfo->costumeBody, packet.body_model().c_str());
    strcpy(curInfo->costumeHead, packet.cap_model().c_str());
}

/**
 * @brief 
 * 
 * @param packet 
 */
void Client::updateShineInfo(packets::PacketMetadata const& packetMetadata, packets::system::ShineCollect const& packet) {
    if(collectedShineCount < curCollectedShines.size() - 1) {
        curCollectedShines[collectedShineCount] = packet.shine_id();
        collectedShineCount++;
    }
}

/**
 * @brief 
 * 
 * @param packet 
 */
void Client::updatePlayerConnect(packets::PacketMetadata const& packetMetadata, packets::system::PlayerConnect const& packet) {
    
    PuppetInfo* curInfo = findPuppetInfo(packets::convert(packetMetadata.user_id()), true);

    if (!curInfo) {
        return;
    }

    if (curInfo->isConnected) {

        Logger::log("Info is already being used by another connected player!\n");
        packets::convert(packetMetadata.user_id()).print("Connection ID");
        curInfo->playerID.print("Target Info");

    } else {

        packets::convert(packetMetadata.user_id()).print("Player Connected! ID");

        curInfo->playerID = packets::convert(packetMetadata.user_id());
        curInfo->isConnected = true;
        strcpy(curInfo->puppetName, packet.client_name().c_str());

        mConnectCount++;
    }
}

/**
 * @brief 
 * 
 * @param packet 
 */
void Client::updateGameInfo(packets::PacketMetadata const& packetMetadata, packets::system::GameInf const& packet) {

    PuppetInfo* curInfo = findPuppetInfo(packets::convert(packetMetadata.user_id()), false);

    if (!curInfo) {
        return;
    }

    if(curInfo->isConnected) {

        curInfo->scenarioNo = packet.scenario_num();

        if(strcmp(packet.stage_name().c_str(), "") != 0 && strlen(packet.stage_name().c_str()) > 3) {
            strcpy(curInfo->stageName, packet.stage_name().c_str());
        }

        curInfo->is2D = packet.is_2d().value();
    }
}

/**
 * @brief 
 * 
 * @param packet 
 */
void Client::updateTagInfo(packets::PacketMetadata const& packetMetadata, packets::system::TagInf const& packet) {
    Logger::log("H&S Tag Packet Received\n");
    // if the packet is for our player, edit info for our player
    if (packets::convert(packetMetadata.user_id()) == mUserID && GameModeManager::instance()->isMode(GameMode::HIDEANDSEEK)) {

        HideAndSeekMode* mMode = GameModeManager::instance()->getMode<HideAndSeekMode>();
        HideAndSeekInfo* curInfo = GameModeManager::instance()->getInfo<HideAndSeekInfo>();

        if (packet.update_type() & packets::system::TagUpdateType::STATE) {
            mMode->setPlayerTagState(packet.is_it().value());
        }

        if (packet.update_type() & packets::system::TagUpdateType::TIME) {
            curInfo->mHidingTime.mSeconds = packet.seconds();
            curInfo->mHidingTime.mMinutes = packet.minutes();
        }

        return;

    }

    PuppetInfo* curInfo = findPuppetInfo(packets::convert(packetMetadata.user_id()), false);

    if (!curInfo) {
        return;
    }

    curInfo->isIt = packet.is_it().value();
    curInfo->seconds = packet.seconds();
    curInfo->minutes = packet.minutes();
}

void Client::updateCTFPlayerTeamAssignment(packets::PacketMetadata const& packetMetadata, packets::ctf::PlayerTeamAssignment const& packet) {
    if (packets::convert(packetMetadata.user_id()) != mUserID) {
        PuppetInfo* curInfo = findPuppetInfo(packets::convert(packetMetadata.user_id()), false);
        if (!curInfo) {
            return;
        }

        curInfo->ctfTeam = packet.team();
        return;
    }

    if (!GameModeManager::instance()->isModeAndActive(GameMode::CAPTURE_THE_FLAG)) {
        return;
    }

    CaptureTheFlagInfo* curInfo = GameModeManager::instance()->getInfo<CaptureTheFlagInfo>();

    curInfo->mPlayerTeam = packet.team();

    if (packet.should_hold_team_flag().value()) {
        curInfo->mHeldFlag = packet.team();
    }    
}

void Client::updateCTFGameState(packets::PacketMetadata const& packetMetadata, packets::ctf::GameState const& packet) {
    if (!GameModeManager::instance()->isModeAndActive(GameMode::CAPTURE_THE_FLAG)) {
        return;
    }

    if (packet.reset_game().value()) {
        CaptureTheFlagMode* mMode = GameModeManager::instance()->getMode<CaptureTheFlagMode>();
        mMode->resetGameInfo(); // Team assignment is sent on game start, so if a new game starts, make sure to reset all info from the previous game
    }

    CaptureTheFlagInfo* curInfo = GameModeManager::instance()->getInfo<CaptureTheFlagInfo>();
    curInfo->mRoundTime.mSeconds = packet.seconds();
    curInfo->mRoundTime.mMinutes = packet.minutes();
}

void Client::updateCTFFlagHoldState(packets::PacketMetadata const& packetMetadata, packets::ctf::FlagHoldState const& packet) {
    FlagActor::latestReceivedFlagTeam = packet.flag_team();
    auto flagActorMaybe = FlagActor::getFlagActorForTeam(packet.flag_team());
    if (flagActorMaybe.has_value()) {
        auto* flagActor = flagActorMaybe.value();
        flagActor->setHeldPlayer(packet.is_held().value() ? packets::convert(packetMetadata.user_id()) : nn::account::Uid());
        flagActor->setIsHeld(packet.is_held().value());
    }
}

void Client::updateCTFFlagDroppedPosition(packets::PacketMetadata const& packetMetadata, packets::ctf::FlagDroppedPosition const& packet) {
    if (!GameModeManager::instance()->isModeAndActive(GameMode::CAPTURE_THE_FLAG)) {
        return;
    }

    if (GameModeManager::instance()->getMode<CaptureTheFlagMode>()->isFirstFrame()) {
        return; // Getting local player is invalid on first frame after loading a stage (causes crash)
    }
    
    FlagActor::latestReceivedFlagTeam = packet.flag_team();
    FlagActor::latestReceivedPosition = packets::convert(packet.pos());
    strcpy(FlagActor::latestReceivedStage, packet.stage_name().c_str());

    auto flagActorMaybe = FlagActor::getFlagActorForTeam(packet.flag_team());
    if (flagActorMaybe.has_value()) {
        auto* flagActor = flagActorMaybe.value();
        // if (StageData(mStageName.cstr(), mScenario) != StageData(packet.stageName, packet.scenarioNo)) {
        //     flagActor->makeActorDead();
        //     return;
        // }

        flagActor->makeActorAlive();
        flagActor->setHeldPlayer(nn::account::Uid());
        flagActor->setIsHeld(false);
        flagActor->setTransform(packets::convert(packet.pos()), packets::convert(packet.rot()));
    }
}

void Client::updateCTFWinState(packets::PacketMetadata const& packetMetadata, packets::ctf::Win const& packet) {
    if (!GameModeManager::instance()->isModeAndActive(GameMode::CAPTURE_THE_FLAG)) {
        return;
    }

    CaptureTheFlagInfo* curInfo = GameModeManager::instance()->getInfo<CaptureTheFlagInfo>();
    curInfo->mWinningTeam = packet.winning_team();
}

std::optional<std::tuple<sead::Vector3f, sead::Quatf>> Client::getPlayerTransformInSameStage(nn::account::Uid const& userID) {
    if (userID == mUserID) {
        if (!mSceneInfo || !mSceneInfo->mSceneObjHolder) {
            return {};
        }

        if (!GameModeManager::instance()->isModeAndActive(GameMode::CAPTURE_THE_FLAG)) {
            return {};
        }

        if (GameModeManager::instance()->getMode<CaptureTheFlagMode>()->isFirstFrame()) {
            return {}; // Getting local player is invalid on first frame after loading a stage (causes crash)
        }

        auto* player = rs::getPlayerActor(mCurStageScene);
        if (!player) {
            return {};
        }

        sead::Quatf rot{};
        al::calcQuat(&rot, player);
        return std::make_tuple(
            al::getTrans(player),
            rot
        );
    }

    PuppetInfo* curInfo = findPuppetInfo(userID, false);
    if (!curInfo) {
        return {};
    }

    if (!curInfo->isInSameStage) {
        return {};
    }

    return std::make_tuple(curInfo->playerPos, curInfo->playerRot);
}

/**
 * @brief 
 * 
 * @param packet 
 */
void Client::sendToStage(packets::PacketMetadata const& packetMetadata, packets::system::ChangeStage const& packet) {
    if (mSceneInfo && mSceneInfo->mSceneObjHolder) {

        GameDataHolderAccessor accessor(mSceneInfo->mSceneObjHolder);

        Logger::log("Sending Player to %s at Entrance %s in Scenario %d\n", packet.change_stage().c_str(),
                     packet.change_id().c_str(), packet.scenario_num());
        
        ChangeStageInfo info(accessor.mData, packet.change_id().c_str(), packet.change_stage().c_str(), false, packet.scenario_num(), static_cast<ChangeStageInfo::SubScenarioType>(packet.sub_scenario_type()));
        GameDataFunction::tryChangeNextStage(accessor, &info);
    }
}

/**
 * @brief 
 * 
 * @param packet 
 */
void Client::disconnectPlayer(packets::PacketMetadata const& packetMetadata, packets::system::PlayerDC const& packet) {

    PuppetInfo* curInfo = findPuppetInfo(packets::convert(packetMetadata.user_id()), false);

    if (!curInfo || !curInfo->isConnected) {
        return;
    }
    
    curInfo->isConnected = false;

    curInfo->scenarioNo = -1;
    strcpy(curInfo->stageName, "");
    curInfo->isInSameStage = false;

    mConnectCount--;
}

/**
 * @brief 
 * 
 * @param shineId 
 * @return true 
 * @return false 
 */
bool Client::isShineCollected(int shineId) {

    for (size_t i = 0; i < curCollectedShines.size(); i++)
    {
        if(curCollectedShines[i] >= 0) {
            if(curCollectedShines[i] == shineId) {
                return true;
            }
        }
    }
    
    return false;

}

/**
 * @brief 
 * 
 * @param id 
 * @return int 
 */
PuppetInfo* Client::findPuppetInfo(const nn::account::Uid& id, bool isFindAvailable) {

    PuppetInfo *firstAvailable = nullptr;

    for (size_t i = 0; i < getMaxPlayerCount() - 1; i++) {

        PuppetInfo* curInfo = mPuppetInfoArr[i];

        if (curInfo->playerID == id) {
            return curInfo;
        } else if (isFindAvailable && !firstAvailable && !curInfo->isConnected) {
            firstAvailable = curInfo;
        }
    }

    if (!firstAvailable) {
        Logger::log("Unable to find Assigned Puppet for Player!\n");
        id.print("User ID");
    }

    return firstAvailable;
}

/**
 * @brief 
 * 
 * @param holder 
 */
void Client::setStageInfo(GameDataHolderAccessor holder) {
    if (sInstance) {
        sInstance->mStageName = GameDataFunction::getCurrentStageName(holder);
        sInstance->mScenario = holder.mData->mGameDataFile->getScenarioNo(); //holder.mData->mGameDataFile->getMainScenarioNoCurrent();
        
        sInstance->mPuppetHolder->setStageInfo(sInstance->mStageName.cstr(), sInstance->mScenario);
    }
}

/**
 * @brief 
 * 
 * @param puppet 
 * @return true 
 * @return false 
 */
bool Client::tryAddPuppet(PuppetActor *puppet) {
    if(sInstance) {
        return sInstance->mPuppetHolder->tryRegisterPuppet(puppet);
    }else {
        return false;
    }
}

/**
 * @brief 
 * 
 * @param puppet 
 * @return true 
 * @return false 
 */
bool Client::tryAddDebugPuppet(PuppetActor *puppet) {
    if(sInstance) {
        return sInstance->mPuppetHolder->tryRegisterDebugPuppet(puppet);
    }else {
        return false;
    }
}

/**
 * @brief 
 * 
 * @param idx 
 * @return PuppetActor* 
 */
PuppetActor *Client::getPuppet(int idx) {
    if(sInstance) {
        return sInstance->mPuppetHolder->getPuppetActor(idx);
    }else {
        return nullptr;
    }
}

/**
 * @brief 
 * 
 * @return PuppetInfo* 
 */
PuppetInfo *Client::getLatestInfo() {
    if(sInstance) {
        return Client::getPuppetInfo(sInstance->mPuppetHolder->getSize() - 1);
    }else {
        return nullptr;
    }
}

/**
 * @brief 
 * 
 * @param idx 
 * @return PuppetInfo* 
 */
PuppetInfo *Client::getPuppetInfo(int idx) {
    if(sInstance) {
        // unsafe get
        PuppetInfo *curInfo = sInstance->mPuppetInfoArr[idx];

        if (!curInfo) {
            Logger::log("Attempting to Access Puppet Out of Bounds! Value: %d\n", idx);
            return nullptr;
        }

        return curInfo;
    }else {
        return nullptr;
    }
}

/**
 * @brief 
 * 
 */
void Client::resetCollectedShines() {
    collectedShineCount = 0;
    curCollectedShines.fill(-1);
}

/**
 * @brief 
 * 
 * @param shineId 
 */
void Client::removeShine(int shineId) {
    for (size_t i = 0; i < curCollectedShines.size(); i++)
    {
        if(curCollectedShines[i] == shineId) {
            curCollectedShines[i] = -1;
            collectedShineCount--;
        }
    }
}

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool Client::isNeedUpdateShines() {
    return sInstance ? sInstance->collectedShineCount > 0 : false;
}

/**
 * @brief 
 * 
 */
void Client::updateShines() {
    if (!sInstance) {
        Logger::log("Client Null!\n");
        return;
    }

    // skip shine sync if player is in cap kingdom scenario zero (very start of the game)
    if (sInstance->mStageName == "CapWorldHomeStage" && (sInstance->mScenario == 0 || sInstance->mScenario == 1)) {
        return;
    }

    GameDataHolderAccessor accessor(sInstance->mCurStageScene);
    
    for (size_t i = 0; i < sInstance->getCollectedShinesCount(); i++)
    {
        int shineID = sInstance->getShineID(i);

        if(shineID < 0) continue;

        Logger::log("Shine UID: %d\n", shineID);

        GameDataFile::HintInfo* shineInfo = CustomGameDataFunction::getHintInfoByUniqueID(accessor, shineID);

        if (shineInfo) {
            if (!GameDataFunction::isGotShine(accessor, shineInfo->mStageName.cstr(), shineInfo->mObjId.cstr())) {

                Shine* stageShine = findStageShine(shineID);
                
                if (stageShine) {

                    if (al::isDead(stageShine)) {
                        stageShine->makeActorAlive();
                    }
                    
                    stageShine->getDirect();
                    stageShine->onSwitchGet();
                }

                accessor.mData->mGameDataFile->setGotShine(shineInfo);
            }
        }
    }
    
    sInstance->resetCollectedShines();
    sInstance->mCurStageScene->mSceneLayout->startShineCountAnim(false);
    sInstance->mCurStageScene->mSceneLayout->updateCounterParts(); // updates shine chip layout to (maybe) prevent softlocks
}

/**
 * @brief 
 * 
 */
void Client::update() {
    if (sInstance) {
        
        sInstance->mPuppetHolder->update();

        if (isNeedUpdateShines()) {
            updateShines();
        }

        GameModeManager::instance()->update();
    }
}

/**
 * @brief 
 * 
 */
void Client::clearArrays() {
    if(sInstance) {
        sInstance->mPuppetHolder->clearPuppets();
        sInstance->mShineArray.clear();

    }
}

/**
 * @brief 
 * 
 * @return PuppetInfo* 
 */
PuppetInfo *Client::getDebugPuppetInfo() {
    if(sInstance) {
        return &sInstance->mDebugPuppetInfo;
    }else {
        return nullptr;
    }
}

/**
 * @brief 
 * 
 * @return PuppetActor* 
 */
PuppetActor *Client::getDebugPuppet() {
    if(sInstance) {
        return sInstance->mPuppetHolder->getDebugPuppet();
    }else {
        return nullptr;
    }
}

/**
 * @brief 
 * 
 * @return Keyboard* 
 */
Keyboard* Client::getKeyboard() {
    if (sInstance) {
        return sInstance->mKeyboard;
    }
    return nullptr;
}

/**
 * @brief 
 * 
 * @return const char* 
 */
const char* Client::getCurrentIP() {
    if (sInstance) {
        return sInstance->mServerIP.cstr();
    }
    return nullptr;
}

/**
 * @brief 
 * 
 * @return const int 
 */
const int Client::getCurrentPort() {
    if (sInstance) {
        return sInstance->mServerPort;
    }
    return -1;
}

/**
 * @brief sets server IP to supplied string, used specifically for loading IP from the save file.
 * 
 * @param ip 
 */
void Client::setLastUsedIP(const char* ip) {
    if (sInstance) {
        sInstance->mServerIP = ip;
    }
}

/**
 * @brief sets server port to supplied string, used specifically for loading port from the save file.
 * 
 * @param port 
 */
void Client::setLastUsedPort(const int port) {
    if (sInstance) {
        sInstance->mServerPort = port;
    }
}

/**
 * @brief creates new scene info and copies supplied info to the new info, as well as stores a const ptr to the current stage scene.
 * 
 * @param initInfo 
 * @param stageScene 
 */
void Client::setSceneInfo(const al::ActorInitInfo& initInfo, const StageScene *stageScene) {

    if (!sInstance) {
        Logger::log("Client Null!\n");
        return;
    }

    sInstance->mSceneInfo = new al::ActorSceneInfo();

    memcpy(sInstance->mSceneInfo, &initInfo.mActorSceneInfo, sizeof(al::ActorSceneInfo));

    sInstance->mCurStageScene = stageScene;
}

/**
 * @brief stores shine pointer supplied into a ptr array if space is available, and shine is not collected.
 * 
 * @param shine 
 * @return true if shine was able to be successfully stored
 * @return false if shine is already collected, or ptr array is full
 */
bool Client::tryRegisterShine(Shine* shine) {
    if (sInstance) {
        if (!sInstance->mShineArray.isFull()) {
            if (!shine->isGot()) {
                sInstance->mShineArray.pushBack(shine);
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief finds the actor pointer stored in the shine ptr array based off shine ID
 * 
 * @param shineID Unique ID used for shine actor
 * @return Shine* if shine ptr array contains actor with supplied shine ID.
 */
Shine* Client::findStageShine(int shineID) {
    if (sInstance) {
        for (int i = 0; i < sInstance->mShineArray.size(); i++) {

            Shine* curShine = sInstance->mShineArray[i];

            if (curShine) {

                auto hintInfo =
                    CustomGameDataFunction::getHintInfoByIndex(curShine, curShine->mShineIdx);
                
                if (hintInfo->mUniqueID == shineID) {
                    return curShine;
                }
            }
        }
    }
    return nullptr;
}

void Client::showConnectError(const char16_t* msg) {
    if (!sInstance)
        return;

    sInstance->mConnectionWait->setTxtMessageConfirm(msg);

    al::hidePane(sInstance->mConnectionWait, "Page01");  // hide A button prompt

    if (!sInstance->mConnectionWait->mIsAlive) {
        sInstance->mConnectionWait->appear();

        sInstance->mConnectionWait->playLoop();
    }

    al::startAction(sInstance->mConnectionWait, "Confirm", "State");
}

void Client::showConnect() {
    if (!sInstance)
        return;
    
    sInstance->mConnectionWait->appear();

    sInstance->mConnectionWait->playLoop();
    
}

void Client::hideConnect() {
    if (!sInstance)
        return;

    sInstance->mConnectionWait->tryEnd();
}
