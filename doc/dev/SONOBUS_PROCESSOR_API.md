<!-- 자동 생성: 원본 SonoBus 1.7.2 코드 조사 결과 (MEON 개발 참고용) -->

All data gathered. Here is the reference map.

---

# SonobusAudioProcessor API Reference (fork of SonoBus 1.7.2)

**Absolute paths** (all under `/Users/seungchan/github/meon/`):

| Alias | Absolute path |
|---|---|
| `SPP.h` | `/Users/seungchan/github/meon/Source/SonobusPluginProcessor.h` |
| `SPP.cpp` | `/Users/seungchan/github/meon/Source/SonobusPluginProcessor.cpp` |
| `Types.h` | `/Users/seungchan/github/meon/Source/SonobusTypes.h` |
| `CG.h` | `/Users/seungchan/github/meon/Source/ChannelGroup.h` |
| `EP.h` | `/Users/seungchan/github/meon/Source/EffectParams.h` |
| `Ed.h` / `Ed.cpp` | `/Users/seungchan/github/meon/Source/SonobusPluginEditor.h` / `.cpp` |
| `SFW.h` | `/Users/seungchan/github/meon/Source/SonoStandaloneFilterWindow.h` |
| `AU.cpp` | `/Users/seungchan/github/meon/Source/AutoUpdater.cpp` |
| `VI.cpp` | `/Users/seungchan/github/meon/Source/VersionInfo.cpp` |

Class decl: `SPP.h:116` — `class SonobusAudioProcessor : public AudioProcessor, public AudioProcessorValueTreeState::Listener, public ChangeListener`

---

## 1. Server connection

### Constants (`SPP.h:32-35`)
```cpp
#define MAX_PEERS 32
#define MAX_CHANGROUPS 64
#define DEFAULT_SERVER_PORT 10998
#define DEFAULT_SERVER_HOST "aoo.sonobus.net"
```
Also `SPP.cpp:159` `#define DEFAULT_UDP_PORT 11000`, `SPP.cpp:42` `#define PEER_PING_INTERVAL_MS 2000.0`.

### Connect / disconnect
| Signature | Decl | Impl |
|---|---|---|
| `bool connectToServer(const String & host, int port, const String & username, const String & passwd="");` | `SPP.h:297` | `SPP.cpp:1126` |
| `bool isConnectedToServer() const;` | `SPP.h:298` | `SPP.cpp:1151` |
| `bool disconnectFromServer();` | `SPP.h:299` | `SPP.cpp:1158` |
| `double getElapsedConnectedTime() const` (inline) | `SPP.h:300` | — |
| `bool setCurrentUsername(const String & name);` (fails while connected) | `SPP.h:318` | `SPP.cpp:1117` |
| `String getCurrentUsername() const` (inline) | `SPP.h:319` | — |

`connectToServer` calls `removeAllRemotePeers()` first (unless recovering), sets `mServerEndpoint->ipaddr/port`, sets `mCurrentUsername`, then `mAooClient->connect(...)` (`SPP.cpp:1142`). Return value is only "request accepted"; actual result arrives via `aooClientConnected`.

### Recents (host/port/group/user persistence)
- `struct AooServerConnectionInfo` — `SPP.h:38-54`. Fields: `userName, userPassword, groupName, groupPassword, bool groupIsPublic, serverHost, int serverPort, int64 timestamp`. `ValueTree getValueTree() const` / `void setFromValueTree(const ValueTree&)`.
- `void addRecentServerConnectionInfo(const AooServerConnectionInfo & cinfo);` — `SPP.h:313`, impl `SPP.cpp:1187` (dedupes, sorts newest-first, caps at 10).
- `void removeRecentServerConnectionInfo(int index);` — `SPP.h:314`, impl `SPP.cpp:1222`
- `int getRecentServerConnectionInfos(Array<AooServerConnectionInfo> & retarray);` — `SPP.h:315`, impl `SPP.cpp:1209`
- `void clearRecentServerConnectionInfos();` — `SPP.h:316`, impl `SPP.cpp:1216`

### Ping / round-trip time
There is **no** server-ping / server-RTT getter. Only per-peer RTT exists (see §3). Implementation: `SPP.cpp:3402 sendPingEvent(RemotePeer*)`, `SPP.cpp:3426 handlePingEvent(EndpointState*, uint64_t tt1, uint64_t tt2, uint64_t tt3)` — sets `peer->pingTime = rtt` and pushes into `peer->smoothPingTime` (rejects >600 ms).

### Built-in server (hide for a client-only UI)
- `void startAooServer();` — `SPP.h:288`
- `void stopAooServer();` — `SPP.h:289`
- `int32_t handleServerEvents(const aoo_event ** events, int32_t n);` — `SPP.h:284`

---

## 2. Groups

| Signature | Decl | Impl |
|---|---|---|
| `bool joinServerGroup(const String & group, const String & groupsecret = "", bool isPublic=false);` | `SPP.h:305` | `SPP.cpp:1271` |
| `bool leaveServerGroup(const String & group);` | `SPP.h:306` | `SPP.cpp:1284` |
| `String getCurrentJoinedGroup() const;` | `SPP.h:307` | `SPP.cpp:1303` |
| `void setAutoconnectToGroupPeers(bool flag);` | `SPP.h:302` | `SPP.cpp:1243` |
| `bool getAutoconnectToGroupPeers() const` (inline) | `SPP.h:303` | — |

There is **no** `setAutoconnectToGroup`. The closest names are `setAutoconnectToGroupPeers` (auto-connect to peers of the joined group; default `true`, `SPP.h:1089`) and the parameter `paramAutoReconnectLast` / `getAutoReconnectToLast()` (`SPP.h:574`) + `bool reconnectToMostRecent()` (`SPP.h:942`, impl `SPP.cpp:8847`).

### Join failure reporting
There is **no** `aooClientGroupJoinFailed`. Failure is reported through the same success/fail callback:
```cpp
virtual void aooClientGroupJoined(SonobusAudioProcessor *comp, bool success, const String & group, const String & errmesg="") {}
```
`SPP.h:667`; fired at `SPP.cpp:4243` with `success = (e->result > 0)` and `errmesg = String::fromUTF8(e->errormsg)`.

### Public group listing (candidates to hide)
- `struct AooPublicGroupInfo` — `SPP.h:56-64`: `String groupName; int activeCount = 0; int64 timestamp = 0;`
- `bool setWatchPublicGroups(bool flag);` — `SPP.h:308`, impl `SPP.cpp:1249` (calls `mAooClient->group_watch_public`)
- `bool getWatchPublicGroups() const` — `SPP.h:309`
- `int getPublicGroupInfos(Array<AooPublicGroupInfo> & retarray);` — `SPP.h:311`, impl `SPP.cpp:1230`
- Callbacks `aooClientPublicGroupModified` (`SPP.h:669`) and `aooClientPublicGroupDeleted` (`SPP.h:670`)

### Peer cap
`MAX_PEERS 32` (`SPP.h:32`) — used for the `mRemoteSendMatrix[MAX_PEERS][MAX_PEERS]` (`SPP.h:1164`). It is not enforced as a group join limit in the processor.

---

## 3. Peers

### Identity / presence
| Signature | Decl | Impl |
|---|---|---|
| `int getNumberRemotePeers() const;` | `SPP.h:339` | `SPP.cpp:4695` |
| `String getRemotePeerUserName(int index) const;` | `SPP.h:366` | `SPP.cpp:4878` |
| `void setRemotePeerUserName(int index, const String & name);` | `SPP.h:365` | `SPP.cpp:4869` |
| `bool isRemotePeerUserInGroup(const String & name) const;` | `SPP.h:363` | `SPP.cpp:4888` |
| `bool getRemotePeerConnected(int index) const;` | `SPP.h:634` | `SPP.cpp:5853` |
| `void setRemotePeerConnected(int index, bool active);` | `SPP.h:633` | `SPP.cpp:5844` |
| `bool getRemotePeerAddressInfo(int index, String & rethost, int & retport) const;` | `SPP.h:636` | `SPP.cpp:5864` |

