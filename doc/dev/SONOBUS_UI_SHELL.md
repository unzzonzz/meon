<!-- 자동 생성: 원본 SonoBus 1.7.2 코드 조사 결과 (MEON 개발 참고용) -->

I have enough to compile the reference.

---

# SonoBus 1.7.2 — App Shell & UI Plumbing Reference

Scope: what a replacement for `SonobusAudioProcessorEditor` must reuse, reimplement, or delete. Repo root `/Users/seungchan/github/meon`. All line numbers are as-of the current working tree.

---

## 1. Standalone app shell

### 1.1 Files

| File | Role |
|---|---|
| `/Users/seungchan/github/meon/Source/SonoStandaloneFilterApp.cpp` (920 lines) | `juce::SonobusStandaloneFilterApp : JUCEApplication, Timer`; `START_JUCE_APPLICATION` at :918 |
| `/Users/seungchan/github/meon/Source/SonoStandaloneFilterWindow.h` (1272 lines) | Forked copy of JUCE's `StandalonePluginHolder` (:56) + `StandaloneFilterWindow` (:823) + inner `MainContentComponent` (:1046) |

`JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1` is set in `CMakeLists.txt:450`. JUCE's own `deps/juce/modules/juce_audio_plugin_client/juce_audio_plugin_client_Standalone.cpp:49` then skips its `StandaloneFilterApp`/`juce_StandaloneFilterWindow.h`, and at :184 expects `juce_CreateApplication()` — supplied by `START_JUCE_APPLICATION(SonobusStandaloneFilterApp)`. So the fork is fully in-tree; a new UI keeps this arrangement unchanged.

### 1.2 PropertiesFile / settings location

`SonoStandaloneFilterApp.cpp:82-93` (app ctor):

```cpp
options.applicationName     = getApplicationName();   // == JucePlugin_Name == "SonoBus"
options.filenameSuffix      = ".settings";
options.osxLibrarySubFolder = "Application Support/SonoBus";
#if JUCE_LINUX
 options.folderName         = "~/.config/sonobus";
#else
 options.folderName         = "";
#endif
appProperties.setStorageParameters (options);
```

macOS result: `~/Library/Application Support/SonoBus/SonoBus.settings`.
Linux 1.3.19 one-time migration from `~/.config/SonoBus.settings` at :95-105.

A **second, parallel** options block lives in the processor, `SonobusPluginProcessor.cpp:740-751`, with `filenameSuffix = ".xml"` and the same folder; only `options.getDefaultFile().getParentDirectory()` is kept as `mSupportDir`. `mSupportDir` is what the editor uses for user-supplied localisation files and `PluginDefault.xml` / `GlobalState.xml` (`SonobusPluginProcessor.cpp:8810,8992`).

### 1.3 Window creation and default size

`createWindow()` — `SonoStandaloneFilterApp.cpp:182-236`:
- preferred `AudioDeviceSetup`: `sampleRate = 48000`; `bufferSize` 128 (mac) / 192 (Android) / 256 (other) — :188-197
- `LookAndFeel::setDefaultLookAndFeel(&sonoLNF)` at :216 (a `SonoLookAndFeel` app member, :119)
- `new StandaloneFilterWindow(getApplicationName(), findColour(ResizableWindow::backgroundColourId), appProperties.getUserSettings(), /*takeOwnership*/false, {}, &setupOptions, {}, /*autoOpenMidiDevices*/false)` — :218-230

`StandaloneFilterWindow` ctor — `SonoStandaloneFilterWindow.h:837-910`:
- `DocumentWindow(title, bg, DocumentWindow::allButtons)` :850
- desktop: `setTitleBarButtonsRequired(minimise|close|maximise, false)`, `setUsingNativeTitleBar(true)` :858-863; mobile: `setTitleBarHeight(0)` :856
- **`setResizable(true, false)` at :866** — resizable, no corner-resizer
- **No `setResizeLimits` / constrainer on the window.** The only size limits come from the editor (`setResizeLimits(320, 340, 10000, 10000)`, see §2.5), propagated up by `MainContentComponent::componentMovedOrResized` :1228-1233.
- Window geometry restored from the settings file keys `windowX/windowY/windowW/windowH` (default sentinel `-100`) at :878-904; saved in the dtor at :915-921.

There is **no hard-coded default window pixel size**. Initial size comes from the editor, which uses `processor.getLastPluginBounds()` → `mPluginWindowWidth=800`, `mPluginWindowHeight=600` (`SonobusPluginProcessor.h:1063-1064`).

### 1.4 AudioDeviceManager

Owned by the holder: `AudioDeviceManager deviceManager;` — `SonoStandaloneFilterWindow.h:547`.

- ctor `:79-124` → `init()` `:126-138`: Windows forces `deviceManager.setCurrentAudioDeviceType("ASIO", false)` :130, then `setupAudioDevices()` → `reloadAudioDeviceState()`, `reloadPluginState()`, `startPlaying()`.
- `reloadAudioDeviceState` `:378-455`: reads `audioSetup` XML, `shouldOverrideSampleRate`, `shouldCheckForNewVersion`, `allowBluetoothInput`, `recentSetupFiles`, `lastRecentsSetupFolder`, `shouldMuteInput`. If `shouldOverrideSampleRate` is false it *strips* `audioDeviceRate` from the saved XML (:420-428). Final call is `deviceManager.initialise(inCh, outCh, savedState, true, preferredDefaultDeviceName, prefSetupOptions)` :442.
- `saveAudioDeviceState` `:346-376`: writes `audioSetup` (from `deviceManager.createStateXml()`), plus the three `Value`s and the recents list.
- Plugin state: `savePluginState` `:458-486` writes `filterStateXML` (preferred; `filterState` base64 is the legacy fallback); `reloadPluginState` `:488-507`.
- Defaults: `shouldOverrideSampleRate=true`, `allowBluetoothInput=false`, `shouldCheckForNewVersion=true` (`:99-101`).

### 1.5 How the editor reaches the device manager

Via `std::function` hooks assigned in `MainContentComponent`'s ctor, `SonoStandaloneFilterWindow.h:1061-1073`:

```cpp
sonoeditor->getAudioDeviceManager = [this]() { return &owner.getDeviceManager(); };
sonoeditor->getInputChannelGroupsView()->getAudioDeviceManager = ...;
sonoeditor->getPeersContainerView()->getAudioDeviceManager = ...;
sonoeditor->isInterAppAudioConnected / getIAAHostIcon / switchToHostApplication
sonoeditor->getShouldOverrideSampleRateValue / getAllowBluetoothInputValue / getShouldCheckForNewVersionValue
sonoeditor->getRecentSetupFiles / getLastRecentsFolder
```

Declared on the editor at `SonobusPluginEditor.h:153-164`. `saveSettingsIfNeeded` is assigned separately, in the app, at `SonoStandaloneFilterApp.cpp:461-465` (calls `savePluginState()`, `saveAudioDeviceState()`, `appProperties.saveIfNeeded()`).

**Any replacement editor must expose these same `std::function` members (or the window/app must be edited in lockstep).** They are the only channel between shell and UI.

### 1.6 Microphone permission

`SonoStandaloneFilterWindow.h:116-123`:

```cpp
if (audioInputRequired && RuntimePermissions::isRequired (RuntimePermissions::recordAudio)
    && ! RuntimePermissions::isGranted (RuntimePermissions::recordAudio))
    RuntimePermissions::request (RuntimePermissions::recordAudio,
                                 [this, preferredDefaultDeviceName] (bool granted) { init (granted, …); });
else
    init (audioInputRequired, preferredDefaultDeviceName);
```

On macOS the usage string comes from `MICROPHONE_PERMISSION_ENABLED TRUE` (`CMakeLists.txt:165`) — no custom `MICROPHONE_PERMISSION_TEXT`, so JUCE's default text is used. Hardened runtime entitlement `com.apple.security.device.audio-input` at `CMakeLists.txt:178`.

### 1.7 `sonobus://` URL scheme

