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

    // 콤보박스 (디자인의 select 필드: 높이 48 → 글자 16/여백 14, 높이 40·38 → 글자 15/여백 12)
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&, juce::Drawable*) override;
    juce::PopupMenu::Options getOptionsForComboBoxPopupMenu (juce::ComboBox&, juce::Label&) override;

    // 드롭다운 목록 (팝업 메뉴). 배경색을 투명으로 두어 창이 불투명 사각형으로 만들어지지 않게 하고,
    // OS 그림자 없이 둥근 흰 배경 + 1px 테두리만 직접 그린다.
    int getMenuWindowFlags() override { return 0; }
    int getPopupMenuBorderSize() override { return metric::popupPad; }
    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    void drawPopupMenuItemWithOptions (juce::Graphics&, const juce::Rectangle<int>& area, bool isHighlighted,
                                       const juce::PopupMenu::Item&, const juce::PopupMenu::Options&) override;
    void getIdealPopupMenuItemSizeWithOptions (const juce::String& text, bool isSeparator, int standardMenuItemHeight,
                                               int& idealWidth, int& idealHeight, const juce::PopupMenu::Options&) override;
    void drawPopupMenuUpDownArrow (juce::Graphics&, int width, int height, bool isScrollUpArrow) override;
    juce::Font getPopupMenuFont() override;

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

private:
    struct FieldMetrics { float fontPx; int pad; int itemH; };
    static FieldMetrics metricsForField (int fieldHeight);
    static FieldMetrics metricsForMenu (const juce::PopupMenu::Options&);
};

} // namespace meon