- **`isRemotePeerConnected` does not exist** — use `getRemotePeerConnected(int)` (`SPP.h:634`).
- **Remote peer "compact" name does not exist.** No `getRemotePeerCompactName` / compact-name API anywhere in the processor. The only "compact" occurrence in `SPP.cpp` is `SPP.cpp:2301` (a comment about a compact AOO data message). Peer display density is controlled by `enum PeerDisplayMode { PeerDisplayModeFull = 0, PeerDisplayModeMinimal }` (`SPP.h:155-158`) with `getPeerDisplayMode()` / `setPeerDisplayMode(PeerDisplayMode)` (`SPP.h:777-778`).
- **Nickname uniqueness is not enforced by the processor.** `isRemotePeerUserInGroup(const String&)` (`SPP.h:363`) is the only helper; it does a linear `userName ==` compare over `mRemotePeers` (`SPP.cpp:4888-4897`).

### Latency / ping
```cpp
struct LatencyInfo            // SPP.h:453-463
{
    float pingMs = 0.0f;
    float totalRoundtripMs = 0.0f;
    float outgoingMs = 0.0f;
    float incomingMs = 0.0f;
    float jitterMs = 0.0f;
    bool isreal = false;
    bool estimated = false;
    bool legacy = false;
};
bool getRemotePeerLatencyInfo(int index, LatencyInfo & retinfo) const;   // SPP.h:465, impl SPP.cpp:5599
```
- **`getRemotePeerPingMs` and `getRemotePeerTotalLatency` do not exist.** Use `getRemotePeerLatencyInfo(...)` and read `retinfo.pingMs` / `retinfo.totalRoundtripMs`. `pingMs` is sourced from `remote->smoothPingTime.xbar` (`SPP.cpp:5669`); the roundtrip is computed at `SPP.cpp:5682-5689` (new-style, `hasRemoteInfo`) or `SPP.cpp:5693-5722` (legacy).
- Latency test: `bool startRemotePeerLatencyTest(int index, float durationsec = 1.0);` (`SPP.h:467`, impl `SPP.cpp:5762`), `bool stopRemotePeerLatencyTest(int index);` (`SPP.h:468`), `bool isRemotePeerLatencyTestActive(int index);` (`SPP.h:469`, impl `SPP.cpp:5730`).

### Jitter buffer
```cpp
enum AutoNetBufferMode {            // SPP.h:123-128
    AutoNetBufferModeOff = 0,
    AutoNetBufferModeAutoIncreaseOnly,
    AutoNetBufferModeAutoFull,
    AutoNetBufferModeInitAuto
};
void setRemotePeerBufferTime(int index, float bufferMs);                                        // SPP.h:410, SPP.cpp:5274
float getRemotePeerBufferTime(int index) const;                                                 // SPP.h:411, SPP.cpp:5310
void setRemotePeerAutoresizeBufferMode(int index, AutoNetBufferMode flag);                      // SPP.h:413, SPP.cpp:5324
AutoNetBufferMode getRemotePeerAutoresizeBufferMode(int index, bool & initCompleted) const;     // SPP.h:414, SPP.cpp:5348
void setAutoresizeBufferDropRateThreshold(float);                                               // SPP.h:417, SPP.cpp:5360
float getAutoresizeBufferDropRateThreshold() const;                                             // SPP.h:418 (inline)
bool getRemotePeerReceiveBufferFillRatio(int index, float & retratio, float & retstddev) const; // SPP.h:421, SPP.cpp:5367
void setDefaultAutoresizeBufferMode(AutoNetBufferMode flag);                                    // SPP.h:569, SPP.cpp:1398
AutoNetBufferMode getDefaultAutoresizeBufferMode() const;                                       // SPP.h:570 (inline)
```

### Packet-loss / dropout / resend statistics
```cpp
int64_t getRemotePeerPacketsReceived(int index) const;   // SPP.h:440, SPP.cpp:5506
int64_t getRemotePeerPacketsSent(int index) const;       // SPP.h:441, SPP.cpp:5516
int64_t getRemotePeerBytesReceived(int index) const;     // SPP.h:443, SPP.cpp:5536
int64_t getRemotePeerBytesSent(int index) const;         // SPP.h:444, SPP.cpp:5526
int64_t getRemotePeerPacketsDropped(int index) const;    // SPP.h:446, SPP.cpp:5546
int64_t getRemotePeerPacketsResent(int index) const;     // SPP.h:447, SPP.cpp:5556
void    resetRemotePeerPacketStats(int index);           // SPP.h:448, SPP.cpp:5586
bool getRemotePeerSafetyMuted(int index) const;          // SPP.h:450, SPP.cpp:5566
bool getRemotePeerBlockedUs(int index) const;            // SPP.h:451, SPP.cpp:5576
```
- **`getRemotePeerRecvAudioStats` and `getRemotePeerRecvPacketDropCount` do not exist.** Closest: `getRemotePeerPacketsDropped` / `getRemotePeerPacketsReceived` / `getRemotePeerReceiveBufferFillRatio`.

### Volume / pan / mute / solo / allow
```cpp
void  setRemotePeerLevelGain(int index, float levelgain);           // SPP.h:341, SPP.cpp:4700
float getRemotePeerLevelGain(int index) const;                      // SPP.h:342, SPP.cpp:4709
void  setRemotePeerChannelGain(int index, int changroup, float levelgain);          // SPP.h:344
float getRemotePeerChannelGain(int index, int changroup) const;                     // SPP.h:345
void  setRemotePeerChannelPan(int index, int changroup, int chan, float pan);       // SPP.h:347, SPP.cpp:4900
float getRemotePeerChannelPan(int index, int changroup, int chan) const;            // SPP.h:348
void  setRemotePeerChannelMuted(int index, int changroup, bool muted);              // SPP.h:350
bool  getRemotePeerChannelMuted(int index, int changroup) const;                    // SPP.h:351
void  setRemotePeerChannelSoloed(int index, int changroup, bool soloed);            // SPP.h:353
bool  getRemotePeerChannelSoloed(int index, int changroup) const;                   // SPP.h:354
void  setRemotePeerChannelReverbSend(int index, int changroup, float rgain);        // SPP.h:356
float getRemotePeerChannelReverbSend(int index, int changroup);                     // SPP.h:357
void  setRemotePeerPolarityInvert(int index, int changroup, bool invert);           // SPP.h:359
bool  getRemotePeerPolarityInvert(int index, int changroup);                        // SPP.h:360

void setRemotePeerSendActive(int index, bool active);                   // SPP.h:424, SPP.cpp:5818
bool getRemotePeerSendActive(int index) const;                          // SPP.h:425, SPP.cpp:5834
void setRemotePeerRecvActive(int index, bool active);                   // SPP.h:427, SPP.cpp:5381
bool getRemotePeerRecvActive(int index) const;                          // SPP.h:428, SPP.cpp:5407
void setRemotePeerSendAllow(int index, bool allow, bool cached=false);  // SPP.h:430, SPP.cpp:5417
bool getRemotePeerSendAllow(int index, bool cached=false) const;        // SPP.h:431, SPP.cpp:5436
void setRemotePeerRecvAllow(int index, bool allow, bool cached=false);  // SPP.h:433, SPP.cpp:5446
bool getRemotePeerRecvAllow(int index, bool cached=false) const;        // SPP.h:434, SPP.cpp:5465
void setRemotePeerSoloed(int index, bool soloed);                       // SPP.h:436, SPP.cpp:5475
bool getRemotePeerSoloed(int index) const;                              // SPP.h:437, SPP.cpp:5495
bool isAnythingSoloed() const;                                          // SPP.h:653 (inline)
```

### Per-peer meters (foleys)
```cpp
foleys::LevelMeterSource * getRemotePeerRecvMeterSource(int index);   // SPP.h:630, SPP.cpp:2176
foleys::LevelMeterSource * getRemotePeerSendMeterSource(int index);   // SPP.h:631, SPP.cpp:2184
```
Header marks these `// danger` (`SPP.h:629`). Backing fields: `RemotePeer::sendMeterSource` / `recvMeterSource` (`SPP.cpp:337-338`).