- Registered in the merged plist, `CMakeLists.txt:123-139`: `CFBundleURLName = net.sonobus`, `CFBundleURLSchemes = [sonobus]`.
- macOS/iOS delivery: `SonobusStandaloneFilterApp::urlOpened(const URL&)` — `SonoStandaloneFilterApp.cpp:698-712` → `sonoeditor->handleURL(url.toString(true)); mainWindow->toFront(true);`
- Windows/second-instance delivery: `anotherInstanceStarted(const String& url)` — `:714-729`, identical body. Note `moreThanOneInstanceAllowed()` returns **`true`** (`:117`), so on most platforms a second instance actually launches; `anotherInstanceStarted` still fires where the OS routes it.
- Linux: the trailing command-line argument is treated as the URL (`:428-430`, applied at `:477-483` under `#if JUCE_LINUX`).
- Editor side: `SonobusAudioProcessorEditor::handleURL` — `SonobusPluginEditor.cpp:2819-2836`. Only acts when not already connected/in a group; delegates to `mConnectView->handleSonobusURL(url)` then `connectWithInfo(currConnectionInfo)`.
- Parsing lives in `ConnectView::handleSonobusURL` — `ConnectView.cpp:1365-1417`. Accepts both `sonobus://host:port/?g=…&p=…&public=…` and `http://go.sonobus.net/sblaunch?g=…&p=…&s=host:port`. Query params: `g` group, `p` group password, `public` 0/1, `s` server host:port (http form only). Default port `DEFAULT_SERVER_PORT` = 10998; default host `aoo.sonobus.net` (`SonobusPluginProcessor.h:34-35`).
- Link generation: `ConnectView::copyInfoToClipboard` — `ConnectView.cpp:866-889`, builds `sonobus://<host:port>/…`.
- `#define SONOBUS_SCHEME "sonobus"` at `SonobusPluginEditor.cpp:49` is defined but unused elsewhere in that file.

### 1.8 macOS menu bar

Built **inside the editor**, not the app — `SonobusPluginEditor.cpp:1316-1333`:

```cpp
menuBarModel = std::make_unique<SonobusMenuBarModel>(*this);
menuBarModel->setApplicationCommandManagerToWatch(&commandManager);
#if JUCE_MAC
auto extraAppleMenuItems = PopupMenu();
extraAppleMenuItems.addCommandItem(&commandManager, SonobusCommands::ShowOptions);
MenuBarModel::setMacMainMenu(menuBarModel.get(), &extraAppleMenuItems);
#endif
#if (JUCE_WINDOWS || JUCE_LINUX)
mMenuBar = std::make_unique<MenuBarComponent>(menuBarModel.get());
addAndMakeVisible(mMenuBar.get());
#endif
```

Torn down in `~SonobusAudioProcessorEditor` (`:1388-1393`). `SonobusMenuBarModel` is declared at `SonobusPluginEditor.h:580-597`; implemented at `SonobusPluginEditor.cpp:5946-6042`. Top-level names: File / Connect / Group / Transport / View (`:5948-5953`; index enum at `:5399-5404`, `MenuHelpIndex` exists but its menu is empty).

On Windows/Linux the menu bar is laid out inside `resized()` (`:4476-4479`) using `getLookAndFeel().getDefaultMenuBarHeight()`.

### 1.9 First run

There is **no explicit "first run" flag**. The nearest behaviours:

1. **Crash sentinel** — `SonoStandaloneFilterApp.cpp:199-214` (and the duplicate in `createHeadlessPlugin`, `:147-162`). A file `SENTINEL` next to the settings file is created before constructing the window and deleted after (`:233`). If it still exists at launch, the settings file is renamed `POSSIBLY_BAD_<timestamp>` and an alert `TRANS("Crashed Last Time")` is shown after 800 ms.
2. **Clipboard auto-fill** — `SonobusPluginEditor.cpp:1336-1343`: on standalone launch, if the clipboard holds a `sonobus://` link, the connect popup opens on the private-group tab with a status message.
3. **"Setup Audio" prompt** — `mSetupAudioButton` (`:755-757`) is shown only when standalone, desktop, and `processor.getNumberRemotePeers() == 0` (`:3714-3720`); main message is `"Press Connect button to start." + "Please use headphones if you are using a microphone!"` (`:3698-3701`).
4. **First tooltip-window / focus grab** — `timerCallback` `:1891-1906` creates the `CustomTooltipWindow` once a parent exists and does a one-shot `mConnectButton->grabKeyboardFocus()`.
5. `ConnectView::firstTimeConnectShow` (`ConnectView.h:106`) and the `firsttime` local in `showSettings` (`SonobusPluginEditor.cpp:3453`) are per-session, not persisted.

### 1.10 Quit / confirm-on-close

- `StandaloneFilterWindow::closeButtonPressed()` — `SonoStandaloneFilterWindow.h:988-993`: `savePluginState()` then `JUCEApplicationBase::getInstance()->systemRequestedQuit()`.
- `SonobusStandaloneFilterApp::systemRequestedQuit()` — `SonoStandaloneFilterApp.cpp:806-840`: saves plugin + device state + properties, then asks the editor:

```cpp
if (auto * sonoeditor = dynamic_cast<SonobusAudioProcessorEditor*>(editor)) {
    if (!sonoeditor->requestedQuit()) return;   // editor will handle it
}
if (ModalComponentManager::getInstance()->cancelAllModalComponents())
    Timer::callAfterDelay(100, [](){ … systemRequestedQuit(); });
else
    quit();
```

- `SonobusAudioProcessorEditor::requestedQuit()` — `SonobusPluginEditor.cpp:1434-1450`: if `currConnected && currGroup.isNotEmpty()`, shows `AlertWindow::showOkCancelBox(TRANS("Quit Confirmation"), TRANS("You are connected, are you sure you want to quit?"), TRANS("Quit"), …)` and returns `false`; the callback `doActuallyQuit` (`:1427-1432`) calls `JUCEApplicationBase::quit()`.
- `shutdown()` `:675-696`, `suspended()` `:731-765` (mobile: closes the audio device when no peers), `resumed()` `:779-803` (restarts last audio device).
- `Desktop::getInstance().setScreenSaverEnabled(false)` on launch (`:455`), re-enabled on suspend (`:764`).
- `#if JUCE_MAC disableAppNap();` at `:568-570`.

### 1.11 Command line (standalone only)

`handleCommandLine()` — `:293-432`. Options: `-c/--connectionserver`, `-g/--group`, `-p/--group-password`, `-n/--username`, `-l/--load-setup`, `-q/--headless`, `-v/--version`, `-h/--help`; any remaining bare argument is treated as a connect URL. Headless path (`:515-564`) builds a `StandalonePluginHolder` with no window and calls `connectToServer` / `joinServerGroup` directly — it does **not** touch the editor, so it survives a UI rewrite untouched.

---

## 2. `SonobusAudioProcessorEditor`

`/Users/seungchan/github/meon/Source/SonobusPluginEditor.h` (732 lines) / `.cpp` (6043 lines).

### 2.1 Base classes (`SonobusPluginEditor.h:42-58`)

`AudioProcessorEditor`, `MultiTimer`, `Button::Listener`, `AudioProcessorValueTreeState::Listener`, `Slider::Listener`, `SonoChoiceButton::Listener`, **`SonobusAudioProcessor::ClientListener`**, `ComponentListener`, `ChangeListener`, `TextEditor::Listener`, **`ApplicationCommandTarget`**, `AsyncUpdater`, `FileDragAndDropTarget`, `GenericItemChooser::Listener`, `ConnectView::Listener`, `ChannelGroupsView::Listener`, `PeersContainerView::Listener`.

### 2.2 Constructor sequence (`.cpp:275-1384`)

Ordered highlights:

