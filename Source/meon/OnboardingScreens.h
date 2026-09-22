// MEON - 첫 실행 화면 (닉네임 → 오디오 장치 → 헤드폰 확인 → 입력 레벨). 플러그인은 닉네임 → 헤드폰만.
#pragma once

#include "MeonEditor.h"
#include "MeonWidgets.h"

namespace meon
{

/** 첫 실행 페이지 공통 틀: 진행 막대, 제목, 설명, 하단 버튼 */
class OnboardingPage : public ScreenBase
{
public:
    OnboardingPage (MeonEditor& editor, int step, int totalSteps, const juce::String& title, const juce::String& desc);

protected:
    MeonEditor& editor;
    const bool plugin;

    struct Metrics
    {
        int padTop, padSide, padBottom;
        int barW;
        float titlePx; int titleTop, titleGap;
        float descPx;
        int footerPad, btnH, backPadX, nextPadX;
        float btnPx;
    };
    Metrics m;

    ProgressSteps steps;
    TextLabel titleLabel, descLabel, footerNote, footerHint;
    Divider footerLine;
    MeonButton backButton, nextButton;

    /** 설명 아래 ~ 하단 구분선 위 영역 */
    virtual void layoutBody (juce::Rectangle<int> body) = 0;
    void resized() override;
    void showBack (bool show) { backButton.setVisible (show); }
};

//==============================================================================
class NicknameScreen : public OnboardingPage
{
public:
    explicit NicknameScreen (MeonEditor&);
    void layoutBody (juce::Rectangle<int> body) override;
    void parentHierarchyChanged() override;
private:
    TextLabel fieldLabel, fieldHint;
    MeonTextEditor input;
    void validate();
    void submit();
};

//==============================================================================
class AudioDeviceScreen : public OnboardingPage,
                          private juce::ChangeListener
{
public:
    explicit AudioDeviceScreen (MeonEditor&);
    ~AudioDeviceScreen() override;
    void layoutBody (juce::Rectangle<int> body) override;

private:
    WarningBox warning;
    MeonButton rescanButton, asioGuideButton;
    TextLabel inLabel, outLabel, bufLabel, bufHint;
    juce::ComboBox inCombo, outCombo;
    juce::OwnedArray<MeonButton> bufferButtons;
    const int bufferSizes[4] = { 64, 128, 256, 512 };
    bool noDevices = false;
    bool updating = false;

    juce::AudioIODeviceType* deviceType() const;
    void refreshDevices();
    void applySelection();
    void selectBuffer (int size);
    void updateBufferButtons();
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
};

//==============================================================================
class HeadphoneScreen : public OnboardingPage,
                        private juce::Timer
{
public:
    explicit HeadphoneScreen (MeonEditor&);
    void layoutBody (juce::Rectangle<int> body) override;

private:
    class CheckRow : public juce::Button
    {
    public:
        CheckRow (bool plugin);
        void paintButton (juce::Graphics&, bool, bool) override;
    private:
        bool plugin;
    };

    WarningBox warning;
    MeonButton changeOutputButton;
    CheckRow row;
    bool speakerWarning = false;
    juce::String outputName;

    void updateState();
    void checkOutputDevice();
    void timerCallback() override { checkOutputDevice(); }
};

//==============================================================================
class InputLevelScreen : public OnboardingPage,
                         private juce::Timer
{
public:
    explicit InputLevelScreen (MeonEditor&);
    void layoutBody (juce::Rectangle<int> body) override;
    void paint (juce::Graphics&) override;

private:
    TextLabel meterLabel, meterValue;
    LevelBar meter;
    WarningBox warning;
    MeonButton changeChannelButton, changeDeviceButton;
    bool noSignal = false;
    double lastSignalMs = 0.0;
    double lastTickMs = 0.0;
    float peakDb = -100.0f;
    double peakAtMs = 0.0;
    juce::Rectangle<int> meterBlock, okRow;

    void timerCallback() override;
    void updateLabels();
    void showChannelMenu();
    juce::String channelText() const;
};

} // namespace meon