### Peer channel groups / channel counts
```cpp
int  getRemotePeerChannelGroupCount(int index) const;                  // SPP.h:369
void setRemotePeerChannelGroupCount(int index, int count);             // SPP.h:370
int  getRemotePeerRecvChannelCount(int index) const;                   // SPP.h:372, SPP.cpp:5066
void setRemotePeerChannelGroupStartAndCount(int index, int changroup, int start, int count);          // SPP.h:377
bool getRemotePeerChannelGroupStartAndCount(int index, int changroup, int & retstart, int & retcount);// SPP.h:378
void setRemotePeerChannelGroupDestStartAndCount(int index, int changroup, int start, int count);      // SPP.h:380
bool getRemotePeerChannelGroupDestStartAndCount(int index, int changroup, int & retstart, int & retcount); // SPP.h:381
void setRemotePeerChannelGroupSendMainMix(int index, int changroup, bool mainmix);   // SPP.h:383
bool getRemotePeerChannelGroupSendMainMix(int index, int changroup);                 // SPP.h:384
bool insertRemotePeerChannelGroup(int index, int atgroup, int chstart, int chcount); // SPP.h:386
bool removeRemotePeerChannelGroup(int index, int atgroup);                           // SPP.h:387
bool copyRemotePeerChannelGroup(int index, int fromgroup, int togroup);              // SPP.h:389
String getRemotePeerChannelGroupName(int index, int changroup) const;                // SPP.h:391
void   setRemotePeerChannelGroupName(int index, int changroup, const String & name); // SPP.h:392
int  getRemotePeerNominalSendChannelCount(int index) const;            // SPP.h:395
void setRemotePeerNominalSendChannelCount(int index, int numchans);    // SPP.h:396
int  getRemotePeerOverrideSendChannelCount(int index) const;           // SPP.h:398
void setRemotePeerOverrideSendChannelCount(int index, int numchans);   // SPP.h:399
int  getRemotePeerActualSendChannelCount(int index) const;             // SPP.h:401
bool getLayoutFormatChangedForRemotePeer(int index) const;             // SPP.h:406
void restoreLayoutFormatForRemotePeer(int index);                      // SPP.h:407
```

### Audio codec formats
```cpp
enum AudioCodecFormatCodec { CodecPCM = 0, CodecOpus };     // SPP.h:130

struct AudioCodecFormatInfo {                                // SPP.h:160-175
    AudioCodecFormatInfo() {}
    AudioCodecFormatInfo(int bitdepth_);                                                   // PCM ctor
    AudioCodecFormatInfo(int bitrate_, int complexity_, int signaltype, int minblocksize=120); // Opus ctor
    void computeName();
    String name;
    AudioCodecFormatCodec codec;
    int bitdepth = 2;   // bytes (PCM)
    int bitrate = 0;    // opus
    int complexity = 0;
    int signal_type = 0;
    int min_preferred_blocksize = 120;
};

int  getNumberAudioCodecFormats() const;                             // SPP.h:554 (inline: mAudioFormats.size())
void setDefaultAudioCodecFormat(int formatIndex);                    // SPP.h:556, SPP.cpp:1388
int  getDefaultAudioCodecFormat() const;                             // SPP.h:557 (inline)
void setChangingDefaultAudioCodecSetsExisting(bool flag);            // SPP.h:559 (inline)
bool getChangingDefaultAudioCodecSetsExisting() const;               // SPP.h:560 (inline)
void setChangingDefaultRecvAudioCodecSetsExisting(bool flag);        // SPP.h:562 (inline)
bool getChangingDefaultRecvAudioCodecSetsExisting() const;           // SPP.h:563 (inline)
String getAudioCodeFormatName(int formatIndex) const;                // SPP.h:566, SPP.cpp:1372
bool   getAudioCodeFormatInfo(int formatIndex, AudioCodecFormatInfo & retinfo) const; // SPP.h:567, SPP.cpp:1380
void setRemotePeerAudioCodecFormat(int index, int formatIndex);      // SPP.h:611, SPP.cpp:1406
int  getRemotePeerAudioCodecFormat(int index) const;                 // SPP.h:612, SPP.cpp:1431
bool getRemotePeerReceiveAudioCodecFormat(int index, AudioCodecFormatInfo & retinfo) const; // SPP.h:615
bool setRequestRemotePeerSendAudioCodecFormat(int index, int formatIndex);                  // SPP.h:616
int  getRequestRemotePeerSendAudioCodecFormat(int index) const;  // -1 = no preference       // SPP.h:617
int  getRemotePeerSendPacketsize(int index) const;                   // SPP.h:619
void setRemotePeerSendPacketsize(int index, int psize);              // SPP.h:620
```
**Format table and indexes** (`SPP.cpp:1329-1350`, `initFormats()`), in order, index 0..10:

| idx | entry |
|---|---|
| 0 | Opus 16000 bps/ch, complexity 10, blocksize 960 |
| 1 | Opus 24000 |
| 2 | Opus 48000 |
| 3 | Opus 64000 |
| 4 | Opus 96000 (**default**, `mDefaultAudioFormatIndex = 4`, `SPP.cpp:1349` and `SPP.h:1151`) |
| 5 | Opus 128000 |
| 6 | Opus 160000 |
| 7 | Opus 256000 |
| 8 | PCM 16 bit (`bitdepth=2`) |
| 9 | PCM 24 bit (`bitdepth=3`) |
| 10 | PCM 32 bit float (`bitdepth=4`) |

Names come from `AudioCodecFormatInfo::computeName()` (`SPP.cpp:1308`): Opus → `"%d kbps/ch"`, PCM → `"PCM 16 bit"` / `"PCM 24 bit"` / …

### Peer ordering / view state
```cpp
int  getRemotePeerOrderPriority(int index) const;     // SPP.h:622
void setRemotePeerOrderPriority(int index, int priority); // SPP.h:623
bool getRemotePeerViewExpanded(int index) const;      // SPP.h:403, SPP.cpp:5088
void setRemotePeerViewExpanded(int index, bool expanded); // SPP.h:404, SPP.cpp:5099
```

---

## 4. Local input / output

### Input channel groups
```cpp
void setInputGroupCount(int count);                       // SPP.h:500, SPP.cpp:1872
int  getInputGroupCount() const;                          // SPP.h:501 (inline: mInputChannelGroupCount)
void setInputGroupChannelStartAndCount(int changroup, int start, int count);               // SPP.h:503
bool getInputGroupChannelStartAndCount(int changroup, int & retstart, int & retcount);     // SPP.h:504
void setInputGroupChannelDestStartAndCount(int changroup, int start, int count);           // SPP.h:506
bool getInputGroupChannelDestStartAndCount(int changroup, int & retstart, int & retcount); // SPP.h:507
bool insertInputChannelGroup(int atgroup, int chstart, int chcount);   // SPP.h:516
bool removeInputChannelGroup(int atgroup);                             // SPP.h:517
bool moveInputChannelGroupTo(int atgroup, int togroup);                // SPP.h:518
void   setInputGroupName(int changroup, const String & name);  // SPP.h:520
String getInputGroupName(int changroup);                       // SPP.h:521
void  setInputChannelPan(int changroup, int chan, float pan);  // SPP.h:523
float getInputChannelPan(int changroup, int chan);             // SPP.h:524
void  setInputGroupGain(int changroup, float gain);            // SPP.h:526
float getInputGroupGain(int changroup);                        // SPP.h:527
void  setInputMonitor(int changroup, float mgain);             // SPP.h:529   <- monitoring level
float getInputMonitor(int changroup);                          // SPP.h:530
void  setInputGroupMuted(int changroup, bool muted);           // SPP.h:532
bool  getInputGroupMuted(int changroup);                       // SPP.h:533
void  setInputGroupSoloed(int changroup, bool muted);          // SPP.h:535
bool  getInputGroupSoloed(int changroup);                      // SPP.h:536
void  setInputReverbSend(int changroup, float rgain, bool input=false);  // SPP.h:539
float getInputReverbSend(int changroup, bool input=false);              // SPP.h:540
void  setInputPolarityInvert(int changroup, bool invert);      // SPP.h:542
bool  getInputPolarityInvert(int changroup);                   // SPP.h:543
bool getInputEffectsActive(int changroup) const;               // SPP.h:549 (inline)
bool getInputMonitorEffectsActive(int changroup) const;        // SPP.h:551 (inline)
```