| Line | Step |
|---|---|
| 276 | Init list: `sonoLookAndFeel(p.getUseUniversalFont())`, `sonoSliderLNF(13)`, `smallLNF(14)`, `teensyLNF(11)`, `panSliderLNF(12)` |
| 278-289 | `SonoLookAndFeel::setFontScale(...)` — 1.0 Android / 1.35 Windows / 1.25 else when universal font on; 1.0 otherwise |
| 291-293 | `LookAndFeel::setDefaultLookAndFeel(&sonoLookAndFeel)`, `setUsingNativeAlertWindows(true)` |
| 295 | `mSettingsFolder = processor.getSupportDir()` |
| 297 | `setupLocalisation(processor.getLanguageOverrideCode())` |
| 299-303 | `setLanguageCode(mActiveLanguageCode, processor.getUseUniversalFont())` on all five L&Fs |
| 305-307 | custom colour ids `nameTextColourId/selectedColourId/separatorColourId` (enum at `.cpp:39-43`) |
| 315-347 | seed `currConnectionInfo` from `processor.getRecentServerConnectionInfos()`, else username from `SystemStats::getFullUserName()` (iOS: `getComputerName()`), then override with `processor.getCurrentUsername()` |
| 349-1180 | build ~150 child components (labels, buttons, sliders, meters, `ConnectView`, `ChatView`, `SoundboardView`, `PeersContainerView`, `ChannelGroupsView`, …) |
| 449-687 | `AudioProcessorValueTreeState::SliderAttachment` / `ButtonAttachment` wiring (full list `.h:700-719`) |
| 690-712 | `addParameterListener` for ~15 params |
| 845 / 880 | chat & soundboard `ComponentBoundsConstrainer::setSizeLimits(180, 100, 1200, 10000)` |
| 1289-1291 | `updateLayout(); updateState();` |
| 1298-1300 | `setSize(processor.getLastPluginBounds().getWidth/Height())` |
| 1303 | `setResizeLimits(320, 340, 10000, 10000)` |
| 1305 | `commandManager.registerAllCommandsForTarget(this)` |
| 1307-1351 | **standalone branch** (see below) vs `else setResizable(true, true)` |
| 1353-1354 | `processor.addClientListener(this)`; `processor.getTransportSource().addChangeListener(this)` |
| 1357-1361 | `updateUseKeybindings(); commandManager.commandStatusChanged(); setWantsKeyboardFocus(true)` |
| 1363 | `startTimer(PeriodicUpdateTimerId, 1000)` |
| 1365-1369 | Win/Mac standalone only: `startTimer(CheckForNewVersionTimerId, 5000)` |
| 1380-1382 | restore transport from `processor.getCurrentLoadedTransportURL()` |

### 2.3 Standalone vs plugin detection

Always `JUCEApplicationBase::isStandaloneApp()` (or `JUCEApplication::isStandaloneApp()`); `wrapperType` is **never** read in the editor. Occurrences: `.cpp:1206, 1307, 1366, 1457, 1940, 2027(indirect), 3341, 3499, 3714, 3972, 5341`.

In the standalone branch (`:1307-1351`):
- `processor.startAooServer()` (desktop only)
- `setResizable(true, false)`
- `commandManager.registerAllCommandsForTarget(JUCEApplication::getInstance())` (pulls in `StandardApplicationCommandIDs::quit`)
- builds the menu bar (§1.8)
- clipboard link check

Plugin branch: `setResizable(true, true)` (with corner resizer).

Also `SonobusPluginProcessor.cpp:780-789`: plugin defaults `mDry = 1.0`, `mSendChannels = 0`, `mSyncMetToHost = true`; standalone `mDry = 0.0`, `mSyncMetToHost = false`. `mMetSyncButton` is only added when **not** standalone (`.cpp:1206-1208`).

### 2.4 Timers

Two ids, enum at `.cpp:33-36`:

- **`PeriodicUpdateTimerId = 0`, 1000 ms** (`timerCallback` `:1862-1957`). Each tick: `updatePeerState()`, `updateChannelState()`; `updateState()` if `currGroup`/`currConnected` drifted; status-label fade-out; record elapsed time via `SonoUtility::durationToString(processor.getElapsedRecordTime(), true)`; connection elapsed time via `processor.getElapsedConnectedTime()`; lazy `CustomTooltipWindow` creation + one-shot focus grab; sync chat/soundboard visibility against `processor.getLastChatShown()/getLastSoundboardShown()`; `mChatButton->setToggleState(mChatView->haveNewSinceLastView())`; show/hide the peer-recording indicator; iOS IAA state poll.
- **`CheckForNewVersionTimerId = 1`, 5000 ms one-shot** (`:1958-1967`), Windows/macOS standalone only. Calls `LatestVersionCheckerAndUpdater::getInstance()->checkForNewVersion(false)` if `getShouldCheckForNewVersionValue()` is true, then `stopTimer`.

Sub-views run their own timers: `PeersContainerView` `FillRatioUpdateTimerId = 0` at 100 ms (`PeersContainerView.cpp:35-36, 964, 1939`), started only while a jitter meter needs it. `OptionsView::timerCallback` is an empty stub (`OptionsView.cpp:537-540`).

### 2.5 Sizes

| Item | Value | Location |
|---|---|---|
| Initial editor size | `processor.getLastPluginBounds()` → default **800 × 600** | `.cpp:1298-1300`; `SonobusPluginProcessor.h:1063-1064` |
| Editor resize limits | `setResizeLimits(320, 340, 10000, 10000)` | `.cpp:1303` |
| Standalone resizable | `setResizable(true, false)` | `.cpp:1311` |
| Plugin resizable | `setResizable(true, true)` | `.cpp:1350` |
| Narrow / really-narrow thresholds | desktop 690 / 380; mobile 480 / 360 (plus chat & soundboard widths) | `.cpp:4435-4450` |
| Options callout | desktop 340 × 400, mobile 320 × 420, clamped to `dw->getWidth()-30` / `dw->getHeight()-90`, height overridden by `mOptionsView->getPreferredContentBounds()` | `.cpp:3444-3483` |
| Audio settings dialog (holder path, unused by the editor) | 500 × 550 | `SonoStandaloneFilterWindow.h:334` |
| Chat / soundboard size limits | 180–1200 wide, 100–10000 tall | `.cpp:845, 880` |
| Chat overlay threshold | overlay when `width - chatwidth < 340` | `.cpp:4483` |

Window size is persisted through the editor: `resized()` calls `processor.setLastPluginBounds(getLocalBounds())` (`.cpp:4473`), stored as `lastWindowWidth`/`lastWindowHeight` in the processor's extra-state tree.

### 2.6 `resized()` / layout approach

`SonobusAudioProcessorEditor::resized()` — `.cpp:4428-~4620`. Pure **FlexBox**: ~45 `FlexBox` members declared at `.h:605-673` (`mainBox`, `titleBox`, `knobBox`, `remoteBox`, `inputBox`, `transportBox`, `metBox`, `effectsBox`, `reverb*Box`, …). Structure of `resized()`:

1. recompute `isNarrow` / `isReallyNarrow`, call `updateLayout()` on change (:4452-4464)
2. `processor.setLastPluginBounds(mainBounds)` (:4473)
3. carve out the menu bar (:4476-4479)
4. position `mChatView` / `mSoundboardView` from the right, guarded by `mIgnoreResize` (:4481-4505)
5. `mTopLevelContainer->setBounds(getLocalBounds())`; **`mainBox.performLayout(mainBounds)`** (:4508-4510)
6. size `mInputChannelsContainer` and `mPeerContainer` inside `mMainViewport` from their `getMinimumContentBounds()` (:4518-4547)
7. manual `setCentrePosition` for `mSetupAudioButton` / `mMainMessageLabel` (:4554-4561)

`updateLayout()` (`.cpp:4625-…`) is the ~500-line function that (re)populates every `FlexBox.items`. `PeersContainerView::internalSizesChanged` simply calls `resized()` (`.cpp`, `internalSizesChanged`).

### 2.7 Keyboard: `keyPressed`, `keyStateChanged`, command manager

`keyPressed` — `.cpp:3359-3389`. Only three things: **push-to-talk on bare `T`** (unmutes send, mutes recv, guarded by `processor.getDisableKeyboardShortcuts()`), **Escape** forwarded to `mConnectView->escapePressed()` when visible, then `mSoundboardView->processKeystroke(key)`.
`keyStateChanged` — `:3391-3430`: releases push-to-talk when `T` comes up; a disabled block for Alt→menu-bar focus on Windows/Linux sits inside `#if 0`.

Everything else is an `ApplicationCommandTarget`:

- Command ids: `SonobusTypes.h:7-43`, `class SonobusCommands` — `MuteAllInput = 1`, `MuteAllPeers`, `TogglePlayPause`, `ToggleLoop`, `TrimSelectionToNewFile`, `CloseFile`, `Connect`, `Disconnect`, `ShareFile`, `RevealFile`, `ShowOptions`, `OpenFile`, `RecordToggle`, `CheckForNewVersion`, `LoadSetupFile`, `SaveSetupFile`, `ChatToggle`, `SoundboardToggle`, `SkipBack`, `ShowFileMenu`, `ShowTransportMenu`, `ShowViewMenu`, `ShowConnectMenu`, `ShowGroupMenu`, `ToggleFullInfoView`, `StopAllSoundboardPlayback`, `ToggleAllMonitorDelay`, `CopyGroupLink`, `GroupLatencyMatch`, `VDONinjaVideoLink`, `SuggestNewGroup`, `ResetAllJitterBuffers`.
- `getCommandInfo` `.cpp:5408-5705`; `getAllCommands` `:5707-…`; `perform` (`MuteAllInput` toggles `mMainMuteButton`, `MuteAllPeers` toggles `mMainRecvMuteButton`).
- **Mute keybindings** (`:5413-5431`): `MuteAllInput` → bare **`m`** *and* **`Cmd/Ctrl+M`**; `MuteAllPeers` → **`Cmd/Ctrl+U`**.
- Other notable defaults: Space / Cmd+P play-pause; Alt+L loop; `0`/Cmd+0 skip back; Cmd+T trim; Cmd+W close file; Cmd+O open; Cmd+S save setup; Cmd+Y chat; Cmd+G soundboard; Cmd+K stop soundboard; Cmd+B monitor delay; Cmd+N connect; Cmd+D disconnect; Cmd+, options; Cmd+R record; Cmd+I full-info view; Alt+F/C/G/V/T for the menus. All guarded by `bool useKeybindings = !processor.getDisableKeyboardShortcuts()` (`:5409`).
- `SonobusCommandManager` (`.h:563-571`) always returns `&parent` as first target; `getNextCommandTarget()` returns `findFirstTargetParentComponent()` (`.h:95-97`).
- `updateUseKeybindings()` — `.cpp:1452-1482`: clears + re-registers commands, `getKeyMappings()->resetToDefaultMappings()`, then `addKeyListener(commandManager.getKeyMappings())` or `removeKeyListener` depending on `processor.getDisableKeyboardShortcuts()`.

### 2.8 ClientListener wiring

`processor.addClientListener(this)` at `.cpp:1353`; removed at `:1418`. Every callback (`.cpp:1579-1760`) does the same thing: take `clientStateLock`, push a `ClientEvent` onto `clientEvents`, `triggerAsyncUpdate()`. `ClientEvent`/`Type` are defined at `.h:459-501`; the arrays at `.h:501-505`.

`handleAsyncUpdate()` — `.cpp:3861-…` — drains the array and acts. Notable:

- `ConnectEvent` (:3881): on `"access denied"` auto-increments the username via `generateNewUsername()` and reconnects; on success joins `currConnectionInfo.groupName`, or enables `processor.setWatchPublicGroups(true)` + `mConnectView->updatePublicGroups()` when no group is set.
- `GroupJoinEvent` success (:3961-3985): `showConnectPopup(false)`; adds a `SBChatEvent::SystemType` message `"Joined Group: " + group`; calls `saveSettingsIfNeeded()` when standalone; `updateLayout(); resized();`; focuses `mMainMessageLabel`.
- `PeerChangedState` (:3872): `updatePeerState(true); updateState(false);`
- Chat events arrive separately through `sbChatEventReceived` (`.cpp:1723`) into `newChatEvents` / `haveNewChatEvents` (`.h:503-505`).

`ConnectView` is shown/hidden by `showConnectPopup(bool)` — `.cpp:3191-3205`. It is **not** a callout: it's a child component added with `addChildComponent(mConnectView.get())` (`.cpp:1270`) and simply made visible + `toFront` + `updateState()` + `grabInitialFocus()`.

### 2.9 Callouts / popups

Two mechanisms:

- plain `juce::CallOutBox::launchAsynchronously(std::move(wrap), bounds, dw, false)` — used for met config (`:2918`), effects (`:2970`), patchbay (`:3015`), VDO Ninja (`:3119`), **settings/options** (`:3490`), monitor delay (`:3560`), latency-match (`:4258`). Followed by `box->setDismissalMouseClicksAreAlwaysConsumed(true)`.
- **`SonoCallOutBox`** (`/Users/seungchan/github/meon/Source/SonoCallOutBox.h`, `.cpp`) — a `CallOutBox` subclass adding a `canPassthrough` predicate (`canModalEventBeSentToComponent`, `.cpp:55-61`) and a `wasHidden` callback (`.cpp:63-70`). `launchAsynchronously(content, area, parent, dismissIfBackgrounded=true, canPassthroughFunc)` (`.cpp:48-53`); when `dismissIfBackgrounded` it runs a 200 ms timer that dismisses when `!Process::isForegroundProcess()` (`.cpp:35-39`). Used for latency-match (`:3063`), suggested-group (`:3173`, `:4331`).

Each callout target is tracked by a `WeakReference<Component>` member (`.h:430-448`). Bounds are computed as `dw->getLocalArea(nullptr, someComponent->getScreenBounds().reduced(10))`, with `dw = this` (the editor itself is the callout parent).

Transient tips: `showPopTip(message, timeoutMs, target, maxwidth)` — `.cpp` (lazy `BubbleMessageComponent`, placement above|below, 12 pt font). Hover tips: `CustomTooltipWindow` (`.h:676-698`) suppresses tooltips while a `popTip` is showing.

### 2.10 `PeersContainerView` ↔ processor

`/Users/seungchan/github/meon/Source/PeersContainerView.h` / `.cpp` (2429 lines). One `PeerViewInfo` (`.h:22-166`) per remote peer, held in `OwnedArray<PeerViewInfo> mPeerViews`.

`rebuildPeerViews()` / `updatePeerViews(int specific=-1)` (`.h:232-233`) read **only** through the processor. The main update loop is `PeersContainerView.cpp:~1620-1830`:

| UI element | Processor getter | Format |
|---|---|---|
| `addrLabel` | `getRemotePeerAddressInfo(i, host, port)` | `"<host> : <port>"`, or `TRANS("<Press to show>")` until clicked — :1652-1661 |
| `sendActualBitrateLabel` | `getRemotePeerBytesSent(i)` deltas | `" %.d kb/s"`; or `TRANS("Other end BLOCKED us")` / `TRANS("Other end muted us")` / `TRANS("SEND DISABLED")` — :1690-1707 |
| `recvActualBitrateLabel` | `getRemotePeerRecvChannelCount`, `getRemotePeerReceiveAudioCodecFormat`, `getRemotePeerBytesReceived`, `getRemotePeerPacketsDropped`, `getRemotePeerPacketsResent` | `"<n>ch <fmt> \| %d kb/s [\| %d drop] [\| %d resent]"`; text turns `droppedTextColor` for 1500 ms after a drop increments — :1709-1750 |
| `pingLabel` | `getRemotePeerLatencyInfo(i, latinfo)` → `latinfo.pingMs` | `String::formatted("%d", (int)lrintf(pingMs))` — :1764 |
| `latencyUpLabel` / `latencyDownLabel` | `latinfo.outgoingMs` / `latinfo.incomingMs` | integer ms; `"***"` while testing, `TRANS("PRESS")` when `latinfo.legacy && !latinfo.isreal` — :1766-1782 |
| `bufferLabel` | `getRemotePeerBufferTime(i)`, `getRemotePeerAutoresizeBufferMode(i, initCompleted)` | `"%d ms"` + `""` / `" (Auto+)"` / `" (IA-Man)"` / `" (IA-Auto)"` / `" (Auto)"` — :1788-1796 |
| `sendQualityLabel` | `getRemotePeerActualSendChannelCount`, `getAudioCodeFormatName(getRemotePeerAudioCodecFormat(i))` | `"<n>ch <fmtname>"` — :1811-1813 |
| `jitterBufferMeter` | `getRemotePeerReceiveBufferFillRatio(i, ratio, stdev)` | polled at 100 ms in `timerCallback` — :1947 |
| mute/solo/connected states | `getRemotePeerConnected/SendActive/SendAllow/RecvActive/RecvAllow/SafetyMuted/BlockedUs`, `isRemotePeerLatencyTestActive` | :1647-1671 |

