// MEON - 파트(악기): 종류, 이름, 아이콘, 선택 칸 6개
// 아이콘 도형은 디자인의 icons/part-*.svg (24 그리드) 그대로. 선 굵기는 크기와 관계없이 1.5px.
#pragma once

#include "MeonTheme.h"

namespace meon
{

enum class Part { None = -1, Vocal, Guitar, Bass, Drums, Keys, Other };

namespace parts
{
    constexpr int count = 6;
    inline Part at (int i) { return (i >= 0 && i < count) ? (Part) i : Part::None; }

    juce::String key (Part);            // "vocal" … (설정 파일에 저장하는 값)
    Part fromKey (const juce::String&);
    juce::String label (Part);          // "보컬" …
    juce::juce_wchar code (Part);       // 사용자 이름 끝에 붙이는 한 글자 (v g b d k o)
    Part fromCode (juce::juce_wchar);

    /** area 가운데에 size×size 로 그린다. colour 는 #1A1A1A 또는 끊긴 멤버용 #AAAAAA */
    void drawIcon (juce::Graphics&, juce::Rectangle<float> area, float size, Part, juce::Colour colour);
}

//==============================================================================
/** 파트 6칸 (하나만 선택). 선택한 칸은 배경·테두리 #FFD700, 아이콘·글자는 #1A1A1A 그대로 */
class PartSelector : public juce::Component
{
public:
    struct Style
    {
        int gap;            // 칸 사이
        float iconSize;
        int iconLabelGap;   // 아이콘과 라벨 사이
        float labelPx;
        int labelWeight;
    };

    explicit PartSelector (Style);
    ~PartSelector() override;

    void setSelected (Part p);
    Part getSelected() const { return selected; }
    std::function<void (Part)> onChange;

    void resized() override;

private:
    class Cell;
    Style style;
    Part selected = Part::None;
    juce::OwnedArray<Cell> cells;
};

} // namespace meon