### Main gain / mute
- **There is no `setMainSendMute()` method.** Main send mute, main recv mute, main in mute, main monitor solo, in-gain, dry and wet are all **APVTS parameters only**. Typical UI access pattern (from `Ed.cpp:3254`):
  `processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainSendMute)->setValueNotifyingHost(0.0);`
  or via attachment (`Ed.cpp:679`).
- Reverb/metronome/file-playback do have direct setters (`SPP.h:692-732`).

### Meters
```cpp
foleys::LevelMeterSource & getInputMeterSource()        { return inputMeterSource; }      // SPP.h:643
foleys::LevelMeterSource & getPostInputMeterSource()    { return postinputMeterSource; }  // SPP.h:644
foleys::LevelMeterSource & getSendMeterSource()         { return sendMeterSource; }       // SPP.h:645
foleys::LevelMeterSource & getOutputMeterSource()       { return outputMeterSource; }     // SPP.h:646
foleys::LevelMeterSource & getFilePlaybackMeterSource() { return filePlaybackMeterSource; } // SPP.h:648
foleys::LevelMeterSource & getMetronomeMeterSource()    { return metMeterSource; }        // SPP.h:649
```
Members declared `SPP.h:1074-1079`.

### Send-channel config
```cpp
int getSendChannels() const;              // SPP.h:330 (0 = match inputs, 1 = mono, 2 = stereo)
int getActiveSendChannelCount() const;    // SPP.h:641
bool isAnythingRoutedToPeer(int index) const;  // SPP.h:651, SPP.cpp:6295
bool getPatchMatrixValue(int srcindex, int destindex) const;        // SPP.h:638
void setPatchMatrixValue(int srcindex, int destindex, bool value);  // SPP.h:639
```

---

## 5. Chat

```cpp
struct SBChatEvent {                       // SPP.h:67-86
    enum EventType { SelfType=0, UserType, SystemType };
    SBChatEvent() {};
    SBChatEvent(EventType type_, const String & group_, const String & from_,
                const String &targets_, const String & tags_, const String & mesg_);
    EventType type = UserType;
    String group;
    String from;
    String targets;    // '|'-separated usernames; empty = broadcast
    String tags;
    String message;
};
typedef Array<SBChatEvent, CriticalSection> SBChatEventList;   // SPP.h:89

bool sendChatEvent(const SBChatEvent & event);                              // SPP.h:800, SPP.cpp:2862
Array<SBChatEvent, CriticalSection> & getAllChatEvents() { return mAllChatEvents; }  // SPP.h:809
```
- **`sendChatMessage` does not exist** — the sender is `sendChatEvent(const SBChatEvent&)`.
- **`aooClientChatEvent` does not exist** — the receive callback is `virtual void sbChatEventReceived(SonobusAudioProcessor *comp, const SBChatEvent & chatevent) {}` (`SPP.h:678`), fired at `SPP.cpp:2709`.
- **History storage**: `SBChatEventList mAllChatEvents;` (`SPP.h:1058`). Received messages are appended in the processor at `SPP.cpp:2708` (skipped when `isAddressBlocked(endpoint->ipaddr)`); *sent/self* messages are appended by the view, not the processor — see `ChatView::addNewChatMessage` (`/Users/seungchan/github/meon/Source/ChatView.cpp:503-511`). History is **not** persisted to disk.
- Wire message: `#define SONOBUS_MSG_CHAT "/chat"` (`SPP.cpp:2429`), payload `s:groupname s:from s:targets s:tags s:message`.
- UI prefs: `setLastChatWidth/getLastChatWidth` (`SPP.h:801-802`), `setLastChatShown/getLastChatShown` (`SPP.h:803-804`), `setChatUseFixedWidthFont/getChatUseFixedWidthFont` (`SPP.h:805-806`), `setChatFontSizeOffset/getChatFontSizeOffset` (`SPP.h:807-808`).

---

## 6. Parameters (AudioProcessorValueTreeState)

`AudioProcessorValueTreeState& getValueTreeState();` — `SPP.h:236`. Member `AudioProcessorValueTreeState mState;` (`SPP.h:1268`), tree type id `"SonoBusAoO"` (`SPP.cpp:603`).
`void parameterChanged (const String &parameterID, float newValue) override;` — `SPP.h:226`.

All IDs are `static String` members declared `SPP.h:238-275`, defined `SPP.cpp:44-81`:

| Member (decl line) | ID string | def line | Type / range (`SPP.cpp` ctor list, 605-697) |
|---|---|---|---|
| `paramInGain` (238) | `"ingain"` | 44 | Float 0..4, skew 0.33, "In Gain" |
| `paramInMonitorMonoPan` (239) | `"inmonmonopan"` | 46 | Float -1..1 |
| `paramInMonitorPan1` (240) | `"inmonpan1"` | 47 | Float -1..1 |
| `paramInMonitorPan2` (241) | `"inmonpan2"` | 48 | Float -1..1 |
| `paramDry` (242) | `"dry"` | 45 | Float 0..1 skew 0.5, "Dry Level" |
| `paramWet` (243) | `"wet"` | 49 | Float 0..2 skew 0.5, "Output Level" |
| `paramSendChannels` (244) | `"sendchannels"` | 61 | Choice `{"Match # Inputs","Send Mono","Send Stereo"}` |
| `paramBufferTime` (245) | `"buffertime"` | 50 | **declared but never added as a parameter** |
| `paramDefaultAutoNetbuf` (246) | `"defnetauto"` | 52 | Choice `{"Off","Auto-Increase","Auto-Full","Initial-Auto"}` |
| `paramDefaultNetbufMs` (247) | `"defnetbuf"` | 51 | Float 0..mMaxBufferTime, step .001, skew .5 |
| `paramDefaultSendQual` (248) | `"defsendqual"` | 53 | Int 0..14 (format index) |
| `paramMainSendMute` (249) | `"mastsendmute"` | 54 | Bool |
| `paramMainRecvMute` (250) | `"mastrecvmute"` | 55 | Bool |
| `paramMetEnabled` (251) | `"metenabled"` | 58 | Bool |
| `paramMetGain` (252) | `"metgain"` | 59 | Float 0..1 skew .5 |
| `paramMetTempo` (253) | `"mettempo"` | 60 | Float 10..400 step 1 skew .5 |
| `paramSendMetAudio` (254) | `"sendmetaudio"` | 62 | Bool |
| `paramSendFileAudio` (255) | `"sendfileaudio"` | 63 | Bool |
| `paramSendSoundboardAudio` (256) | `"sendsoundboardaudio"` | 64 | Bool |
| `paramHearLatencyTest` (257) | `"hearlatencytest"` | 65 | Bool |
| `paramMetIsRecorded` (258) | `"metisrecorded"` | 66 | Bool |
| `paramMainReverbEnabled` (259) | `"mainreverbenabled"` | 67 | Bool |
| `paramMainReverbLevel` (260) | `"nmainreverblevel"` *(note leading `n`)* | 68 | Float 0..1 skew .4 |
| `paramMainReverbSize` (261) | `"mainreverbsize"` | 69 | Float 0..1 |
| `paramMainReverbDamping` (262) | `"mainreverbdamp"` | 70 | Float 0..1 |
| `paramMainReverbPreDelay` (263) | `"mainreverbpredelay"` | 71 | Float 0..100 ms |
| `paramMainReverbModel` (264) | `"mainreverbmodel"` | 72 | Choice `{"Freeverb","MVerb","Zita"}` |
| `paramDynamicResampling` (265) | `"dynamicresampling"` | 73 | Bool |
| `paramMainInMute` (266) | `"mastinmute"` | 56 | Bool |
| `paramMainMonitorSolo` (267) | `"mastmonsolo"` | 57 | Bool |
| `paramAutoReconnectLast` (268) | `"reconnectlast"` | 74 | Bool |
| `paramDefaultPeerLevel` (269) | `"defPeerLevel"` | 75 | Float 0..1 skew .5 |
| `paramSyncMetToHost` (270) | `"syncMetHost"` | 76 | Bool |
| `paramSyncMetToFilePlayback` (271) | `"syncMetFile"` | 77 | Bool |
| `paramInputReverbLevel` (272) | `"inreverblevel"` | 78 | Float 0..1 skew .4 |
| `paramInputReverbSize` (273) | `"inreverbsize"` | 79 | Float 0..1 |
| `paramInputReverbDamping` (274) | `"inreverbdamp"` | 80 | Float 0..1 |
| `paramInputReverbPreDelay` (275) | `"inreverbpredelay"` | 81 | Float 0..100 ms |

