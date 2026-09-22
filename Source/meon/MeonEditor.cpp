#include "MeonEditor.h"
#include "OnboardingScreens.h"
#include "LobbyScreens.h"
#include "JamScreen.h"
#include "SettingsScreen.h"

namespace meon
{

void MeonEditor::Fader::fade (juce::Component* c, float from, float to, int ms, std::function<void()> onDone)
{
    cancel (c);
    if (c == nullptr)
        return;
    c->setAlpha (from);
    items.push_back ({ c, from, to, juce::Time::getMillisecondCounterHiRes(), ms, std::move (onDone) });
    startTimerHz (60);
}

void MeonEditor::Fader::cancel (juce::Component* c)
{
    items.erase (std::remove_if (items.begin(), items.end(), [c] (const Item& i) { return i.comp == nullptr || i.comp.getComponent() == c; }), items.end());
    if (items.empty())
        stopTimer();
}

void MeonEditor::Fader::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    std::vector<std::function<void()>> finished;
    for (auto it = items.begin(); it != items.end();)
    {
        if (it->comp == nullptr) { it = items.erase (it); continue; }
        const double t = juce::jlimit (0.0, 1.0, (now - it->startMs) / (double) juce::jmax (1, it->ms));
        it->comp->setAlpha (it->from + (it->to - it->from) * (float) t);
        if (t >= 1.0)
        {
            if (it->onDone) finished.push_back (std::move (it->onDone));
            it = items.erase (it);
        }
        else ++it;
    }
    if (items.empty())
        stopTimer();
    for (auto& f : finished)
        f();
}

MeonEditor::MeonEditor (SonobusAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p),
      pluginMode (! juce::JUCEApplicationBase::isStandaloneApp())
{
    setLookAndFeel (&lookAndFeel);
    juce::LookAndFeel::setDefaultLookAndFeel (&lookAndFeel);

    settings = std::make_unique<MeonSettings>();
    session = std::make_unique<MeonSession> (processor, *settings, pluginMode);
    session->setAudioInfoProvider ([this] { return collectAudioInfo(); });
    session->addListener (this);

    setOpaque (true);
    setWantsKeyboardFocus (true);

    // 크기: 독립 앱 기본 1280×800 (최소 1024×680), 플러그인 기본 900×600 (최소 760×520)
    {
        const int minW = pluginMode ? 760 : 1024, minH = pluginMode ? 520 : 680;
        const int defW = pluginMode ? 900 : 1280, defH = pluginMode ? 600 : 800;
        auto last = processor.getLastPluginBounds();
        const int w = last.getWidth() >= minW ? last.getWidth() : defW;
        const int h = last.getHeight() >= minH ? last.getHeight() : defH;
        setSize (w, h);
        setResizable (true, pluginMode);
        setResizeLimits (minW, minH, pluginMode ? 4000 : 10000, pluginMode ? 4000 : 10000);
    }

    go (initialScreen(), false);
    session->start();
}

