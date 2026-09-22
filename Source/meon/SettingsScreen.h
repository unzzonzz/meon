// MEON - 설정 (오디오 장치, 닉네임, 로그 폴더 열기, 앱 정보)
#pragma once

#include "MeonEditor.h"
#include "MeonWidgets.h"

namespace meon
{

class SettingsScreen : public ScreenBase,
                       private juce::Timer,
                       private juce::ChangeListener
{
public:
    explicit SettingsScreen (MeonEditor&);
    ~SettingsScreen() override;

    bool handleShortcut (const juce::KeyPress&) override;
    void resized() override;
    void paint (juce::Graphics&) override;

    /** 개발용: --screen=settings:input|output|buffer 로 드롭다운을 열어 둔 상태를 캡처한다. */
    void showPopupForTest (const juce::String& which);

private:
    class LinkLabel : public juce::Button
    {
    public:
        LinkLabel (const juce::String& text, const juce::String& url, float px);
        void paintButton (juce::Graphics&, bool, bool) override;
        int getIdealWidth() const;
    private:
        juce::String text, url;
        float px;
    };

    class Content : public juce::Component
    {
    public:
        explicit Content (SettingsScreen& owner);
        void layout (int width);
        void paint (juce::Graphics&) override;
        void refreshDevices();
        void pushLevel (float db, double dt) { meter.push (db, dt); repaint (levelTextArea); }

        SettingsScreen& owner;
        const bool plugin;
        juce::ComboBox inCombo, outCombo, bufCombo;
        LevelBar meter;
        MeonTextEditor nickInput;
        MeonButton saveButton, logButton;
        LinkLabel licenseLink, sourceLink;
        juce::Rectangle<int> levelTextArea;
        struct Row { juce::String label; juce::Rectangle<int> area; };
        std::vector<std::pair<juce::String, juce::Rectangle<int>>> sectionTitles, rowLabels, infoRows, texts, subTexts;
        std::vector<juce::Rectangle<int>> dividers;
        juce::Rectangle<int> audioPanel;
        bool updating = false;
        juce::Array<int> bufferChoices;
        void applyDevices();
        void applyBuffer();
        void saveNickname();
    };

    MeonEditor& editor;
    const bool plugin;
    MeonButton closeButton;
    juce::Viewport viewport;
    Content content;
    double lastTickMs = 0.0;

    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
};

} // namespace meon
