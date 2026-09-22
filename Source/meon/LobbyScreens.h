// MEON - 홈 / 방 만들기 / 코드로 입장
#pragma once

#include "MeonEditor.h"
#include "MeonWidgets.h"

namespace meon
{

class HomeScreen : public ScreenBase,
                   private MeonSession::Listener,
                   private juce::Timer
{
public:
    explicit HomeScreen (MeonEditor&);
    ~HomeScreen() override;
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    MeonEditor& editor;
    const bool plugin;
    MeonButton settingsButton, createButton, joinButton;
    juce::Rectangle<int> centre, footer;

    void updateState();
    void sessionStateChanged() override { updateState(); }
    void timerCallback() override { repaint (footer); }
};

//==============================================================================
class CreateRoomScreen : public ScreenBase,
                         private MeonSession::Listener
{
public:
    explicit CreateRoomScreen (MeonEditor&);
    ~CreateRoomScreen() override;
    bool handleShortcut (const juce::KeyPress&) override;
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    MeonEditor& editor;
    const bool plugin;
    MeonButton homeButton, copyButton, enterButton;
    juce::String note;
    bool failed = false;
    juce::Rectangle<int> card, titleArea;

    void copyCode();
    void updateState();
    void sessionStateChanged() override { updateState(); }
    void roomJoined() override { updateState(); }
    void joinFailed (MeonSession::JoinFailure) override;
};

//==============================================================================
class JoinRoomScreen : public ScreenBase,
                       private MeonSession::Listener
{
public:
    explicit JoinRoomScreen (MeonEditor&);
    ~JoinRoomScreen() override;
    bool handleShortcut (const juce::KeyPress&) override;
    void resized() override;
    void paint (juce::Graphics&) override;
    void parentHierarchyChanged() override;

private:
    enum class State { Idle, Checking, Wrong, Full, Error };
    MeonEditor& editor;
    const bool plugin;
    MeonButton homeButton, enterButton, newRoomButton;
    CodeInput code;
    State state = State::Idle;
    juce::Rectangle<int> messageArea;

    void submit();
    void updateState();
    void sessionStateChanged() override { updateState(); }
    void joinFailed (MeonSession::JoinFailure) override;
};

} // namespace meon
