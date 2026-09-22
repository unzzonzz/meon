#include "MeonSession.h"

namespace meon
{

static const char* kCodeChars = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";   // 혼동 문자(O,0,I,1) 제외

//==============================================================================
/** 서버까지의 왕복 시간을 TCP 접속 시간으로 어림한다 (참고용). */
class ServerPinger : public juce::Thread
{
public:
    ServerPinger (const juce::String& h, int p) : juce::Thread ("MEON server ping"), host (h), port (p) {}
    ~ServerPinger() override { stopThread (5000); }

    void run() override
    {
        while (! threadShouldExit())
        {
            measure();
            wait (intervalMs.load());
        }
    }

    std::atomic<float> lastMs { -1.0f };
    std::atomic<int> intervalMs { 5000 };

private:
    void measure()
    {
        juce::StreamingSocket sock;
        const double t0 = juce::Time::getMillisecondCounterHiRes();
        if (sock.connect (host, port, 3000))
            lastMs = (float) (juce::Time::getMillisecondCounterHiRes() - t0);
        else
            lastMs = -1.0f;
        sock.close();
    }

    juce::String host;
    int port;
};

//==============================================================================
MeonSession::MeonSession (SonobusAudioProcessor& p, MeonSettings& s, bool plugin)
    : processor (p), settings (s), isPlugin (plugin)
{
    applyProcessorDefaults();
    processor.addClientListener (this);
    pinger = std::make_unique<ServerPinger> (DEFAULT_SERVER_HOST, DEFAULT_SERVER_PORT);
    pinger->startThread();
    startTimer (200);
}

MeonSession::~MeonSession()
{
    stopTimer();
    processor.removeClientListener (this);
    cancelPendingUpdate();
    if (roomState != RoomState::None)
    {
        log.end ("quit");
        processor.leaveServerGroup (roomCode.isNotEmpty() ? roomCode : pendingCode);
    }
    pinger = nullptr;
}

//==============================================================================
juce::String MeonSession::generateRoomCode()
{
    auto& rng = juce::Random::getSystemRandom();
    juce::String code;
    const int n = (int) strlen (kCodeChars);
    for (int i = 0; i < 6; ++i)
        code += juce::String::charToString ((juce::juce_wchar) kCodeChars[rng.nextInt (n)]);
    return code;
}

bool MeonSession::isValidRoomCode (const juce::String& code)
{
    if (code.length() != 6)
        return false;
    for (int i = 0; i < 6; ++i)
    {
        const juce::juce_wchar c = code[i];
        if (! ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')))
            return false;
    }
    return true;
}

juce::String MeonSession::displayNameFor (const juce::String& user)
{
    const int hash = user.lastIndexOfChar ('#');
    if (hash > 0 && user.length() - hash - 1 == 4)
        return user.substring (0, hash);
    return user;
}

juce::String MeonSession::makeUserName() const
{
    auto& rng = juce::Random::getSystemRandom();
    juce::String suffix;
    const int n = (int) strlen (kCodeChars);
    for (int i = 0; i < 4; ++i)
        suffix += juce::String::charToString ((juce::juce_wchar) kCodeChars[rng.nextInt (n)]);
    return settings.getNickname().trim() + "#" + suffix;
}

juce::String MeonSession::getDisplayName() const
{
    return settings.getNickname().trim();
}

//==============================================================================
void MeonSession::applyProcessorDefaults()
{
    // 코덱: 무압축 PCM 16 bit 고정
    pcmFormatIndex = -1;
    for (int i = 0; i < processor.getNumberAudioCodecFormats(); ++i)
    {
        SonobusAudioProcessor::AudioCodecFormatInfo info;
        if (processor.getAudioCodeFormatInfo (i, info) && info.codec == SonobusAudioProcessor::CodecPCM && info.bitdepth == 2)
        {
            pcmFormatIndex = i;
            break;
        }
    }
    if (pcmFormatIndex >= 0)
    {
        processor.setChangingDefaultAudioCodecSetsExisting (true);
        processor.setDefaultAudioCodecFormat (pcmFormatIndex);
    }

    // 지터버퍼: 자동 (양방향)
    processor.setDefaultAutoresizeBufferMode (SonobusAudioProcessor::AutoNetBufferModeAutoFull);

    // 이펙트 전부 끔 (엔진 코드는 그대로, 사용만 하지 않음)
    for (int g = 0; g < processor.getInputGroupCount(); ++g)
    {
        SonoAudio::CompressorParams cp;
        if (processor.getInputCompressorParams (g, cp)) { cp.enabled = false; processor.setInputCompressorParams (g, cp); }
        SonoAudio::CompressorParams ep;
        if (processor.getInputExpanderParams (g, ep)) { ep.enabled = false; processor.setInputExpanderParams (g, ep); }
        SonoAudio::CompressorParams lp;
        if (processor.getInputLimiterParams (g, lp)) { lp.enabled = false; processor.setInputLimiterParams (g, lp); }
        SonoAudio::ParametricEqParams eq;
        if (processor.getInputEqParams (g, eq)) { eq.enabled = false; processor.setInputEqParams (g, eq); }
    }

    auto& state = processor.getValueTreeState();
    auto setParam = [&state] (const juce::String& id, float value)
    {
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    setParam (SonobusAudioProcessor::paramMainReverbEnabled, 0.0f);
    setParam (SonobusAudioProcessor::paramMainRecvMute, 0.0f);
    setParam (SonobusAudioProcessor::paramMainInMute, 0.0f);
    setParam (SonobusAudioProcessor::paramMetEnabled, 0.0f);
    setParam (SonobusAudioProcessor::paramSendMetAudio, 0.0f);
    setParam (SonobusAudioProcessor::paramSendFileAudio, 0.0f);
    setParam (SonobusAudioProcessor::paramSendSoundboardAudio, 0.0f);
    setParam (SonobusAudioProcessor::paramHearLatencyTest, 0.0f);
    setParam (SonobusAudioProcessor::paramMainSendMute, 0.0f);

    // 그룹 피어에 자동 연결, 서버 끊김 시 자동 재접속(엔진 내장)
    processor.setAutoconnectToGroupPeers (true);
    processor.setReconnectAfterServerLoss (true);
    processor.setWatchPublicGroups (false);

    if (! isPlugin)
        setInputChannels (settings.getInputChannelStart(), settings.getInputChannelCount());

    processor.getInputMeterSource().setMaxHoldMS (100);
    processor.getSendMeterSource().setMaxHoldMS (100);
}

void MeonSession::setInputChannels (int start, int count)
{
    if (isPlugin)
        return;
    count = juce::jlimit (1, 2, count);
    start = juce::jmax (0, start);
    processor.setInputGroupCount (1);
    processor.setInputGroupChannelStartAndCount (0, start, count);
    processor.setInputGroupChannelDestStartAndCount (0, 0, 2);
    processor.setInputGroupMuted (0, false);

    if (auto* param = processor.getValueTreeState().getParameter (SonobusAudioProcessor::paramSendChannels))
        param->setValueNotifyingHost (param->convertTo0to1 ((float) count));   // 1 = mono, 2 = stereo
}

void MeonSession::applyPeerDefaults (int peerIndex, Member& m)
{
    if (pcmFormatIndex >= 0)
        processor.setRemotePeerAudioCodecFormat (peerIndex, pcmFormatIndex);
    processor.setRemotePeerAutoresizeBufferMode (peerIndex, SonobusAudioProcessor::AutoNetBufferModeAutoFull);
    if (auto* src = processor.getRemotePeerRecvMeterSource (peerIndex))
        src->setMaxHoldMS (100);
    m.defaultsApplied = true;
}

//==============================================================================
void MeonSession::start()
{
    if (settings.getNickname().trim().isEmpty())
        return;
    wantConnected = true;
    if (serverState == ServerState::Disconnected)
        connectNow();
}

void MeonSession::applyNicknameChange()
{
    if (roomState != RoomState::None)
        return;   // 합주 중에 바꾸면 다음 입장부터 적용
    if (serverState != ServerState::Disconnected)
    {
        processor.disconnectFromServer();
        serverState = ServerState::Disconnected;
    }
    nextConnectAttemptMs = 0.0;
    start();
}

void MeonSession::connectNow()
{
    if (settings.getNickname().trim().isEmpty())
        return;
    userName = makeUserName();
    processor.setCurrentUsername (userName);
    serverState = ServerState::Connecting;
    connectDeadlineMs = juce::Time::getMillisecondCounterHiRes() + 12000.0;
    if (! processor.connectToServer (DEFAULT_SERVER_HOST, DEFAULT_SERVER_PORT, userName))
    {
        serverState = ServerState::Disconnected;
        nextConnectAttemptMs = juce::Time::getMillisecondCounterHiRes() + 3000.0;
    }
    listeners.call ([] (Listener& l) { l.sessionStateChanged(); });
}

float MeonSession::getServerPingMs() const
{
    return pinger != nullptr ? pinger->lastMs.load() : -1.0f;
}

void MeonSession::setServerPingInterval (int ms)
{
    if (pinger != nullptr)
    {
        pinger->intervalMs = juce::jmax (1000, ms);
        pinger->notify();
    }
}

//==============================================================================
void MeonSession::createRoom()
{
    if (roomState != RoomState::None)
        return;
    lastJoinError.clear();
    pendingCode = generateRoomCode();
    creatingRoom = true;
    joinRequested = false;
    roomState = RoomState::Joining;
    listeners.call ([] (Listener& l) { l.sessionStateChanged(); });
    performPendingRoomAction();
}

void MeonSession::joinRoom (const juce::String& codeIn)
{
    if (roomState != RoomState::None)
        return;
    auto code = codeIn.trim().toUpperCase();
    if (! isValidRoomCode (code))
    {
        listeners.call ([] (Listener& l) { l.joinFailed (JoinFailure::InvalidCode); });
        return;
    }
    lastJoinError.clear();
    pendingCode = code;
    creatingRoom = false;
    joinRequested = false;
    roomState = RoomState::Joining;
    listeners.call ([] (Listener& l) { l.sessionStateChanged(); });
    performPendingRoomAction();
}

void MeonSession::performPendingRoomAction()
{
    if (roomState != RoomState::Joining || pendingCode.isEmpty() || joinRequested)
        return;
    if (serverState != ServerState::Connected)
    {
        start();
        return;   // 연결되면 aooClientConnected 에서 다시 시도
    }
    joinRequested = true;
    if (! processor.joinServerGroup (pendingCode, "", false))
        finishJoin (false, JoinFailure::Error, "join request failed");
}

void MeonSession::finishJoin (bool success, JoinFailure failure, const juce::String& message)
{
    if (success)
    {
        roomCode = pendingCode;
        roomState = RoomState::InRoom;
        joinedAtMs = juce::Time::getMillisecondCounterHiRes();

        AooServerConnectionInfo info;
        info.userName = userName;
        info.groupName = roomCode;
        info.groupIsPublic = false;
        info.serverHost = DEFAULT_SERVER_HOST;
        info.serverPort = DEFAULT_SERVER_PORT;
        info.timestamp = juce::Time::getCurrentTime().toMilliseconds();
        processor.addRecentServerConnectionInfo (info);

        MeonSessionLog::AudioInfo audio;
        if (audioInfoProvider)
            audio = audioInfoProvider();
        log.begin (roomCode, userName, isPlugin, audio);
        log.addEvent (creatingRoom ? "roomCreated" : "roomJoined", roomCode);
        lastSampleMs = 0.0;

        listeners.call ([] (Listener& l) { l.sessionStateChanged(); l.roomJoined(); });
    }
    else
    {
        lastJoinError = message;
        if (roomState != RoomState::None && (roomState == RoomState::Verifying || roomState == RoomState::InRoom))
            processor.leaveServerGroup (pendingCode);
        roomState = RoomState::None;
        pendingCode.clear();
        roomCode.clear();
        members.clear();
        listeners.call ([failure] (Listener& l) { l.sessionStateChanged(); l.membersChanged(); l.joinFailed (failure); });
    }
}

void MeonSession::leaveRoom()
{
    if (roomState == RoomState::None)
        return;
    const auto code = roomCode.isNotEmpty() ? roomCode : pendingCode;
    processor.leaveServerGroup (code);
    clearRoom ("leave");
}

void MeonSession::clearRoom (const juce::String& reason)
{
    if (log.isActive())
    {
        log.addEvent ("leave", reason);
        log.end (reason);
    }
    roomState = RoomState::None;
    roomCode.clear();
    pendingCode.clear();
    creatingRoom = false;
    members.clear();
    chat.clear();
    unread = 0;
    listeners.call ([] (Listener& l) { l.sessionStateChanged(); l.membersChanged(); l.chatChanged(); l.roomLeft(); });
}

//==============================================================================
int MeonSession::getMemberCount() const
{
    return 1 + (int) members.size();
}

std::vector<MeonSession::Member> MeonSession::getMembers() const
{
    std::vector<Member> out;
    for (auto& m : members)
        if (m.slot >= 0)
            out.push_back (m);
    std::sort (out.begin(), out.end(), [] (const Member& a, const Member& b) { return a.slot < b.slot; });
    return out;
}

const MeonSession::Member* MeonSession::getMemberInSlot (int slot) const
{
    for (auto& m : members)
        if (m.slot == slot)
            return &m;
    return nullptr;
}

MeonSession::Member* MeonSession::findMember (const juce::String& user)
{
    for (auto& m : members)
        if (m.userName == user)
            return &m;
    return nullptr;
}

int MeonSession::findPeerIndex (const juce::String& user) const
{
    const int n = processor.getNumberRemotePeers();
    for (int i = 0; i < n; ++i)
        if (processor.getRemotePeerUserName (i) == user)
            return i;
    return -1;
}

int MeonSession::firstFreeSlot() const
{
    for (int s = 0; s < metric::maxOthers; ++s)
        if (getMemberInSlot (s) == nullptr)
            return s;
    return -1;
}

void MeonSession::setMemberMuted (int slot, bool muted)
{
    if (auto* m = const_cast<Member*> (getMemberInSlot (slot)))
    {
        const int idx = findPeerIndex (m->userName);
        if (idx >= 0)
            processor.setRemotePeerRecvActive (idx, ! muted);
        m->muted = muted;
        listeners.call ([] (Listener& l) { l.memberStatsChanged(); });
    }
}

void MeonSession::setMemberGain (int slot, float gain)
{
    if (auto* m = const_cast<Member*> (getMemberInSlot (slot)))
    {
        const int idx = findPeerIndex (m->userName);
        if (idx >= 0)
            processor.setRemotePeerLevelGain (idx, gain);
        m->gain = gain;
    }
}

float MeonSession::meterDb (foleys::LevelMeterSource* src, int channel) const
{
    if (src == nullptr || src->getNumChannels() == 0)
        return -100.0f;
    float lin = 0.0f;
    if (channel >= 0 && channel < src->getNumChannels())
        lin = src->getMaxLevel (channel);
    else
        for (int c = 0; c < src->getNumChannels(); ++c)
            lin = juce::jmax (lin, src->getMaxLevel (c));
    return juce::Decibels::gainToDecibels (lin, -100.0f);
}

float MeonSession::getMemberLevelDb (int slot) const
{
    if (auto* m = getMemberInSlot (slot))
    {
        if (! m->connected || m->muted)
            return -100.0f;
        const int idx = findPeerIndex (m->userName);
        if (idx >= 0)
            return meterDb (processor.getRemotePeerRecvMeterSource (idx));
    }
    return -100.0f;
}

float MeonSession::getMemberPeakLevelDb (int slot) const
{
    return getMemberLevelDb (slot);
}

//==============================================================================
bool MeonSession::isMyMuted() const
{
    if (auto* param = processor.getValueTreeState().getParameter (SonobusAudioProcessor::paramMainSendMute))
        return param->getValue() > 0.5f;
    return false;
}

void MeonSession::setMyMuted (bool muted)
{
    if (auto* param = processor.getValueTreeState().getParameter (SonobusAudioProcessor::paramMainSendMute))
        param->setValueNotifyingHost (muted ? 1.0f : 0.0f);
    if (log.isActive())
        log.addEvent (muted ? "selfMute" : "selfUnmute", "");
    listeners.call ([] (Listener& l) { l.memberStatsChanged(); });
}

float MeonSession::getMyInputLevelDb (int channel) const
{
    return meterDb (&processor.getInputMeterSource(), channel);
}

float MeonSession::getMySendLevelDb() const
{
    if (isMyMuted())
        return -100.0f;
    return meterDb (&processor.getSendMeterSource());
}

//==============================================================================
void MeonSession::sendChat (const juce::String& textIn)
{
    auto text = textIn.trim();
    if (text.isEmpty() || roomState != RoomState::InRoom)
        return;
    SBChatEvent ev;
    ev.type = SBChatEvent::UserType;
    ev.from = userName;
    ev.group = roomCode;
    ev.message = text;
    processor.sendChatEvent (ev);

    ChatMessage m;
    m.kind = ChatMessage::Mine;
    m.from = getDisplayName();
    m.text = text;
    m.time = juce::Time::getCurrentTime();
    chat.push_back (m);
    listeners.call ([] (Listener& l) { l.chatChanged(); });
}

void MeonSession::addSystemChat (const juce::String& text)
{
    ChatMessage m;
    m.kind = ChatMessage::System;
    m.text = text;
    m.time = juce::Time::getCurrentTime();
    chat.push_back (m);
    listeners.call ([] (Listener& l) { l.chatChanged(); });
}

//==============================================================================
// ClientListener (네트워크 스레드) → 큐에 넣고 메시지 스레드에서 처리
void MeonSession::pushEvent (Event e)
{
    {
        const juce::ScopedLock sl (eventLock);
        pendingEvents.push_back (std::move (e));
    }
    triggerAsyncUpdate();
}

void MeonSession::aooClientConnected (SonobusAudioProcessor*, bool success, const juce::String& errmesg)
{
    Event e; e.type = Event::Connected; e.success = success; e.message = errmesg; pushEvent (e);
}
void MeonSession::aooClientDisconnected (SonobusAudioProcessor*, bool success, const juce::String& errmesg)
{
    Event e; e.type = Event::Disconnected; e.success = success; e.message = errmesg; pushEvent (e);
}
void MeonSession::aooClientGroupJoined (SonobusAudioProcessor*, bool success, const juce::String& group, const juce::String& errmesg)
{
    Event e; e.type = Event::GroupJoined; e.success = success; e.group = group; e.message = errmesg; pushEvent (e);
}
void MeonSession::aooClientGroupLeft (SonobusAudioProcessor*, bool success, const juce::String& group, const juce::String& errmesg)
{
    Event e; e.type = Event::GroupLeft; e.success = success; e.group = group; e.message = errmesg; pushEvent (e);
}
void MeonSession::aooClientPeerPendingJoin (SonobusAudioProcessor*, const juce::String& group, const juce::String& user)
{
    Event e; e.type = Event::PeerPending; e.group = group; e.user = user; pushEvent (e);
}
void MeonSession::aooClientPeerJoined (SonobusAudioProcessor*, const juce::String& group, const juce::String& user)
{
    Event e; e.type = Event::PeerJoined; e.group = group; e.user = user; pushEvent (e);
}
void MeonSession::aooClientPeerJoinFailed (SonobusAudioProcessor*, const juce::String& group, const juce::String& user)
{
    Event e; e.type = Event::PeerJoinFailed; e.group = group; e.user = user; pushEvent (e);
}
void MeonSession::aooClientPeerLeft (SonobusAudioProcessor*, const juce::String& group, const juce::String& user)
{
    Event e; e.type = Event::PeerLeft; e.group = group; e.user = user; pushEvent (e);
}
void MeonSession::aooClientError (SonobusAudioProcessor*, const juce::String& errmesg)
{
    Event e; e.type = Event::Error; e.message = errmesg; pushEvent (e);
}
void MeonSession::aooClientPeerChangedState (SonobusAudioProcessor*, const juce::String& mesg)
{
    Event e; e.type = Event::PeerState; e.message = mesg; pushEvent (e);
}
void MeonSession::sbChatEventReceived (SonobusAudioProcessor*, const SBChatEvent& chatevent)
{
    Event e; e.type = Event::Chat; e.chat = chatevent; pushEvent (e);
}

void MeonSession::handleAsyncUpdate()
{
    std::vector<Event> events;
    {
        const juce::ScopedLock sl (eventLock);
        events.swap (pendingEvents);
    }
    for (auto& e : events)
        handleEvent (e);
}

void MeonSession::handleEvent (const Event& e)
{
    const double now = juce::Time::getMillisecondCounterHiRes();

    switch (e.type)
    {
        case Event::Connected:
        {
            if (e.success)
            {
                const bool wasInRoom = roomState == RoomState::InRoom;
                serverState = ServerState::Connected;
                if (wasInRoom && log.isActive())
                    log.addEvent ("serverReconnected", "");
                listeners.call ([] (Listener& l) { l.sessionStateChanged(); });
                performPendingRoomAction();
            }
            else
            {
                serverState = ServerState::Disconnected;
                nextConnectAttemptMs = now + (e.message.containsIgnoreCase ("denied") ? 500.0 : 3000.0);
                if (roomState == RoomState::Joining)
                    finishJoin (false, JoinFailure::Error, e.message);
                listeners.call ([] (Listener& l) { l.sessionStateChanged(); });
            }
            break;
        }

        case Event::Disconnected:
        {
            serverState = ServerState::Disconnected;
            nextConnectAttemptMs = now + 3000.0;
            if (roomState == RoomState::InRoom && log.isActive())
                log.addEvent ("serverLost", e.message);
            if (roomState == RoomState::Joining || roomState == RoomState::Verifying)
                finishJoin (false, JoinFailure::Error, e.message);
            listeners.call ([] (Listener& l) { l.sessionStateChanged(); });
            break;
        }

        case Event::GroupJoined:
        {
            if (e.success)
            {
                if (roomState == RoomState::Joining && e.group == pendingCode)
                {
                    if (creatingRoom)
                    {
                        finishJoin (true, JoinFailure::None, {});
                    }
                    else
                    {
                        roomState = RoomState::Verifying;
                        verifyDeadlineMs = now + 4000.0;
                        listeners.call ([] (Listener& l) { l.sessionStateChanged(); });
                    }
                }
                // 서버 재접속 후 같은 방에 다시 들어온 경우는 그대로 둔다
            }
            else if (roomState == RoomState::Joining && e.group == pendingCode)
            {
                finishJoin (false, JoinFailure::Error, e.message);
            }
            break;
        }

        case Event::GroupLeft:
        {
            if (roomState != RoomState::None && (e.group == roomCode || e.group == pendingCode))
            {
                // 우리가 leaveRoom 을 부른 경우는 이미 정리됐다. 그 외(서버가 내보냄)는 여기서 정리.
                clearRoom ("groupLeft");
            }
            break;
        }

        case Event::PeerPending:
        case Event::PeerJoined:
        case Event::PeerJoinFailed:
        {
            if (roomState == RoomState::None || (e.group != pendingCode && e.group != roomCode))
                break;

            auto* m = findMember (e.user);
            if (m == nullptr)
            {
                Member nm;
                nm.userName = e.user;
                nm.displayName = displayNameFor (e.user);
                nm.slot = firstFreeSlot();
                nm.joinedAtMs = now;
                members.push_back (nm);
                m = &members.back();
                if (log.isActive())
                    log.addEvent ("peerPending", e.user);
            }

            if (e.type == Event::PeerJoined)
            {
                const bool wasConnected = m->connected;
                m->pending = false;
                m->connected = true;
                m->joinFailed = false;
                m->disconnectedSinceMs = 0.0;
                if (! wasConnected && roomState == RoomState::InRoom && now - joinedAtMs > 2000.0)
                    addSystemChat (m->displayName + "님이 입장했습니다");
                if (log.isActive())
                    log.addEvent ("peerJoined", e.user);
                const int idx = findPeerIndex (e.user);
                if (idx >= 0)
                    applyPeerDefaults (idx, *m);
            }
            else if (e.type == Event::PeerJoinFailed)
            {
                m->pending = false;
                m->connected = false;
                m->joinFailed = true;
                m->disconnectedSinceMs = now;
                if (log.isActive())
                    log.addEvent ("peerJoinFailed_NAT", e.user);
            }

            if (roomState == RoomState::Verifying)
            {
                if ((int) members.size() >= metric::maxOthers + 1)
                    finishJoin (false, JoinFailure::RoomFull, "room full");
                else
                    finishJoin (true, JoinFailure::None, {});
            }
            else if (roomState == RoomState::InRoom && (int) members.size() >= metric::maxOthers + 1
                     && now - joinedAtMs < 3000.0)
            {
                // 내가 6번째로 들어온 경우
                finishJoin (false, JoinFailure::RoomFull, "room full");
            }
            listeners.call ([] (Listener& l) { l.membersChanged(); });
            break;
        }

        case Event::PeerLeft:
        {
            if (auto* m = findMember (e.user))
            {
                const auto name = m->displayName;
                members.erase (std::remove_if (members.begin(), members.end(),
                                               [&e] (const Member& mm) { return mm.userName == e.user; }), members.end());
                if (roomState == RoomState::InRoom)
                    addSystemChat (name + "님이 나갔습니다");
                if (log.isActive())
                    log.addEvent ("peerLeft", e.user);
                listeners.call ([] (Listener& l) { l.membersChanged(); });
            }
            break;
        }

        case Event::Chat:
        {
            if (roomState != RoomState::InRoom || e.chat.group != roomCode)
                break;
            if (e.chat.from == userName)
                break;
            ChatMessage m;
            m.kind = ChatMessage::Other;
            m.from = displayNameFor (e.chat.from);
            m.text = e.chat.message;
            m.time = juce::Time::getCurrentTime();
            chat.push_back (m);
            ++unread;
            listeners.call ([] (Listener& l) { l.chatChanged(); });
            break;
        }

        case Event::PeerState:
            refreshStats();
            break;

        case Event::Error:
            if (log.isActive())
                log.addEvent ("error", e.message);
            break;
    }
}

//==============================================================================
void MeonSession::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    ++tickCount;

    // 서버 연결 관리 (방 밖에서만 직접 재시도; 방 안에서는 엔진의 자동 재접속을 쓴다)
    if (wantConnected && serverState == ServerState::Disconnected && roomState == RoomState::None
        && now >= nextConnectAttemptMs)
    {
        connectNow();
    }
    else if (serverState == ServerState::Connecting && now > connectDeadlineMs)
    {
        processor.disconnectFromServer();
        serverState = ServerState::Disconnected;
        nextConnectAttemptMs = now + 3000.0;
        if (roomState == RoomState::Joining)
            finishJoin (false, JoinFailure::Error, "connect timeout");
        listeners.call ([] (Listener& l) { l.sessionStateChanged(); });
    }

    // 코드 확인 시간 초과 → 잘못된 코드
    if (roomState == RoomState::Verifying && now > verifyDeadlineMs)
        finishJoin (false, JoinFailure::InvalidCode, "no peers");

    if (roomState == RoomState::InRoom)
    {
        reconcilePeers();
        if ((tickCount % 2) == 0)
            refreshStats();
        if (now - lastSampleMs >= 5000.0)
        {
            lastSampleMs = now;
            writeLogSample();
        }
    }
}

void MeonSession::reconcilePeers()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    bool changed = false;

    // 엔진에는 있는데 목록에 없는 피어 (끊겼다 다시 붙은 경우 등)
    const int n = processor.getNumberRemotePeers();
    for (int i = 0; i < n; ++i)
    {
        auto user = processor.getRemotePeerUserName (i);
        if (user.isEmpty() || findMember (user) != nullptr)
            continue;
        Member nm;
        nm.userName = user;
        nm.displayName = displayNameFor (user);
        nm.slot = firstFreeSlot();
        nm.pending = false;
        nm.connected = processor.getRemotePeerConnected (i);
        nm.joinedAtMs = now;
        members.push_back (nm);
        applyPeerDefaults (i, members.back());
        changed = true;
    }

    // 연결 상태 변화, 30초 이상 끊긴 멤버 정리
    for (auto it = members.begin(); it != members.end();)
    {
        auto& m = *it;
        const int idx = findPeerIndex (m.userName);
        if (idx >= 0)
        {
            const bool conn = processor.getRemotePeerConnected (idx);
            if (conn != m.connected && ! m.pending)
            {
                if (! conn)
                {
                    ++m.dropCount;
                    m.disconnectedSinceMs = now;
                    if (log.isActive()) log.addEvent ("peerDisconnected", m.userName);
                }
                else
                {
                    m.disconnectedSinceMs = 0.0;
                    if (log.isActive()) log.addEvent ("peerReconnected", m.userName);
                }
                m.connected = conn;
                changed = true;
            }
            if (! m.defaultsApplied && m.connected)
                applyPeerDefaults (idx, m);
        }

        if (! m.connected && ! m.pending && m.disconnectedSinceMs > 0.0 && now - m.disconnectedSinceMs > 30000.0)
        {
            if (log.isActive()) log.addEvent ("peerRemovedAfterTimeout", m.userName);
            it = members.erase (it);
            changed = true;
            continue;
        }
        ++it;
    }

    if (changed)
        listeners.call ([] (Listener& l) { l.membersChanged(); });
}

void MeonSession::refreshStats()
{
    for (auto& m : members)
    {
        const int idx = findPeerIndex (m.userName);
        if (idx < 0)
            continue;
        SonobusAudioProcessor::LatencyInfo li;
        if (processor.getRemotePeerLatencyInfo (idx, li))
        {
            m.pingMs = li.pingMs;
            m.roundtripMs = li.totalRoundtripMs;
        }
        m.jitterBufferMs = processor.getRemotePeerBufferTime (idx);
        m.dropped = processor.getRemotePeerPacketsDropped (idx);
        m.resent = processor.getRemotePeerPacketsResent (idx);
        m.received = processor.getRemotePeerPacketsReceived (idx);
        m.gain = processor.getRemotePeerLevelGain (idx);
        m.muted = ! processor.getRemotePeerRecvActive (idx);
        m.hasStats = true;
    }
    listeners.call ([] (Listener& l) { l.memberStatsChanged(); });
}

void MeonSession::writeLogSample()
{
    if (! log.isActive())
        return;
    juce::Array<juce::var> arr;
    for (auto& m : members)
    {
        arr.add (MeonSessionLog::makeObject ({
            { "user", m.userName },
            { "connected", m.connected },
            { "pending", m.pending },
            { "natFailed", m.joinFailed },
            { "pingMs", m.pingMs },
            { "roundtripMs", m.roundtripMs },
            { "jitterBufferMs", m.jitterBufferMs },
            { "packetsDropped", (juce::int64) m.dropped },
            { "packetsResent", (juce::int64) m.resent },
            { "packetsReceived", (juce::int64) m.received },
            { "dropCount", m.dropCount },
            { "gain", m.gain },
            { "mutedByMe", m.muted } }));
    }
    log.addSample (MeonSessionLog::makeObject ({
        { "t", juce::Time::getCurrentTime().toISO8601 (true) },
        { "serverConnected", serverState == ServerState::Connected },
        { "serverPingMs", getServerPingMs() },
        { "selfMuted", isMyMuted() },
        { "memberCount", getMemberCount() },
        { "members", arr } }));
}

} // namespace meon