Latency popup text (`generateLatencyMessage`, :1926-1930):
```
TRANS("Estimated Round-trip Latency:") + " %d ms"
TRANS("Round-trip Network Ping:")      + " %.1f ms"
TRANS("Est. Outgoing:")                + " %.1f ms"
TRANS("Est. Incoming:")                + " %.1f ms"
```

Pending/blocked peers use `PendingPeerViewInfo` (`.h:168-185`) with a `std::map<String,PendingUserInfo> mPendingUsers`, fed by the editor's `peerPendingJoin/peerFailedJoin/peerBlockedJoin/peerLeftGroup` calls.

`PeersContainerView` also needs `getAudioDeviceManager` (`.h:255`) — set by the standalone window (§1.5).

### 2.11 `OptionsView` and audio settings

`/Users/seungchan/github/meon/Source/OptionsView.h` / `.cpp` (1391 lines). Constructed lazily inside `showSettings(true)` (`SonobusPluginEditor.cpp:3454-3466`) with the function hooks `getShouldOverrideSampleRateValue`, `getShouldCheckForNewVersionValue`, `getAllowBluetoothInputValue`, `updateSliderSnap`, `setupLocalisation`, `saveSettingsIfNeeded`, `updateKeybindings`.

Three tabs in a `TabbedComponent` (`OptionsView.cpp:96`): **AUDIO** (standalone only), **OPTIONS**, **RECORDING** (`:488-502`). A `SonobusOptionsTabbedComponent` subclass is declared at `:14-17`.

`AudioDeviceSelectorComponent` construction — `OptionsView.cpp:470-482`:

```cpp
mAudioDeviceSelector = std::make_unique<AudioDeviceSelectorComponent>(*getAudioDeviceManager(),
                          minNumInputs, maxNumInputs,
                          minNumOutputs, maxNumOutputs,
                          false,   // showMidiInputOptions
                          false,   // showMidiOutputSelector
                          false,   // showChannelsAsStereoPairs
                          false);  // hideAdvancedOptionsWithButton
#if JUCE_IOS || JUCE_ANDROID
mAudioDeviceSelector->setItemHeight(44);
#endif
mAudioOptionsViewport->setViewedComponent(mAudioDeviceSelector.get(), false);
```

Guarded by `if (JUCEApplicationBase::isStandaloneApp() && getAudioDeviceManager && getAudioDeviceManager())` (`:419`). Channel min/max are derived from `processor.getBus(true/false, 0)->getMaxSupportedChannels(128)` etc. (`:440-466`). Re-laid-out in `resized()` at `:950-951`.

The `StandalonePluginHolder::showAudioSettingsDialog()` path (`SonoStandaloneFilterWindow.h:329-344`, `SettingsComponent`, 500 × 550) exists but is only reachable from the window's own Options button, which is only visible when the native title bar is off.

### 2.12 Direct processor state access from the UI

The editor reads/writes processor state three ways:

1. **`processor.getValueTreeState()`** — 60 call sites in `SonobusPluginEditor.cpp`. Attachments at `:449-687`, `addParameterListener` at `:690-712`, `removeParameterListener` at `:1401-1414`. Direct parameter pokes for push-to-talk at `:3370-3372` and `:3399-3400` (`getParameter(paramMainSendMute)->setValueNotifyingHost(...)`). `PeersContainerView.cpp:2052` also pokes `paramMainMonitorSolo`.
2. **Plain getters/setters on the processor** — `getRecentServerConnectionInfos`, `getCurrentUsername`, `getCurrentJoinedGroup`, `isConnectedToServer`, `getNumberRemotePeers`, `getLastChatShown/Width`, `getLastSoundboardShown/Width`, `getPeerDisplayMode`, `getDisableKeyboardShortcuts`, `getUseUniversalFont`, `getLanguageOverrideCode`, `getSupportDir`, `getLastPluginBounds`/`setLastPluginBounds`, `getAllChatEvents`, `sendChatEvent`, `startAooServer`, `setWatchPublicGroups`, `joinServerGroup`, `addRecentServerConnectionInfo`, `getTransportSource`.
3. **Shell-owned `Value`s via `std::function`** — `getShouldOverrideSampleRateValue`, `getShouldCheckForNewVersionValue`, `getAllowBluetoothInputValue` (§1.5). These live in `StandalonePluginHolder`, not in the processor.

Persisted "extra state" keys (`SonobusPluginProcessor.cpp:85-134`; written `:8511-8549`, read `:~8690-8715`) relevant to the UI:

```
lastUsername          lastWindowWidth / lastWindowHeight
langOverrideCode      useUnivFont
lastChatShown / lastChatWidth       chatFixedWidthFont / chatFontSizeOffset
lastSoundboardShown / lastSoundboardWidth
PeerDisplayMode       DisableKeyShortcuts       SliderSnapToMouse
```

`lastUsername` specifically: `static String lastUsernameKey("lastUsername")` (`:121`), written at `:8541` from `mCurrentUsername`, read at `:8706`. The editor consumes it at `SonobusPluginEditor.cpp:344-347`. It lives in the plugin state (which the standalone stores under `filterStateXML` in `SonoBus.settings`), **not** in the PropertiesFile directly.

Again: **there is no "first run" boolean anywhere in the codebase.**

---

## 3. Localization

### 3.1 Loading

`SonobusAudioProcessorEditor::setupLocalisation(const String& overrideLang)` — `SonobusPluginEditor.cpp:5302-5395`. Declared `.h:179`. Called from the editor ctor at `:297` and from `OptionsView` (`.cpp:1258`) when the user changes language.

Algorithm:

1. `lang = displang = SystemStats::getDisplayLanguage()` (:5304-5305).
2. If the bare language is `nl` or `ja`, **force `en-us`** — "currently ignore the system default language if it's one of our non-vetted translations" (:5309-5315).
3. If `overrideLang` is non-empty it replaces both (:5317-5319).
4. `LocalisedStrings::setCurrentMappings(nullptr)` (:5321).
5. Build four names (:5329-5335):
   - `sflang` = full lang, `-` stripped, lowercased → resource `localized_<sflang>_txt`
   - `slang` = bare lang → resource `localized_<slang>_txt`
   - file names `localized_<lang>.txt` and `localized_<slang>.txt`
6. Lookup order (:5349-5377): **user file in `mSettingsFolder`** (standalone only, `:5341-5346`) → **full-locale BinaryData resource** → **bare-language BinaryData resource** → if `lang.startsWith("en")` succeed with no mappings → fail.
7. `mActiveLanguageCode = displang` on success, else `"en-us"` (:5383-5387).

`BinaryData::getNamedResource(resname.toRawUTF8(), retbytes)` is used (:5337-5338), so resource names are resolved at runtime by string.

Language list for the UI: `OptionsView::initializeLanguages()` — `OptionsView.cpp:39-78`. Codes: `""` (system default), `en`, `es`, `fr`, `it`, `de`, `pt-pt`, `pt-br`, `nl`, `ja`, `ko`, `zh-hans`, `ru`. Native names for ja/ko/zh are only shown when `processor.getUseUniversalFont()` is true — otherwise the English name is used because the default font cannot render them (comment at `:56`).

Changing the language shows an ok/cancel alert (`"In order to change the language, the application must be closed and restarted by you."`), then `setupLocalisation(code)`, `processor.setLanguageOverrideCode(langOverride)`, `saveSettingsIfNeeded()`, and **quits the app after 500 ms** (`OptionsView.cpp:1235-1276`).

### 3.2 Binary data