Declaration site: the APVTS initialiser list in the constructor, `SPP.cpp:603-697`; listener registration `SPP.cpp:699-737`. Cached parameter pointers: `mDefaultAutoNetbufModeParam` (`SPP.cpp:765`), `mDefaultAudioFormatParam` (`SPP.cpp:766`), `mTempoParameter` (`SPP.cpp:781`).

**No compressor / expander / EQ APVTS parameters exist.** Those effects are configured through plain structs, not the value tree: `SonoAudio::CompressorParams` and `SonoAudio::ParametricEqParams` and `SonoAudio::DelayParams` (`EP.h:11-51`), set/get via `setInputCompressorParams` (`SPP.h:488`), `setInputExpanderParams` (`SPP.h:491`), `setInputLimiterParams` (`SPP.h:494`), `setInputEqParams` (`SPP.h:545`), `setInputMonitorDelayParams` (`SPP.h:511`), and the peer variants `SPP.h:475-482`. They are persisted via `ChannelGroupParams::getValueTree()` (`CG.h:29`).

---

## 7. Options persistence

```cpp
void getStateInformation (MemoryBlock& destData) override;                  // SPP.h:222, SPP.cpp:8604
void setStateInformation (const void* data, int sizeInBytes) override;      // SPP.h:223, SPP.cpp:8799
void getStateInformationWithOptions(MemoryBlock& destData, bool includecache=true,
                                    bool includeInputGroups = true, bool xmlformat=false); // SPP.h:229, SPP.cpp:8490
void setStateInformationWithOptions (const void* data, int sizeInBytes, bool includecache=true,
                                    bool includeInputGroups = true, bool xmlformat=false); // SPP.h:230, SPP.cpp:8610
bool saveCurrentAsDefaultPluginSettings();   // SPP.h:232, SPP.cpp:8807  -> <support>/PluginDefault.xml
void resetDefaultPluginSettings();           // SPP.h:233, SPP.cpp:8827
bool loadDefaultPluginSettings();            // SPP.h:234, SPP.cpp:8815
File getSupportDir() const;                  // SPP.h:822  (macOS: ~/Library/Application Support/SonoBus)
```

### Top-level ValueTree child keys (`SPP.cpp:83-152`)
`RecentConnections` (83) / item `ServerConnectionInfo` (84); `ExtraState` (86); `InputChannelGroups` (130); `ExtraChannelGroups` (131); `ChannelGroups` (132); `MultiChannelGroups` (133); `ChannelGroup` (134); `ChannelLayouts` (139); `PeerStateCacheMap` (142) / item `PeerStateCache` (143); `BlockedAddresses` (123) with `Address`/`value` (124-125); `CompressorState` (116), `ExpanderState` (117), `LimiterState` (118), `ParametricEqState` (119).

### `ExtraState` property keys (write `SPP.cpp:8511-8549`, read `SPP.cpp:8640-8723`)
| Key constant (`SPP.cpp` line) | String | Backing accessor |
|---|---|---|
| `useSpecificUdpPortKey` (87) | `"UseUdpPort"` | `setUseSpecificUdpPort` / `getUseSpecificUdpPort` |
| `changeQualForAllKey` (88) | `"ChangeQualForAll"` | `setChangingDefaultAudioCodecSetsExisting` |
| `changeRecvQualForAllKey` (89) | `"ChangeRecvQualForAll"` | `setChangingDefaultRecvAudioCodecSetsExisting` |
| `defRecordOptionsKey` (90) | `"DefaultRecordingOptions"` | `setDefaultRecordingOptions` |
| `defRecordFormatKey` (91) | `"DefaultRecordingFormat"` | `setDefaultRecordingFormat` |
| `defRecordBitsKey` (92) | `"DefaultRecordingBitsPerSample"` | `setDefaultRecordingBitsPerSample` |
| `recordSelfPreFxKey` (93) | `"RecordSelfPreFx"` | `setSelfRecordingPreFX` |
| `recordSelfSilenceMutedKey` (94) | `"RecordSelfSilenceWhenMuted"` | `setSelfRecordingSilenceWhenMuted` |
| `recordFinishOpenKey` (95) | `"RecordFinishOpen"` | `setRecordFinishOpens` |
| `defRecordDirKey` (96) / `defRecordDirURLKey` (97) | `"DefaultRecordDir"` / `"DefaultRecordDirURL"` | `setDefaultRecordingDirectory` |
| `lastBrowseDirKey` (98) | `"LastBrowseDir"` | `setLastBrowseDirectory` |
| `sliderSnapKey` (99) | `"SliderSnapToMouse"` | `setSlidersSnapToMousePosition` |
| `disableShortcutsKey` (100) | `"DisableKeyShortcuts"` | `setDisableKeyboardShortcuts` |
| `peerDisplayModeKey` (101) | `"PeerDisplayMode"` | `setPeerDisplayMode` |
| `lastChatWidthKey` (102) | `"lastChatWidth"` | `setLastChatWidth` |
| `lastChatShownKey` (103) | `"lastChatShown"` | `setLastChatShown` |
| `lastSoundboardWidthKey` (104) | `"lastSoundboardWidth"` | `setLastSoundboardWidth` |
| `lastSoundboardShownKey` (105) | `"lastSoundboardShown"` | `setLastSoundboardShown` |
| `chatUseFixedWidthFontKey` (106) | `"chatFixedWidthFont"` | `setChatUseFixedWidthFont` |
| `chatFontSizeOffsetKey` (107) | `"chatFontSizeOffset"` | `setChatFontSizeOffset` |
| `linkMonitoringDelayTimesKey` (108) | `"linkMonDelayTimes"` | `setLinkMonitoringDelayTimes` |
| **`lastUsernameKey` (121)** | **`"lastUsername"`** | `mCurrentUsername` — written `SPP.cpp:8541`, read `SPP.cpp:8707` |
| `langOverrideCodeKey` (109) | `"langOverrideCode"` | `setLanguageOverrideCode` |
| `useUnivFontKey` (110) | `"useUnivFont"` | `setUseUniversalFont` |
| `lastWindowWidthKey`/`lastWindowHeightKey` (111-112) | `"lastWindowWidth"` / `"lastWindowHeight"` | `setLastPluginBounds` |
| `autoresizeDropRateThreshKey` (113) | `"autoDropRateThreshNew"` | `setAutoresizeBufferDropRateThreshold` |
| `reconnectServerLossKey` (114) | `"reconnServLoss"` | `setReconnectAfterServerLoss` |
| `videoLinkInfoKey` (8454) | `"VideoLinkInfo"` | `mVideoLinkInfo` (child tree) |

### Last group / last server
There are **no** dedicated "last group"/"last server" keys. Last group+server+password are the **first element of `RecentConnections`** (sorted newest-first). `reconnectToMostRecent()` (`SPP.cpp:8847`) reads `recents.getReference(0)`. Only the username has its own key (`lastUsername`).

### Default codec / auto-jitter defaults
Persisted as **APVTS parameters**, not `ExtraState` keys:
- default jitter mode → `paramDefaultAutoNetbuf` = `"defnetauto"` (`SPP.cpp:52`). The name you guessed, `defaultAutoNetbuf`, does not exist.
- default jitter ms → `paramDefaultNetbufMs` = `"defnetbuf"` (`SPP.cpp:51`).
- default send quality/codec → `paramDefaultSendQual` = `"defsendqual"` (`SPP.cpp:53`); `defaultSendQual` does not exist.

### UDP port
```cpp
void setUseSpecificUdpPort(int port);   // SPP.h:294, SPP.cpp:893   (0 = system chooses)
int  getUseSpecificUdpPort() const;     // SPP.h:295 (inline)
int  getUdpLocalPort() const;           // SPP.h:326 (inline: mUdpLocalPort)
IPAddress getLocalIPAddress() const;    // SPP.h:327 (inline)
void initializeAoo(int udpPort=0);      // SPP.h:861, SPP.cpp:905 (private)
```

