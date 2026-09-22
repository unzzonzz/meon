// MEON - 기본 JUCE 컴포넌트(콤보박스, 텍스트 입력, 팝업 메뉴, 슬라이더, 스크롤바)를 디자인대로 그리는 LookAndFeel
#pragma once

#include "MeonTheme.h"

namespace meon
{

class MeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MeonLookAndFeel();

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font& font) override;

    // 콤보박스
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&, juce::Drawable*) override;

    // 팝업 메뉴
    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
                            const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;
    juce::Font getPopupMenuFont() override;
    void getIdealPopupMenuItemSize (const juce::String& text, bool isSeparator, int standardMenuItemHeight,
                                    int& idealWidth, int& idealHeight) override;
    int getPopupMenuBorderSize() override { return 1; }

    // 텍스트 입력
    void fillTextEditorBackground (juce::Graphics&, int width, int height, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int width, int height, juce::TextEditor&) override;
    juce::CaretComponent* createCaretComponent (juce::Component* keyFocusOwner) override;

    // 슬라이더 (볼륨)
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle, juce::Slider&) override;
    int getSliderThumbRadius (juce::Slider& s) override { return (int) s.getProperties().getWithDefault ("meonThumb", 16) / 2; }

    // 스크롤바
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int width, int height,
                        bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                        bool isMouseOver, bool isMouseDown) override;
    int getDefaultScrollbarWidth() override { return 8; }
    bool areScrollbarButtonsVisible() override { return false; }

    // 라벨
    juce::Font getLabelFont (juce::Label&) override;

    // 텍스트 버튼(거의 쓰지 않음)
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
};

} // namespace meon