MeonEditor::~MeonEditor()
{
    session->removeListener (this);
    settingsOverlay = nullptr;
    settingsFadingOut = nullptr;
    screen = nullptr;
    fadingOut = nullptr;
    session = nullptr;
    settings = nullptr;
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

//==============================================================================
MeonEditor::Screen MeonEditor::initialScreen() const
{
    if (session->isInRoom())
        return Screen::Jam;   // 플러그인 창을 다시 연 경우
    if (settings->getNickname().trim().isEmpty())
        return Screen::Nickname;
    if (! settings->isOnboardingDone (pluginMode))
        return pluginMode ? Screen::Headphone : Screen::Audio;
    return Screen::Home;
}

std::unique_ptr<juce::Component> MeonEditor::createScreen (Screen s)
{
    switch (s)
    {
        case Screen::Nickname:  return std::make_unique<NicknameScreen> (*this);
        case Screen::Audio:     return std::make_unique<AudioDeviceScreen> (*this);
        case Screen::Headphone: return std::make_unique<HeadphoneScreen> (*this);
        case Screen::Level:     return std::make_unique<InputLevelScreen> (*this);
        case Screen::Create:    return std::make_unique<CreateRoomScreen> (*this);
        case Screen::Join:      return std::make_unique<JoinRoomScreen> (*this);
        case Screen::Jam:       return std::make_unique<JamScreen> (*this);
        case Screen::Home:
        default:                return std::make_unique<HomeScreen> (*this);
    }
}

void MeonEditor::go (Screen s, bool fade)
{
    if (pluginMode && (s == Screen::Audio || s == Screen::Level))
        s = Screen::Home;
    current = s;
    showComponent (createScreen (s), fade);
    if (s != Screen::Jam)
        session->setServerPingInterval (s == Screen::Home ? 5000 : 15000);
}

void MeonEditor::showComponent (std::unique_ptr<juce::Component> c, bool fade)
{
    if (fadingOut != nullptr)
    {
        fader.cancel (fadingOut.get());
        fadingOut = nullptr;
    }

    auto old = std::move (screen);
    screen = std::move (c);
    screen->setBounds (getLocalBounds());
    addAndMakeVisible (*screen);
    if (settingsOverlay != nullptr)
        settingsOverlay->toFront (false);

    if (old != nullptr)
    {
        if (fade)
        {
            fadingOut = std::move (old);
            juce::Component::SafePointer<MeonEditor> safe (this);
            fader.fade (fadingOut.get(), 1.0f, 0.0f, metric::fadeMs, [safe] { if (safe != nullptr) safe->finishFade(); });
        }
        else
        {
            old = nullptr;
        }
    }

    if (fade)
        fader.fade (screen.get(), 0.0f, 1.0f, metric::fadeMs);
    grabKeyboardFocus();
}

void MeonEditor::finishFade()
{
    fadingOut = nullptr;
}

void MeonEditor::openSettings()
{
    if (settingsOverlay != nullptr)
        return;
    settingsOverlay = std::make_unique<SettingsScreen> (*this);
    settingsOverlay->setBounds (getLocalBounds());
    addAndMakeVisible (*settingsOverlay);
    settingsOverlay->toFront (false);
    fader.fade (settingsOverlay.get(), 0.0f, 1.0f, metric::fadeMs);
    grabKeyboardFocus();
}

void MeonEditor::closeSettings()
{
    if (settingsOverlay == nullptr)
        return;
    if (settingsFadingOut != nullptr)
    {
        fader.cancel (settingsFadingOut.get());
        settingsFadingOut = nullptr;
    }
    settingsFadingOut = std::move (settingsOverlay);
    juce::Component::SafePointer<MeonEditor> safe (this);
    fader.fade (settingsFadingOut.get(), 1.0f, 0.0f, metric::fadeMs, [safe] { if (safe != nullptr) safe->settingsFadingOut = nullptr; });
    saveAll();
    grabKeyboardFocus();
}

//==============================================================================
bool MeonEditor::requestedQuit()
{
    if (session->isInRoom())
    {
        if (settingsOverlay != nullptr)
            closeSettings();
        if (auto* s = dynamic_cast<ScreenBase*> (screen.get()))
            if (s->confirmLeaveForQuit())
                return false;
        session->leaveRoom();
    }
    saveAll();
    return true;
}

void MeonEditor::saveAll()
{
    settings->save();
    if (saveSettingsIfNeeded)
        saveSettingsIfNeeded();
}

void MeonEditor::copyRoomCodeToClipboard()
{
    auto code = session->getDisplayRoomCode();
    if (code.isNotEmpty())
        juce::SystemClipboard::copyTextToClipboard (code);
}

juce::String MeonEditor::cmdKeyLabel (const juce::String& key)
{
#if JUCE_MAC
    return TXT ("⌘ ") + key;
#else
    return "Ctrl " + key;
#endif
}

juce::String MeonEditor::getHostDescription() const
{
    if (! pluginMode)
        return "MEON";
    juce::PluginHostType host;
    juce::String desc (host.getHostDescription());
    if (desc.isEmpty() || desc == "Unknown")
        desc = TXT ("DAW");
    return desc;
}

juce::String MeonEditor::getVersionText() const
{
    juce::String v = "MEON " + juce::String (MEON_BUILD_VERSION);
    if (pluginMode)
        v += " (AU / VST3)";
    else
        v += " (build " + juce::String (MEON_BUILD_NUMBER) + ")";
    return v;
}

MeonSessionLog::AudioInfo MeonEditor::collectAudioInfo() const
{
    MeonSessionLog::AudioInfo info;
    if (pluginMode)
    {
        info.deviceType = "host";
        info.host = getHostDescription();
        info.sampleRate = processor.getSampleRate();
        info.bufferSize = processor.getBlockSize();
    }
    else if (auto* dm = deviceManager())
    {
        auto setup = dm->getAudioDeviceSetup();
        info.deviceType = dm->getCurrentAudioDeviceType();
        info.inputDevice = setup.inputDeviceName;
        info.outputDevice = setup.outputDeviceName;
        info.sampleRate = setup.sampleRate;
        info.bufferSize = setup.bufferSize;
        if (auto* dev = dm->getCurrentAudioDevice())
        {
            info.sampleRate = dev->getCurrentSampleRate();
            info.bufferSize = dev->getCurrentBufferSizeSamples();
        }
    }
    return info;
}

//==============================================================================
void MeonEditor::applyTestOptions (const juce::String& nickname, const juce::String& screenName,
                                   bool autoCreateRoom, const juce::String& autoJoinCode, bool autoEnterJam)
{
    if (nickname.isNotEmpty())
    {
        settings->setNickname (nickname);
        settings->setOnboardingDone (pluginMode, true);
        session->applyNicknameChange();   // 이미 연결 중이면 새 이름으로 다시 연결
    }

    autoCreate = autoCreateRoom;
    autoJoin = autoJoinCode.trim().toUpperCase();
    autoEnter = autoEnterJam;

    if (screenName.isNotEmpty())
    {
        const auto n = screenName.toLowerCase();
        if (n == "settings")       { go (Screen::Home, false); openSettings(); }
        else if (n == "nick" || n == "nickname") go (Screen::Nickname, false);
        else if (n == "audio")     go (Screen::Audio, false);
        else if (n == "hp" || n == "headphone")  go (Screen::Headphone, false);
        else if (n == "level")     go (Screen::Level, false);
        else if (n == "create")    go (Screen::Create, false);
        else if (n == "join")      go (Screen::Join, false);
        else if (n == "jam")       go (Screen::Jam, false);
        else                       go (Screen::Home, false);
    }
    runAutoRoomAction();
}

void MeonEditor::runAutoRoomAction()
{
    if (! session->isServerConnected() || session->getRoomState() != MeonSession::RoomState::None)
        return;
    if (autoCreate)
    {
        autoCreate = false;
        session->createRoom();
        go (Screen::Create);
    }
    else if (autoJoin.isNotEmpty())
    {
        const auto code = autoJoin;
        autoJoin.clear();
        go (Screen::Join);
        session->joinRoom (code);
    }
}

void MeonEditor::sessionStateChanged()
{
    runAutoRoomAction();
}

void MeonEditor::roomJoined()
{
    if (current == Screen::Join || (autoEnter && current == Screen::Create))
        go (Screen::Jam);
}

void MeonEditor::roomLeft()
{
    if (current == Screen::Jam)
        go (Screen::Home);
}

//==============================================================================
void MeonEditor::paint (juce::Graphics& g)
{
    g.fillAll (col::white);
}

void MeonEditor::resized()
{
    if (screen != nullptr)
        screen->setBounds (getLocalBounds());
    if (fadingOut != nullptr)
        fadingOut->setBounds (getLocalBounds());
    if (settingsOverlay != nullptr)
        settingsOverlay->setBounds (getLocalBounds());
    if (settingsFadingOut != nullptr)
        settingsFadingOut->setBounds (getLocalBounds());
    processor.setLastPluginBounds (getLocalBounds());
}

void MeonEditor::mouseDown (const juce::MouseEvent&)
{
    grabKeyboardFocus();
}

bool MeonEditor::keyPressed (const juce::KeyPress& k)
{
    // 설정: ⌘ , (Windows: Ctrl ,)
    if (k.getModifiers().isCommandDown() && k.getTextCharacter() == ',')
    {
        if (settingsOverlay != nullptr) closeSettings();
        else openSettings();
        return true;
    }

    // 텍스트 입력 중에는 글자 단축키를 먹지 않는다 (Esc 는 입력 컴포넌트가 onEscapeKey 로 전달)
    if (auto* focused = juce::Component::getCurrentlyFocusedComponent())
        if (dynamic_cast<juce::TextEditor*> (focused) != nullptr && ! k.getModifiers().isCommandDown())
            return false;

    if (settingsOverlay != nullptr)
    {
        if (auto* s = dynamic_cast<ScreenBase*> (settingsOverlay.get()))
            return s->handleShortcut (k);
        return false;
    }
    if (auto* s = dynamic_cast<ScreenBase*> (screen.get()))
        return s->handleShortcut (k);
    return false;
}

} // namespace meon