### Peer cache / global state
- `PeerStateCache` struct — `SPP.h:835-853`; `PeerStateCacheMap` = `std::map<String, PeerStateCache>` keyed by peer name (`SPP.h:856`). Per-peer keys: `peerNameKey "name"`, `peerLevelKey "level"`, `peerNetbufKey "netbuf"`, `peerNetbufAutoKey "netbufauto"`, `peerSendFormatKey "sendformat"`, `peerOrderPriorityKey "orderpriority"` (`SPP.cpp:144-152`); serialiser at `SPP.cpp:8861`.
- `void loadGlobalState(); bool storeGlobalState();` — `SPP.h:933-934`, impl `SPP.cpp:8990` / `SPP.cpp:9006`. File: `<supportDir>/GlobalState.xml`, root `"SonobusGlobalState"` (`SPP.cpp:602`). Currently only used for the IP block list.

---

## 8. Update check / version check / sonobus.net URLs

- `void LatestVersionCheckerAndUpdater::checkForNewVersion (bool showAlerts);` — decl `/Users/seungchan/github/meon/Source/AutoUpdater.h:19`, impl `AU.cpp:48` (starts a `Thread`).
- Network call site: `VI.cpp:70` —
  `URL latestVersionURL ("https://api.github.com/repos/sonosaurus/sonobus/releases/" + endpoint);`
  reached from `VersionInfo::fetchLatestFromUpdateServer()` / `fetchFromUpdateServer(const String&)` (`/Users/seungchan/github/meon/Source/VersionInfo.h:19-21`), called from `LatestVersionCheckerAndUpdater::run()` at `AU.cpp:60`.
- **Trigger sites**:
  - `Ed.cpp:1963` — inside `timerCallback(CheckForNewVersionTimerId)`, gated on `getShouldCheckForNewVersionValue()` being true. Timer armed at `Ed.cpp:1367` (`startTimer(CheckForNewVersionTimerId, 5000)`), only when `JUCE_WINDOWS || JUCE_MAC` **and** `JUCEApplicationBase::isStandaloneApp()`.
  - `Ed.cpp:5885` — manual menu command (`SonobusCommands::CheckForNewVersion`, `Types.h:24`), `checkForNewVersion(true)`.
  - `Ed.cpp:1347` — already commented out.
- The `shouldCheckForNewVersion` toggle is persisted by the standalone holder: `SFW.h:355` (write) / `SFW.h:396` (read), exposed to the editor as `std::function<Value*()> getShouldCheckForNewVersionValue;` (`Ed.h:158`).

### All `sonobus.net` occurrences in `Source/`
| File:line | Text |
|---|---|
| `SPP.h:35` | `#define DEFAULT_SERVER_HOST "aoo.sonobus.net"` |
| `AU.cpp:69` | alert string "…download the latest version of SonoBus from sonobus.net" |
| `/Users/seungchan/github/meon/Source/ConnectView.cpp:891` | `URL url2("http://go.sonobus.net/sblaunch");` (group-link generation) |
| `/Users/seungchan/github/meon/Source/ConnectView.cpp:1343-1344` | clipboard parse of `http(s)://go.sonobus.net/sblaunch?` |
| `/Users/seungchan/github/meon/Source/ConnectView.cpp:1367` | comment about that URL form |

---

## 9. Feature APIs to hide (names + file:line only)

**Recording** — `startRecordingToFile` `SPP.h:743` / `SPP.cpp:9092`; `stopRecordingToFile` `SPP.h:744` / `SPP.cpp:9510`; `isRecordingToFile` `SPP.h:745` / `SPP.cpp:9577`; `getElapsedRecordTime` `SPP.h:746`; `getLastErrorMessage` `SPP.h:747`; `setDefaultRecordingDirectory`/`getDefaultRecordingDirectory` `SPP.h:749-750`; `setLastBrowseDirectory`/`getLastBrowseDirectory` `SPP.h:752-753`; `getDefaultRecordingOptions`/`setDefaultRecordingOptions` `SPP.h:755-756`; `getDefaultRecordingFormat`/`setDefaultRecordingFormat` `SPP.h:758-759`; `getDefaultRecordingBitsPerSample`/`set…` `SPP.h:761-762`; `getSelfRecordingPreFX`/`set…` `SPP.h:764-765`; `getSelfRecordingSilenceWhenMuted`/`set…` `SPP.h:767-768`; `getRecordFinishOpens`/`set…` `SPP.h:770-771`; `isAnyRemotePeerRecording` `SPP.h:472`; `isRemotePeerRecording` `SPP.h:473`. Enums `RecordFileOptions` `SPP.h:139-145`, `RecordFileFormat` `SPP.h:147-153`.

**File playback / transport** — `loadURLIntoTransport` `SPP.h:793` / `SPP.cpp:9595`; `clearTransportURL` `SPP.h:794` / `SPP.cpp:9586`; `getCurrentLoadedTransportURL` `SPP.h:795`; `getTransportSource` `SPP.h:796`; `getFormatManager` `SPP.h:797`; `setFilePlaybackMonitorDelayParams`/`get…` `SPP.h:725-726`; `setFilePlaybackDestStartAndCount`/`get…` `SPP.h:727-728`; `setFilePlaybackGain`/`get…` `SPP.h:729-730` (`SPP.cpp:1745`); `setFilePlaybackMonitor`/`get…` `SPP.h:731-732`; `getSendingFilePlaybackAudio` `SPP.h:572`; `getFilePlaybackMeterSource` `SPP.h:648`.

**Soundboard** — `setLastSoundboardWidth`/`get…` `SPP.h:812-813`; `setLastSoundboardShown`/`get…` `SPP.h:814-815`; `SoundboardChannelProcessor* getSoundboardProcessor()` `SPP.h:816`. Classes in `/Users/seungchan/github/meon/Source/SoundboardChannelProcessor.h:159`, `/Users/seungchan/github/meon/Source/SoundboardProcessor.h`, `/Users/seungchan/github/meon/Source/SoundboardView.h`.

**Metronome** — `setMetronomeMonitorDelayParams`/`get…` `SPP.h:713-714` (`SPP.cpp:1640`); `setMetronomeChannelDestStartAndCount`/`get…` `SPP.h:715-716`; `setMetronomePan`/`get…` `SPP.h:717-718` (`SPP.cpp:1676`); `setMetronomeGain`/`get…` `SPP.h:719-720` (`SPP.cpp:1687`, `1692`); `setMetronomeMonitor`/`get…` `SPP.h:721-722` (`SPP.cpp:1697`); `getMetronomeMeterSource` `SPP.h:649`; `getSyncMetToHost` `SPP.h:576`. Class `/Users/seungchan/github/meon/Source/Metronome.h`.

**VDO.Ninja** — `struct VideoLinkInfo` `SPP.h:586-605` (fields `roomMode, showNames, beDirector, screenShareMode, largeShare, pushViewMode, extraParams`, enum `PushAndView/PushOnly/ViewOnly`); `VideoLinkInfo & getVideoLinkInfo()` `SPP.h:607`. View: `/Users/seungchan/github/meon/Source/VDONinjaView.h:20`. Command id `SonobusCommands::VDONinjaVideoLink` `Types.h:39`.

**Latency match** — `beginLatencyMatchProcedure` `SPP.h:781` / `SPP.cpp:3030`; `isLatencyMatchProcedureReady` `SPP.h:782` / `SPP.cpp:3043`; `sendLatencyMatchToAll` `SPP.h:783` / `SPP.cpp:2999`; `getLatencyInfoList` `SPP.h:784` / `SPP.cpp:3053`; `commitLatencyMatch` `SPP.h:785` / `SPP.cpp:3059`; `struct LatInfo` `SPP.h:177-181`. Callback `peerRequestedLatencyMatch` `SPP.h:679`. View `/Users/seungchan/github/meon/Source/LatencyMatchView.h:13`. Command `SonobusCommands::GroupLatencyMatch` `Types.h:38`.

**Suggest new group** — `void suggestNewGroupToPeers(const String & group, const String & groupPass, const StringArray & peernames, bool ispublic=false);` `SPP.h:788` / `SPP.cpp:3089`; callback `peerSuggestedNewGroup` `SPP.h:681`. View `/Users/seungchan/github/meon/Source/SuggestNewGroupView.h:16`. Command `SonobusCommands::SuggestNewGroup` `Types.h:40`.

