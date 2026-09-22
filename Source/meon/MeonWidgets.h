// MEON - 디자인 공통 컴포넌트 (버튼, 배지, 레벨 미터, 볼륨 슬라이더, 체크박스, 경고 상자, 코드 입력, 진행 막대)
#pragma once

#include "MeonTheme.h"

namespace meon
{

//==============================================================================
/** 단축키 배지. 버튼 옆에 항상 표기한다. */
struct Badge
{
    static float width (const juce::String& text, float fontPx, float minWidth = 26.0f, float padX = 7.0f);
    static void draw (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& text,
                      juce::Colour border, juce::Colour bg, juce::Colour ink, float fontPx);
};

//==============================================================================
/** 디자인의 버튼. Primary(채움) / Secondary(흰 바탕 + 테두리) / Dark(#1A1A1A 채움) */
class MeonButton : public juce::Button
{
public:
    enum class Style { Primary, Secondary, Dark };

    explicit MeonButton (const juce::String& label = {}, Style style = Style::Secondary);

    void setStyle (Style s);
    Style getStyle() const { return style; }
    void setLabel (const juce::String& text);
    juce::String getLabel() const { return label; }
    void setSubtitle (const juce::String& text);
    void setBadge (const juce::String& text);
    void setFont (float px, int weight);
    void setSubtitleFont (float px, int weight);
    void setSubtitleInk (juce::Colour c);   // 투명이면 스타일 기본값
    void setBadgeMetrics (float height, float fontPx, float gap);
    void setCornerRadius (float r);
    void setLeftAligned (bool leftAligned, int padding = 14);

    /** 상태별 색을 직접 지정 (예: 끊긴 멤버의 뮤트 버튼). 테두리는 투명이면 안 그린다. */
    void setColourOverride (juce::Colour bg, juce::Colour ink, juce::Colour border, juce::Colour hoverBg);
    void clearColourOverride();

    int getIdealWidth (int horizontalPadding) const;

    void paintButton (juce::Graphics&, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

private:
    Style style;
    juce::String label, subtitle, badge;
    float fontPx = 16.0f;   int fontWeight = 600;
    float subPx = 13.0f;    int subWeight = 400;
    float badgeH = 20.0f, badgePx = 11.0f, badgeGap = 9.0f;
    float radius = (float) metric::radiusButton;
    bool leftAlign = false; int leftPad = 14;
    bool hasOverride = false;
    juce::Colour oBg, oInk, oBorder, oHover;
    juce::Colour subInkOverride = juce::Colours::transparentBlack;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeonButton)
};

//==============================================================================
/** 텍스트 입력 필드 (닉네임, 채팅) */
class MeonTextEditor : public juce::TextEditor
{
public:
    explicit MeonTextEditor (float fontPx = 17.0f, int weight = 400);
    void setPlaceholder (const juce::String& text);
};

//==============================================================================
/** 가로 레벨 미터. dB 스케일은 비선형(-60~0). */
class LevelBar : public juce::Component
{
public:
    LevelBar();

    /** -60~0 dB 를 0~1 위치로 (디자인 눈금: -60/-40/-20/-12/-6/0) */
    static float dbToPosition (float db);

    /** 프레임마다 호출. 상승은 즉시, 하강은 300 ms 감쇠. */
    void push (float db, double dtSeconds);
    void reset();
    float getShownDb() const { return shownDb; }

    void setDisabledLook (bool disabled);
    void setCornerRadius (float r) { radius = r; repaint(); }

    void paint (juce::Graphics&) override;

private:
    float shownDb = -100.0f;
    double clipUntilMs = 0.0;
    bool disabledLook = false;
    float radius = 5.0f;
};

//==============================================================================
/** 볼륨 슬라이더. 값은 게인(0~2). -60 ~ +6 dB. */
class VolumeSlider : public juce::Slider
{
public:
    VolumeSlider();
    static juce::String dbText (double gain);
};

//==============================================================================
/** 체크박스 26×26 (모서리 5). 켜짐은 #FF5A3D 채움 + 흰 체크. */
class MeonCheckbox : public juce::Button
{
public:
    MeonCheckbox();
    void paintButton (juce::Graphics&, bool, bool) override;
};

//==============================================================================
/** 경고 상자. 배경 #FFEDE9 / 테두리 #F5CEC5 / 모서리 8 / 안쪽 여백 18 20 / 아이콘 22 + 간격 14 */
class WarningBox : public juce::Component
{
public:
    WarningBox();

    void setTitle (const juce::String& text);
    void setLines (const juce::StringArray& lines, float lineHeight = 1.6f);
    void setButtonsHeight (int h) { buttonsH = h; }

    /** 텍스트 열의 왼쪽 x (아이콘 오른쪽) */
    int getTextLeft() const { return padX + iconSize + iconGap; }
    /** 본문 아래, 버튼이 놓일 y */
    int getButtonsTop() const;
    /** 내용(버튼 포함)에 맞는 높이 */
    int getPreferredHeight (int width) const;

    void paint (juce::Graphics&) override;

private:
    juce::String title;
    juce::StringArray lines;
    float lineHeightMul = 1.6f;
    int padX = 20, padY = 18, iconSize = 22, iconGap = 14, buttonsH = 0;
    float bodyHeight (int width) const;
};

//==============================================================================
/** 초대 코드 6칸 입력 */
class CodeInput : public juce::Component
{
public:
    CodeInput (int boxW, int boxH, int gap, float fontPx);

    void setErrorState (bool isError);
    bool isComplete() const { return code.length() == 6; }
    juce::String getCode() const { return code; }
    void clear();

    int getPreferredWidth() const { return 6 * boxW + 5 * gap; }
    int getPreferredHeight() const { return boxH; }

    std::function<void()> onChanged;
    std::function<void()> onSubmit;

    void paint (juce::Graphics&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void focusGained (FocusChangeType) override { repaint(); }
    void focusLost (FocusChangeType) override { repaint(); }

    static bool isAllowedChar (juce::juce_wchar c);

private:
    juce::String code;
    bool error = false;
    int boxW, boxH, gap;
    float fontPx;
};

//==============================================================================
/** 첫 실행 진행 막대 (44×4 막대 + "n / total") */
class ProgressSteps : public juce::Component
{
public:
    ProgressSteps (int total, int current, int barWidth = 44);
    void setCurrent (int current);
    int getPreferredWidth() const;
    void paint (juce::Graphics&) override;
private:
    int total, current, barW;
};

//==============================================================================
/** 1px 구분선 */
class Divider : public juce::Component
{
public:
    explicit Divider (juce::Colour c = col::cardBorder) : colour (c) { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics& g) override { g.fillAll (colour); }
private:
    juce::Colour colour;
};

//==============================================================================
/** 단순 텍스트 라벨 (디자인 글꼴 지정) */
class TextLabel : public juce::Component
{
public:
    TextLabel (const juce::String& text = {}, float px = 14.0f, int weight = 400, juce::Colour ink = col::ink,
               juce::Justification just = juce::Justification::centredLeft);
    void setText (const juce::String& t);
    juce::String getText() const { return text; }
    void setFont (float px, int weight);
    void setInk (juce::Colour c);
    void setJustification (juce::Justification j);
    /** 여러 줄 문단 모드 (CSS line-height 배수) */
    void setLineHeight (float mul);
    float getTextWidth() const;
    void paint (juce::Graphics&) override;
private:
    juce::String text; float px; int weight; juce::Colour ink; juce::Justification just; float lineHeightMul = 0.0f;
};

} // namespace meon
