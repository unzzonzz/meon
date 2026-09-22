// MEON - 합주 화면 (내 영역, 멤버 카드 2×2, 채팅, 나가기 확인)
#pragma once

#include "MeonEditor.h"
#include "MeonWidgets.h"

namespace meon
{

class JamScreen : public ScreenBase,
                  private MeonSession::Listener,
                  private juce::Timer
{
public:
    explicit JamScreen (MeonEditor&);
    ~JamScreen() override;

    bool handleShortcut (const juce::KeyPress&) override;
    bool confirmLeaveForQuit() override;
    void resized() override;
    void paint (juce::Graphics&) override;

    MeonEditor& getEditor() { return editor; }
    bool isPlugin() const { return plugin; }
    void copyCode();
    void toggleChat();
    void requestLeave();
    void cancelLeave();
    void doLeave();
    void sendChatText (const juce::String& text);

    static juce::String koreanTime (const juce::Time& t);

private:
    class MyPanel;
    class MemberCard;
    class EmptySlot;
    class AlonePanel;
    class ChatPanel;
    class ChatRail;
    class LeaveDialog;

    MeonEditor& editor;
    const bool plugin;
    MeonButton copyButton, settingsButton, leaveButton;
    std::unique_ptr<MyPanel> me;
    std::unique_ptr<AlonePanel> alone;
    juce::OwnedArray<MemberCard> cards;
    juce::OwnedArray<EmptySlot> empties;
    std::unique_ptr<ChatPanel> chat;
    std::unique_ptr<ChatRail> rail;
    std::unique_ptr<LeaveDialog> leaveDialog;
    bool chatOpen = true;
    bool quitAfterLeave = false;
    double lastTickMs = 0.0;
    juce::Rectangle<int> topBar;

    void rebuildCards();
    void updateCards();
    void setChatOpen (bool open);
    bool effectiveChatOpen() const;
    void showLeaveDialog (bool quit);

    void membersChanged() override { rebuildCards(); }
    void memberStatsChanged() override { updateCards(); }
    void chatChanged() override;
    void sessionStateChanged() override { repaint (topBar); }
    void timerCallback() override;
};

} // namespace meon