`CMakeLists.txt:465-479` — `juce_add_binary_data("${target_name}_SBData" ...)` includes `Source/GoNotoKurrent-Regular.ttf`, `Source/DejaVuSans.ttf`, and `localization/localized_{de,es,fr,it,ja,nl,pt-br,pt-pt,ko,ru,zh-hans}.txt` (11 files; `localized_en.txt` does not exist — English is the untranslated source).

### 3.3 `localized_ko.txt` format

`/Users/seungchan/github/meon/localization/localized_ko.txt` (47681 bytes). Standard JUCE `LocalisedStrings` format. First lines:

```
language: Korean
countries: ko

" - joined group" = "님이 그룹에 참가했습니다"
" - left group" = "님이 그룹에서 나갔습니다"
" active user" = " 접속 중인 유저"
```

Header lines `language:` / `countries:`, blank line, then `"source" = "translation"` pairs sorted by source string. Escapes use backslash (`\'`, `\n`). Constructed with `LocalisedStrings(text, /*ignoreCaseOfKeys*/ true)` (`SonobusPluginEditor.cpp:5351, 5358, 5365`).

`localization/tsv/` holds a source-of-truth TSV directory.

### 3.4 CJK font handling

`SonoLookAndFeel::getTypefaceForFont` — `/Users/seungchan/github/meon/Source/SonoLookAndFeel.cpp:178-254`. Only intercepts when `font.getTypefaceName() == Font::getDefaultSansSerifFontName()`. Language comes from `languageCode` if set, else `SystemStats::getUserLanguage()`.

**Non-universal path** (`mUseUniversalFont == false`, :191-241) — per-platform system faces:

| lang | macOS/iOS | Windows | Android |
|---|---|---|---|
| `ja` | `Hiragino Sans W3` | `Arial Unicode MS` | embedded `DejaVuSans.ttf` |
| `ko` | `Apple SD Gothic Neo` | `Malgun Gothic` | embedded `DejaVuSans.ttf` |
| `zh` | `PingFang SC` | `Arial Unicode MS` | embedded `DejaVuSans.ttf` |
| other | embedded `DejaVuSans.ttf` (:237) | same | same |

**Universal path** (`mUseUniversalFont == true`, :242-251): always
```cpp
return Typeface::createSystemTypefaceFor (BinaryData::GoNotoKurrentRegular_ttf,
                                          BinaryData::GoNotoKurrentRegular_ttfSize);
```

`SonoLookAndFeel::setDefaultSansSerifTypeface` is **never called** — the three `setDefaultSansSerifTypefaceName` lines at `:145-147` are commented out. All font substitution goes through `getTypefaceForFont`.

`setLanguageCode(lang, useUniversalFont)` — `:161-174` — only stores the two values; the per-language `fontScale` tweaks (`zh` 1.0, `ko` 1.15) are commented out. Scale is instead set once by the editor ctor (`SonobusPluginEditor.cpp:278-289`).

Toggling the universal font (`OptionsView.cpp:1178-1216`) also requires an app restart.

`GoNotoKurrent-Regular.ttf` is 15.5 MB; `DejaVuSans.ttf` is 757 KB. Both are compiled into `BinaryData` unconditionally.

---

## 4. `SonoLookAndFeel`

`/Users/seungchan/github/meon/Source/SonoLookAndFeel.h` (183 lines) / `.cpp` (1772 lines). Four classes: `SonoLookAndFeel : LookAndFeel_V4, foleys::LevelMeter::LookAndFeelMethods` (`.h:10`), `SonoBigTextLookAndFeel` (`.h:135`, ctor takes `maxTextSize`, default 32), `SonoPanSliderLookAndFeel` (`.h:154`, default 14), `SonoDashedBorderButtonLookAndFeel` (`.h:177`).

### 4.1 Colours set in the ctor (`.cpp:46-159`)

Base scheme: `setColourScheme(getDarkColourScheme())` then overrides `windowBackground = rgba(0,0,0,1)`, `widgetBackground = rgba(.1,.1,.1,1)`, `outline = rgba(.3,.3,.3,.5)` (`:55-59`).

| Colour id | Value | Line |
|---|---|---|
| `Label::textColourId` | `0xffcccccc` | 61 |
| `Label::textWhenEditingColourId` | `0xffe9e9e9` | 62 |
| `ResizableWindow::backgroundColourId` | `0xff111111` | 64 |
| `TextButton::buttonColourId` | `rgba(.15,.15,.15,.7)` | 67 |
| `TextButton::buttonOnColourId` | `rgba(.5,.4,.6,.8)` | 70 |
| `TextButton::textColourOnId` / `textColourOffId` | `0xddcccccc` / `0xdde9e9e9` | 71-72 |
| `ToggleButton::textColourId` / `tickColourId` | `0xddcccccc` / `rgba(.4,.8,1,1)` | 74, 138 |
| `SonoTextButton::outlineColourId` | `rgba(.3,.3,.3,.5)` | 77 |
| `ScrollBar::thumbColourId` | `rgba(.4,.4,.4,.6)` | 79 |
| `ComboBox::background/text/outline` | `rgba(.15,.15,.15,.7)` / `0xffe9e9e9` / `rgba(.3,.3,.3,.5)` | 82-84 |
| `TextEditor::background/text/highlight/outline/focusedOutline` | `0xff050505` / `0xffe9e9e9` / `0xff5959f9` / `rgba(.3,.3,.3,.5)` / `rgba(.5,.5,.5,.7)` | 86-90 |
| `Slider::background`, `rotarySliderOutline` | `rgba(.2,.2,.2,1)` | 92-93 |
| `Slider::textBoxText/Background/Highlight/Outline` | `0xddcccccc` / `rgba(.05,.05,.05,1)` / `0xaa555555` / `rgba(.3,.3,.3,.5)` | 94-97 |
| `Slider::trackColourId` | `rgba(.1,.4,.6,.8)` | 99 |
| `Slider::thumbColourId`, `rotarySliderFillColourId` | `rgba(.5,.4,.6,.9)` | 100-102 |
| `TabbedButtonBar::tabOutlineColourId` | `rgba(.3,.3,.3,.5)` | 104 |
| `ListBox::background/outline` | `rgba(.15,.15,.15,.7)` / `rgba(.3,.3,.3,.5)` | 108-109 |
| `BubbleComponent::background/outline` | `rgba(.25,.25,.25,1)` / `rgba(.4,.4,.4,.5)` | 111-112 |
| `TooltipWindow::text/background` | `0xee222222` / `0xeeffff99` | 114-115 |
| `PopupMenu::background/highlightedBackground` | `rgba(.2,.2,.2,1)` / `rgba(.35,.35,.4,1)` | 117-118 |
| `SidePanel::backgroundColour` | `rgba(.17,.17,.17,1)` | 120 |
| `SonoDrawableButton::overOverlay/downOverlay` | `rgba(.8,.8,.8,.08)` / `rgba(.8,.8,.8,.3)` | 126-127 |
| `DrawableButton::textColourId/textColourOnId/backgroundOnColourId` | `0xffb9b9b9` / `0xffe9e9e9` / `rgba(.5,.4,.6,.8)` | 129-133 |
| `DirectoryContentsDisplayComponent::highlight/text` | `rgba(.1,.4,.6,.9)` / `0xffe9e9e9` | 140-141 |

`setupDefaultMeterColours()` at `:152` (meter colour ids live in the included `LevelMeterLookAndFeelMethods.h`, 882 lines).

### 4.2 Fonts and global sizes