**Command id list** for menu/keyboard hiding: `class SonobusCommands` enum, `Types.h:11-44`.

---

## 10. AudioDeviceManager / sample rate / buffer size

- **The processor does not own or expose an `AudioDeviceManager`.** There is no `SonobusAudioProcessor::getAudioDeviceManager()`.
- It is owned by the standalone holder: `AudioDeviceManager deviceManager;` — `SFW.h:547`, accessed through `AudioDeviceManager& getDeviceManager() const noexcept { return pluginHolder->deviceManager; }` — `SFW.h:939`.
- The editor receives it as an injected lambda: `std::function<AudioDeviceManager*()> getAudioDeviceManager;` — `Ed.h:153`; wired at `SFW.h:1062` (editor), `SFW.h:1063` (input channel groups view), `SFW.h:1064` (peers container view). Usage examples: `Ed.cpp:1927`, `Ed.cpp:2641`, `Ed.cpp:2676`, `Ed.cpp:2760-2770`, `Ed.cpp:3455`.
- **Persisted device settings key is `"audioSetup"`, not `lastAudioDeviceSettings`.** Write: `settings->setValue ("audioSetup", xml.get());` — `SFW.h:352`; read: `savedState = settings->getXmlValue ("audioSetup");` — `SFW.h:393`. Sibling keys in the same `PropertiesFile`: `"shouldOverrideSampleRate"` (354/395), `"shouldCheckForNewVersion"` (355/396), `"allowBluetoothInput"` (356/397), `"recentSetupFiles"` (364/399), `"lastRecentsSetupFolder"` (365/412), `"shouldMuteInput"` (415), `"filterStateXML"` (472/495), `"filterState"` (479/504), `"lastStateFile"` (207/218).
- Sample rate / block size: no `getCurrentSampleRate()`. Use JUCE `AudioProcessor::getSampleRate()` / `getBlockSize()`, or the processor's own cached block size:
  `int32 getCurrSamplesPerBlock() const { return currSamplesPerBlock; }` — `SPP.h:183` (member `currSamplesPerBlock = 256`, `SPP.h:1042`; `maxBlockSize = 4096`, `SPP.h:1037`).
- Support dir for settings: `PropertiesFile::Options` in the processor ctor, `SPP.cpp:747-760` (`applicationName "SonoBus"`, `osxLibrarySubFolder "Application Support/SonoBus"`, Linux `~/.config/sonobus`); result stored as `mSupportDir` (`SPP.cpp:757`).

---

## 11. Events / listeners / timers

### Full `ClientListener` interface — `SPP.h:661-682`
```cpp
class ClientListener {
public:
    virtual ~ClientListener() {}
    virtual void aooClientConnected(SonobusAudioProcessor *comp, bool success, const String & errmesg="") {}                                   // :664  fired SPP.cpp:4200
    virtual void aooClientDisconnected(SonobusAudioProcessor *comp, bool success, const String & errmesg="") {}                                // :665  fired SPP.cpp:4225
    virtual void aooClientLoginResult(SonobusAudioProcessor *comp, bool success, const String & errmesg="") {}                                 // :666  NEVER FIRED in SPP.cpp
    virtual void aooClientGroupJoined(SonobusAudioProcessor *comp, bool success, const String & group, const String & errmesg="") {}           // :667  fired SPP.cpp:4243
    virtual void aooClientGroupLeft(SonobusAudioProcessor *comp, bool success, const String & group, const String & errmesg="") {}             // :668  fired SPP.cpp:4266
    virtual void aooClientPublicGroupModified(SonobusAudioProcessor *comp, const String & group, int count, const String & errmesg="") {}      // :669  fired SPP.cpp:4283
    virtual void aooClientPublicGroupDeleted(SonobusAudioProcessor *comp, const String & group, const String & errmesg="") {}                  // :670  fired SPP.cpp:4296
    virtual void aooClientPeerPendingJoin(SonobusAudioProcessor *comp, const String & group, const String & user) {}                           // :671  fired SPP.cpp:4307
    virtual void aooClientPeerJoined(SonobusAudioProcessor *comp, const String & group, const String & user) {}                                // :672  fired SPP.cpp:4343
    virtual void aooClientPeerJoinFailed(SonobusAudioProcessor *comp, const String & group, const String & user) {}                            // :673  fired SPP.cpp:4361
    virtual void aooClientPeerJoinBlocked(SonobusAudioProcessor *comp, const String & group, const String & user, const String & address, int port) {} // :674 fired SPP.cpp:4328
    virtual void aooClientPeerLeft(SonobusAudioProcessor *comp, const String & group, const String & user) {}                                  // :675  fired SPP.cpp:4384
    virtual void aooClientError(SonobusAudioProcessor *comp, const String & errmesg) {}                                                        // :676  fired SPP.cpp:4395
    virtual void aooClientPeerChangedState(SonobusAudioProcessor *comp, const String & mesg) {}                                                // :677  fired SPP.cpp:2688, 3857 (arg "format")
    virtual void sbChatEventReceived(SonobusAudioProcessor *comp, const SBChatEvent & chatevent) {}                                            // :678  fired SPP.cpp:2709
    virtual void peerRequestedLatencyMatch(SonobusAudioProcessor *comp, const String & username, float latency) {}                             // :679  fired SPP.cpp:2780
    virtual void peerBlockedInfoChanged(SonobusAudioProcessor *comp, const String & username, bool blocked) {}                                 // :680  fired SPP.cpp:2852
    virtual void peerSuggestedNewGroup(SonobusAudioProcessor *comp, const String & username, const String & newgroup, const String & grouppass, bool isPublic, const StringArray & others) {} // :681 fired SPP.cpp:2812
};

void addClientListener(ClientListener * l)    { clientListeners.add(l); }     // SPP.h:684
void removeClientListener(ClientListener * l) { clientListeners.remove(l); }  // SPP.h:687
```
Storage: `ListenerList<ClientListener> clientListeners;` — `SPP.h:945`. Callbacks are dispatched from `handleClientEvents` / `handleOtherMessage`, i.e. **on the AOO event thread, not the message thread** — a UI must marshal.

### Polling model used by the existing UI
The editor implements `ClientListener` (`Ed.h:47`, overrides `Ed.h:133-150`) and additionally polls:
- `void timerCallback(int timerid) override;` — `Ed.h:70`, impl `Ed.cpp:1862`
- timer ids `enum { PeriodicUpdateTimerId = 0, CheckForNewVersionTimerId }` — `Ed.cpp:34-35`
- `startTimer(PeriodicUpdateTimerId, 1000);` — `Ed.cpp:1363`; `processor.addClientListener(this);` — `Ed.cpp:1353`
All meters, peer stats, latency and buffer readouts are pulled every 1 s from the getters above.

### Routing / expansion / blocked state
```cpp
bool isAnythingRoutedToPeer(int index) const;       // SPP.h:651, SPP.cpp:6295
bool getRemotePeerViewExpanded(int index) const;    // SPP.h:403, SPP.cpp:5088
void setRemotePeerViewExpanded(int index, bool expanded);  // SPP.h:404, SPP.cpp:5099
bool getRemotePeerBlockedUs(int index) const;       // SPP.h:451, SPP.cpp:5576  (they blocked us)
bool isAddressBlocked(const String & ipaddr) const; // SPP.h:656, SPP.cpp:9016  (we blocked them)
void addBlockedAddress(const String & ipaddr);      // SPP.h:657, SPP.cpp:9032
void removeBlockedAddress(const String & ipaddr);   // SPP.h:658, SPP.cpp:9052
StringArray getAllBlockedAddresses() const;         // SPP.h:659, SPP.cpp:9072
void sendBlockedInfoMessage(EndpointState *endpoint, bool blocked);  // SPP.h:790, SPP.cpp:2952
bool getRemotePeerSafetyMuted(int index) const;     // SPP.h:450, SPP.cpp:5566
```
Block list lives in `GlobalState.xml` under `BlockedAddresses`/`Address`/`value` (`SPP.cpp:123-125`), not in the plugin state.

---

## 12. Network: NAT/P2P failure, relay, retry