- `static float SonoLookAndFeel::fontScale = 1.0f` (`.cpp:16`, under `#define OLDFONTSTUFF 1`). Accessors `getFontScale()/setFontScale()` at `.h:113-114`. Set by the editor ctor.
- `myFont = Font(16 * fontScale)` (`.cpp:150`) — the base face for buttons, tabs, text.
- `getMenuBarFont` → `menuBar.getHeight() * 0.7f * fontScale` (:275-278)
- `getLabelFont` → passthrough when `fontScale == 1.0f`, otherwise `label.getFont().withHeight(h * fontScale)` (:639-646)
- `getTextButtonFont` → `myFont.withHeight(jmin(16.0f, buttonHeight * textRatio) * fontScale)` (:694-702)
- `getPopupMenuFont` → `Font(17.0f * fontScale)` (:900-903)
- tooltip font 13 pt bold, max width 400 px (`sonoLayoutTooltipText`, :32-44)
- toggle-button text `jmin(15.0f, height * 0.75f) * fontScale` (:958)
- `SonoBigTextLookAndFeel::getTextButtonFont` → `jmin(maxSize, buttonHeight * textRatio) * fontScale` (:1404-1412)
- `SonoPanSliderLookAndFeel::getSliderPopupFont` → `Font(maxSize * fontScale, Font::bold)` (:1512-1514)
- `labelCornerRadius = 4.0f` (`.h:123`), settable via `setLabelCornerRadius`.
- `setUsingNativeAlertWindows(true)` in the ctor (:51) and again from the editor (`SonobusPluginEditor.cpp:293`).

The editor holds five instances: `sonoLookAndFeel`, `sonoSliderLNF(13)`, `smallLNF(14)`, `teensyLNF(11)`, `panSliderLNF(12)` (`.h:261-265`). `PeerViewInfo` holds six more (`PeersContainerView.h:32-38`).

---

## 5. `ChatView`

`/Users/seungchan/github/meon/Source/ChatView.h` (157 lines) / `.cpp` (1076 lines). `ChatView(SonobusAudioProcessor& proc, AooServerConnectionInfo& connectinfo)` — holds a **reference** to the editor's `currConnectionInfo`.

### 5.1 Message store

The chat log lives in the **processor**, not the view: `processor.getAllChatEvents()` returns a mutable `Array<SBChatEvent>&`.

- `addNewChatMessage(mesg, refresh)` — `.cpp:503-510`: `processor.getAllChatEvents().add(mesg)` then render.
- `addNewChatMessages(Array<SBChatEvent>&, refresh)` — `:512-…`: `addArray`.
- `refreshMessages()` — `:524-527`: renders only `size() - lastShownCount` new entries.
- `refreshAllMessages()` — `:539`: `processNewChatMessages(0, size())`.
- `clearAll()` — `:630`: `processor.getAllChatEvents().clearQuick()`.
- Rendering: `processNewChatMessages(index, count)` — `:819-930ish`, walks `SBChatEvent::{SelfType, UserType, SystemType}`, applies per-user colours (`getOrGenerateUserColor`), and appends into a read-only multi-line `TextEditor` (`mChatTextEditor`).

### 5.2 Sending

`commitChatMessage()` — `.cpp:1038-1068`:

```cpp
SBChatEvent event;
event.from    = currConnectionInfo.userName;
event.group   = currConnectionInfo.groupName;
event.message = mChatSendTextEditor->getText();
if (privateChat) event.targets = mChatTabs->getCurrentTabName();
processor.sendChatEvent(event);
event.type = SBChatEvent::SelfType;
processor.getAllChatEvents().add(event);
processNewChatMessages(processor.getAllChatEvents().size()-1, 1);
mChatSendTextEditor->clear();
```

Inbound messages reach the view through the editor: `sbChatEventReceived` (`SonobusPluginEditor.cpp:1723`) queues into `newChatEvents` under `chatStateLock`, and the editor calls `mChatView->addNewChatMessages(...)`.

### 5.3 Unread badges

Two separate indicators:

1. **Toolbar button** — `mChatButton` (`SonobusPluginEditor.cpp:821-840`) is a `SonoDrawableButton` whose *on* image is `chat_dots.svg` and *off* image `chat.svg`; the periodic timer sets `mChatButton->setToggleState(mChatView->haveNewSinceLastView(), dontSendNotification)` (`.cpp:1917`). `haveNewSinceLastView()` is simply `mLastChatUserMessageStamp > mLastChatViewStamp` (`ChatView.cpp:1073-1075`).
2. **Per-tab badge** — `setMesgUnreadForTab(int index, bool flag)` (`ChatView.cpp:721-737`) attaches/removes a 20 × 20 `SonoDrawableButton` built from `BinaryData::mesgunread_svg` as the tab button's `ExtraComponent` (`ExtraComponentPlacement::afterText`). Set when a message arrives for a non-current tab (`:849, 867, 880`); cleared on tab change (`:332`).

Fonts/prefs come from the processor: `getChatUseFixedWidthFont()` / `setChatUseFixedWidthFont()`, `getChatFontSizeOffset()` / `setChatFontSizeOffset()` (`.cpp:96, 278, 284, 558, 592-613, 819`). Chat export writes `SonoBusChat_<timestamp>` (`.cpp:652-656`).

---

## 6. Startup network calls

### 6.1 `VersionInfo.cpp`

`/Users/seungchan/github/meon/Source/VersionInfo.cpp` (121 lines).

- `VersionInfo::fetch(endpoint)` — :68-120 — hits **`https://api.github.com/repos/sonosaurus/sonobus/releases/<endpoint>`** (:70). Optional `Authorization: Basic <base64(GITUSERPASS)>` from the env var (:72-76). Parses JSON `tag_name`, `body`, `assets[]`.
- `fetchLatestFromUpdateServer()` → `fetch("latest")` (:13-16); `fetchFromUpdateServer(v)` → `fetch("tags/"+v)` (:8-11).
- `createInputStreamForAsset` — :18-33 — `Accept: application/octet-stream`, 1 redirect.
- **No request is made to `sonobus.net` itself** — that domain only appears in an alert string. `LV2URI "https://sonobus.net/lv2/sonobus"` in `CMakeLists.txt:174` is metadata, not a request.

### 6.2 `AutoUpdater.cpp`

`/Users/seungchan/github/meon/Source/AutoUpdater.cpp` (595 lines), `AutoUpdater.h` — `LatestVersionCheckerAndUpdater : DeletedAtShutdown, private Thread`, `JUCE_DECLARE_SINGLETON_SINGLETHREADED_MINIMAL`.

- `checkForNewVersion(bool showAlerts)` — :48-55 — starts the background thread if not already running.
- `run()` — :58-119 — `VersionInfo::fetchLatestFromUpdateServer()`, then `isNewerVersionThanCurrent()`, then looks for an asset named `sonobus-<version>-<mac|win|linux>.*` and, if found, `MessageManager::callAsync(askUserAboutNewVersion)`. When `showAlerts` is false (the automatic path), failures are silent.
- Download/install: `DownloadAndInstallThread : ThreadWithProgressWindow` (:296-…); on completion relaunches `SonoBus.app/Contents/MacOS/SonoBus` / `SonoBus.exe` / `SonoBus` after the old process exits (:555-570) and calls `JUCEApplicationBase::getInstance()->systemRequestedQuit()` (:578).

### 6.3 Invocation points and how to disable

| Trigger | Location |
|---|---|
| Automatic, 5 s after editor construction | `SonobusPluginEditor.cpp:1365-1369` (start timer) → `:1958-1967` (fire) — **`#if (JUCE_WINDOWS \|\| JUCE_MAC)` and `JUCEApplicationBase::isStandaloneApp()` only** |
| Manual, File ▸ Check For New Version | `SonobusCommands::CheckForNewVersion`, info at `:5593-5598`, menu item at `:5982` |
| Commented-out ctor call | `:1347` |

To disable: the automatic check is already gated on the user preference — `getShouldCheckForNewVersionValue()` returns `&StandalonePluginHolder::shouldCheckForNewVersion` (`SonoStandaloneFilterWindow.h:194`), default `true` (`:101`), persisted as `shouldCheckForNewVersion` in the settings file (`:355`, `:396`), exposed in the Options tab. The simplest hard kill is to not call `startTimer(CheckForNewVersionTimerId, 5000)` and to drop `SonobusCommands::CheckForNewVersion` from `getAllCommands`/the File menu — a new editor that never does either makes zero startup network calls from the UI layer. (The processor's own AOO networking is separate and unaffected.)

---

## 7. Build identifiers and name strings

### 7.1 `CMakeLists.txt`

| Setting | Value | Line |
|---|---|---|
| `project()` | `SonoBus VERSION 1.7.2` | 44 |
| `BUILDVERSION` | `80` | 46 |
| `FormatsToBuild` | `VST3 Standalone` (+ AAX/VST2 when SDK paths are set) | 90, 113, 119 |
| `COMPANY_NAME` | `"Sonosaurus"` | 163 |
| `BUNDLE_ID` | `"com.Sonosaurus.SonoBus"` | 164 |
| `MICROPHONE_PERMISSION_ENABLED` | `TRUE` | 165 |
| `ICON_BIG` | `images/sonobus_icon_mac_1024.png` | 167 |
| `ICON_SMALL` | `images/sonobus_icon_mac_256.png` | 168 |
| `LV2URI` | `https://sonobus.net/lv2/sonobus` | 174 |
| `HARDENED_RUNTIME_ENABLED` / `OPTIONS` | `TRUE` / `com.apple.security.device.audio-input` | 177-178 |
| `PLIST_TO_MERGE` | `${MacPList}` | 179 |
| `AU_MAIN_TYPE` | `kAudioUnitType_MusicEffect` | 180 |
| `PLUGIN_MANUFACTURER_CODE` | `Sono` | 184 |
| `PLUGIN_CODE` | `NBus` (effect) / `IBus` (instrument) | 185, 629, 635 |
| `DESCRIPTION` | `"SonoBus - Network Audio"` | 187 |
| `PRODUCT_NAME` | `SonoBus` / `SonoBusInstrument` | 188, 629, 635 |
| `SONOBUS_BUILD_VERSION` | `"${VERSION}"` compile define | 462 |
| Linux exe name | lower-cased target → `sonobus` | 592-598 |

`MacPList` (:123-139) supplies `CFBundleVersion = 80` and the `CFBundleURLTypes` block (`net.sonobus` / scheme `sonobus`).

Two targets are generated: `sono_add_custom_plugin_target(SonoBus SonoBus "${FormatsToBuild}" FALSE "NBus")` (:629) and `sono_add_custom_plugin_target(SonoBusInst SonoBusInstrument "VST3" TRUE "IBus")` (:635). A commented-out mobile target sits at :632.

### 7.2 "SonoBus" in source strings

| File:line | String |
|---|---|
| `Source/SonobusPluginEditor.cpp:349` | `mTitleLabel = Label("title", TRANS("SonoBus"))` — the on-screen title, 20 pt, `0xff47b0f8` |
| `Source/SonobusPluginEditor.cpp:2552, 2599` | commented-out `"SonoBus Setups"` documents folder |
| `Source/SonoStandaloneFilterApp.cpp:86` | `osxLibrarySubFolder = "Application Support/SonoBus"` |
| `Source/SonoStandaloneFilterApp.cpp:97` | legacy `~/.config/SonoBus.settings` |
| `Source/SonobusPluginProcessor.cpp:742, 744` | `applicationName = "SonoBus"`, same `osxLibrarySubFolder` |
| `Source/SonobusPluginProcessor.cpp:603` | ValueTree type id `"SonoBusAoO"` |
| `Source/SonobusPluginProcessor.cpp:769` | default record dir `~/Music/SonoBus` |
| `Source/SonobusPluginProcessor.cpp:417-516` | thread names `SonoBusSendThread`, `SonoBusRecvThread`, `SonoBusEventThread`, `SonoBusServerThread`, `SonoBusClientThread` |
| `Source/AutoUpdater.cpp:67, 69, 79, 127, 132, 207, 469, 517, 560-570` | update dialog copy + relaunch paths (`SonoBus.app`, `SonoBus.exe`, `killall -0 SonoBus`) |
| `Source/ChatView.cpp:652, 656` | chat export filename `SonoBusChat_<ts>` |
| `Source/ConnectView.cpp:89` | "Connect directly to other instances of SonoBus…" |
| `Source/ConnectView.cpp:868` | "Share this link with others to connect with SonoBus:" |
| `Source/PeersContainerView.cpp:1863` | firewall/NAT help text mentioning SonoBus |
| `Source/SonobusPluginProcessor.cpp:2397, 2410` | debug strings |

The window title is `JucePlugin_Name` via `getApplicationName()` (`SonoStandaloneFilterApp.cpp:115`, used at `:218`). **There is no About dialog** — the mac Apple-menu "about" case at `SonobusPluginEditor.cpp:6033-6035` is an empty `break;`.

### 7.3 Release / packaging files referencing the names (list only)

`/Users/seungchan/github/meon/release/`: `SonoBus.entitlements`, `SonoBusPkg.plist`, `SonoBusLayout/Info.plist`, `SonoBusPkgLayout/Info.plist`, `macpkg/SonoBus.pkgproj`, `macpkg/intro.txt`, `buildmac.sh`, `buildwin.sh`, `codesign.sh`, `distmac.sh`, `distwin.sh`, `makedmg.sh`, `makepkgdmg.sh`, `notarize-app.sh`, `notarizedmg.sh`, `pushrelease.sh`, `update_package_version.py`, `wininstaller.iss`.

`/Users/seungchan/github/meon/scripts/`: `SonoBus-mac.entitlements`, `SonoBus-mac-sandbox.entitlements`, `mac_prebuild.sh`.

`/Users/seungchan/github/meon/linux/`: `sonobus.desktop`, `install.sh`, `uninstall.sh`, `deb_get_prereqs.sh`, `fedora_get_prereqs.sh`, `BUILDING.md`, `build.sh`.

`/Users/seungchan/github/meon/snap/`: `snapcraft.yaml`, `gui/sonobus.desktop`.

`/Users/seungchan/github/meon/mobile/` (separate Projucer project for iOS/Android, not built by the root CMake): `SonoBusMobile.jucer`, `Builds/iOS/SonoBus.xcodeproj/project.pbxproj`, `Builds/iOS/Info-Standalone_Plugin.plist`, `Builds/iOS/Info-AUv3_AppExtension.plist`, `Builds/Android/app/src/main/AndroidManifest.xml`, `Builds/Android/app/build.gradle`, `Builds/Android/app/CMakeLists.txt`, `Builds/Android/settings.gradle`, `Builds/Android/app/src/{debug,release}/res/values/string.xml`, `Builds/Android/copyrelease.sh`, `JuceLibraryCode/{JuceHeader.h,JucePluginDefines.h,BinaryData.h,BinaryData.cpp,BinaryData2.cpp}`.

Build helper scripts at repo root: `buildcmake.sh`, `setupcmake.sh`, `setupcmakewin.sh`, `setupcmakewin32.sh`, `setupcmakexcode.sh`.

---

## 8. Minimum contract for a replacement editor

A new `AudioProcessorEditor` subclass used in place of `SonobusAudioProcessorEditor` must provide, because the shell calls them unconditionally via `dynamic_cast<SonobusAudioProcessorEditor*>`:

| Member | Called from |
|---|---|
| `std::function<AudioDeviceManager*()> getAudioDeviceManager` | `SonoStandaloneFilterWindow.h:1062` |
| `getInputChannelGroupsView()` returning something with `getAudioDeviceManager` | `:1063` |
| `getPeersContainerView()` returning something with `getAudioDeviceManager` | `:1064` |
| `isInterAppAudioConnected`, `getIAAHostIcon`, `switchToHostApplication` | `:1065-1067` |
| `getShouldOverrideSampleRateValue`, `getAllowBluetoothInputValue`, `getShouldCheckForNewVersionValue` | `:1068-1070` |
| `getRecentSetupFiles`, `getLastRecentsFolder` | `:1071-1072` |
| `std::function<void()> saveSettingsIfNeeded` | `SonoStandaloneFilterApp.cpp:461` |
| `void handleURL(const String&)` | `SonoStandaloneFilterApp.cpp:706, 723` |
| `void connectWithInfo(const AooServerConnectionInfo&, bool, bool)` | `SonoStandaloneFilterApp.cpp:471, 475` |
| `bool loadSettingsFromFile(const File&)` | `SonoStandaloneFilterApp.cpp:496` |
| `bool requestedQuit()` | `SonoStandaloneFilterApp.cpp:820` |

Alternatively, edit those ~15 call sites in `SonoStandaloneFilterApp.cpp` and `SonoStandaloneFilterWindow.h` to target the new class. Nothing else in the shell reaches into the editor.