- **NAT/P2P hole-punch failure** surfaces as `aooClientPeerJoinFailed(processor, group, user)` (`SPP.h:673`), raised from `AOONET_CLIENT_PEER_JOINFAIL_EVENT` at `SPP.cpp:4354-4368`. The event itself is pushed by the AOO library when the UDP handshake to both the local and public address times out: `/Users/seungchan/github/meon/deps/aoo/lib/src/client.cpp:1343-1359` ("couldn't establish UDP connection to … timed out after N seconds"). Defaults: `#define AOO_NET_CLIENT_REQUEST_TIMEOUT 5000` and `#define AOO_NET_CLIENT_PING_INTERVAL 10000` — `/Users/seungchan/github/meon/deps/aoo/lib/src/client.hpp:19,21`.
- A peer blocked by our IP block list raises `aooClientPeerJoinBlocked(comp, group, user, address, port)` instead (`SPP.h:674`, fired `SPP.cpp:4328`).
- **There is no relay support.** No relay API in `SPP.h`/`SPP.cpp`; the only occurrence in the vendored library is a roadmap bullet at `/Users/seungchan/github/meon/deps/aoo/readme.rst:109` ("relay for AoO server"). Failed peers simply stay unconnected.
- **`getServerConnectRetryCount` does not exist.** The retry mechanism is:
  - `class ServerReconnectTimer : public Timer` — `SPP.h:1130-1139`; member `ServerReconnectTimer mReconnectTimer;` — `SPP.h:1141`; `void ServerReconnectTimer::timerCallback() override;` impl `SPP.cpp:8835` (retries `reconnectToMostRecent()` every 1000 ms until connected; no counter, no cap).
  - Started at `SPP.cpp:4210-4214` on an unexpected disconnect when `mCurrentJoinedGroup.isNotEmpty() && mReconnectAfterServerLoss.get()`; stopped at `SPP.cpp:4194-4198` on reconnect.
  - Gate accessors: `bool getReconnectAfterServerLoss() const` / `void setReconnectAfterServerLoss(bool flag)` — `SPP.h:773-774`; related flags `mPendingReconnect` (`SPP.h:1127`), `mRecoveringFromServerLoss` (`SPP.h:1128`), `mPendingReconnectInfo` (`SPP.h:1126`).
  - `bool reconnectToMostRecent();` — `SPP.h:942`, impl `SPP.cpp:8847` (private).
- Local UDP port: `int getUdpLocalPort() const` (`SPP.h:326`), set during `initializeAoo(int udpPort=0)` (`SPP.cpp:905`); user override via `setUseSpecificUdpPort(int port)` (`SPP.h:294`, `SPP.cpp:893`), persisted as `"UseUdpPort"`.
- `EndpointState` struct (`SPP.cpp:196-232`): `String ipaddr; int port; int64_t sentBytes; int64_t recvBytes;` plus `getRawAddr()`. Lookup helpers `EndpointState * findOrAddEndpoint(const String & host, int port);` (`SPP.h:323`) and `findOrAddRawEndpoint(void * rawaddr)` (`SPP.h:324`).

---

## Appendix: supporting structs

- `SonoAudio::ChannelGroupParams` — `CG.h:23-88`. Key fields for a UI: `name, chanStartIndex, numChannels, muted, soloed, gain, pan[MAX_CHANNELS], panStereo[2], centerPanLaw, panDestStartIndex, panDestChannels, sendMainMix, compressorParams, expanderParams, eqParams, limiterParams, invertPolarity, inReverbSend, monReverbSend, monitor, monDestStartIndex, monDestChannels, monitorDelayParams`. Serialisation: `ValueTree getValueTree() const` (`CG.h:29`), `void setFromValueTree(const ValueTree&)` (`CG.h:30`), `getChannelLayoutValueTree()` (`CG.h:34`). `#define MAX_CHANNELS 64` — `CG.h:18`.
- `SonoAudio::CompressorParams` — `EP.h:11-21`: `enabled, thresholdDb, ratio, attackMs, releaseMs, makeupGainDb, automakeupGain`. (Also used for the expander and limiter.)
- `SonoAudio::ParametricEqParams` — `EP.h:26-41`: `enabled, lowShelfGain/Freq, para1Gain/Freq/Q, para2Gain/Freq/Q, highShelfGain/Freq`.
- `SonoAudio::DelayParams` — `EP.h:43-50`: `enabled, delayTimeMs`.
- `RemotePeer` (private, `SPP.cpp:246-368`) — the backing store for every per-peer getter. Notable fields for a UI: `gain, buffertimeMs, autosizeBufferMode, sendActive/recvActive/sendAllow/recvAllow/soloed, formatIndex, recvFormat, packetsize, sendChannels/nominalSendChannels/recvChannels, connected, userName, groupName, dataPacketsReceived/Sent/Dropped/Resent, pingTime, smoothPingTime, fillRatio, fillRatioSlow, totalEstLatency, totalLatency, hasRealLatency, latencyDirty, sendMeterSource, recvMeterSource, viewExpanded, orderPriority, chanGroups[MAX_CHANGROUPS], numChanGroups, remoteJitterBufMs, remoteInLatMs, remoteOutLatMs, remoteNetType, remoteIsRecording, hasRemoteInfo, blockedUs`.
- Peer message OSC namespace (`SPP.cpp:2418-2459`): `/sb` + `/pinfo`, `/clayinfo`, `/chat`, `/ping`, `/pngack`, `/reqlatinfo`, `/latinfo`, `/suggestlat`, `/blockedinfo`, `/suggestgroup`.

---

## Names asked about that do not exist

| Requested name | Status | Closest real API |
|---|---|---|
| `aooClientGroupJoinFailed` | absent | `aooClientGroupJoined(..., bool success, ..., const String & errmesg)` — `SPP.h:667` |
| `aooClientChatEvent` | absent | `sbChatEventReceived(...)` — `SPP.h:678` |
| `sendChatMessage` | absent | `sendChatEvent(const SBChatEvent&)` — `SPP.h:800` |
| `isRemotePeerConnected` | absent | `getRemotePeerConnected(int)` — `SPP.h:634` |
| `getRemotePeerPingMs` | absent | `getRemotePeerLatencyInfo(...).pingMs` — `SPP.h:465` |
| `getRemotePeerTotalLatency` | absent | `getRemotePeerLatencyInfo(...).totalRoundtripMs` — `SPP.h:465` |
| `getRemotePeerRecvAudioStats` | absent | `getRemotePeerPacketsReceived/Dropped/Resent` — `SPP.h:440-447` |
| `getRemotePeerRecvPacketDropCount` | absent | `getRemotePeerPacketsDropped(int)` — `SPP.h:446` |
| `setMainSendMute` | absent | APVTS parameter `paramMainSendMute` — `SPP.h:249` / `SPP.cpp:54` |
| `setAutoconnectToGroup` | absent | `setAutoconnectToGroupPeers(bool)` — `SPP.h:302` |
| remote-peer "compact" name | absent | `PeerDisplayMode` + `getPeerDisplayMode/setPeerDisplayMode` — `SPP.h:155-158`, `777-778` |
| `getServerConnectRetryCount` | absent | `ServerReconnectTimer` (`SPP.h:1130`) + `getReconnectAfterServerLoss()` (`SPP.h:773`) |
| `getCurrentSampleRate` | absent | `AudioProcessor::getSampleRate()`, `getCurrSamplesPerBlock()` — `SPP.h:183` |
| `getAudioDeviceManager` on the processor | absent | `SFW.h:939` holder / `Ed.h:153` injected lambda |
| `'defaultAutoNetbuf'` / `'defaultSendQual'` state keys | absent | parameters `"defnetauto"` / `"defsendqual"` — `SPP.cpp:52-53` |
| `'lastAudioDeviceSettings'` | absent | `PropertiesFile` key `"audioSetup"` — `SFW.h:352`, `SFW.h:393` |
| relay / NAT fallback API | absent | none; failure reported by `aooClientPeerJoinFailed` — `SPP.h:673` |
| `aooClientLoginResult` | declared but never invoked | `SPP.h:666`; no `clientListeners.call(...)` site in `SPP.cpp` |
| `paramBufferTime` | declared/defined, never registered as a parameter | `SPP.h:245` / `SPP.cpp:50`; use `paramDefaultNetbufMs` |