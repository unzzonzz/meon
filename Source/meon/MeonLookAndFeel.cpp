#include "MeonLookAndFeel.h"

namespace meon
{

MeonLookAndFeel::MeonLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, col::white);
    setColour (juce::DocumentWindow::textColourId, col::ink);

    setColour (juce::Label::textColourId, col::ink);

    setColour (juce::TextEditor::backgroundColourId, col::white);
    setColour (juce::TextEditor::textColourId, col::ink);
    setColour (juce::TextEditor::highlightColourId, col::accent.withAlpha (0.25f));
    setColour (juce::TextEditor::highlightedTextColourId, col::ink);
    setColour (juce::TextEditor::outlineColourId, col::border);
    setColour (juce::TextEditor::focusedOutlineColourId, col::accent);
    setColour (juce::TextEditor::shadowColourId, juce::Colours::transparentBlack);
    setColour (juce::CaretComponent::caretColourId, col::accent);

    setColour (juce::ComboBox::backgroundColourId, col::white);
    setColour (juce::ComboBox::textColourId, col::ink);
    setColour (juce::ComboBox::outlineColourId, col::border);
    setColour (juce::ComboBox::focusedOutlineColourId, col::accent);
    setColour (juce::ComboBox::arrowColourId, col::inkSub);
    setColour (juce::ComboBox::buttonColourId, col::white);

    setColour (juce::PopupMenu::backgroundColourId, juce::Colours::transparentWhite); // 불투명이면 JUCE가 메뉴 창을 사각 불투명 창으로 만든다
    setColour (juce::PopupMenu::textColourId, col::ink);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::panel);
    setColour (juce::PopupMenu::highlightedTextColourId, col::ink);
    setColour (juce::PopupMenu::headerTextColourId, col::inkSub);

    setColour (juce::ScrollBar::thumbColourId, col::border);
    setColour (juce::ScrollBar::trackColourId, juce::Colours::transparentBlack);
    setColour (juce::ScrollBar::backgroundColourId, juce::Colours::transparentBlack);

    setColour (juce::Slider::trackColourId, col::accent);
    setColour (juce::Slider::backgroundColourId, col::meterTrack);
    setColour (juce::Slider::thumbColourId, col::white);

    setColour (juce::TextButton::buttonColourId, col::white);
    setColour (juce::TextButton::buttonOnColourId, col::accent);
    setColour (juce::TextButton::textColourOffId, col::ink);
    setColour (juce::TextButton::textColourOnId, col::white);

    setColour (juce::AlertWindow::backgroundColourId, col::white);
    setColour (juce::AlertWindow::textColourId, col::ink);
    setColour (juce::AlertWindow::outlineColourId, col::border);

    setColour (juce::TooltipWindow::backgroundColourId, col::ink);
    setColour (juce::TooltipWindow::textColourId, col::white);
    setColour (juce::TooltipWindow::outlineColourId, col::ink);
}

juce::Typeface::Ptr MeonLookAndFeel::getTypefaceForFont (const juce::Font& font)
{
    if (auto* cache = FontCache::instance())
    {
        if (font.getTypefaceName() == juce::Font::getDefaultSansSerifFontName()
             || font.getTypefaceName() == "Pretendard")
        {
            if (auto tf = cache->get (font.isBold() ? 700 : 400))
                return tf;
        }
    }
    return juce::LookAndFeel_V4::getTypefaceForFont (font);
}

//==============================================================================
MeonLookAndFeel::FieldMetrics MeonLookAndFeel::metricsForField (int fieldHeight)
{
    // 디자인: 첫 실행 장치 선택 필드 48px = 글자 16px / 안쪽 여백 14px,
    //         설정 필드 40px(플러그인 38px) = 글자 15px / 안쪽 여백 12px. 목록 항목 높이는 임의(40 / 36).
    if (fieldHeight >= 46)
        return { 16.0f, 14, 40 };
    return { 15.0f, 12, 36 };
}

MeonLookAndFeel::FieldMetrics MeonLookAndFeel::metricsForMenu (const juce::PopupMenu::Options& options)
{
    if (auto* target = options.getTargetComponent())
        return metricsForField (target->getHeight());
    return metricsForField (40);
}

void MeonLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                    int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/, juce::ComboBox& box)
{
    const auto m = metricsForField (height);
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
    const bool enabled = box.isEnabled();
    const bool open = box.isPopupActive();

    g.setColour (enabled ? col::white : col::panel);
    g.fillRoundedRectangle (r, (float) metric::radiusButton);

    // 테두리: 기본 #D5D5D5, 목록이 열려 있거나 키보드 포커스면 포인트색, 비활성은 #E0E0E0
    g.setColour (! enabled ? col::cardBorder
                           : (open || box.hasKeyboardFocus (true)) ? col::accent : col::border);
    g.drawRoundedRectangle (r.reduced (0.5f), (float) metric::radiusButton, 1.0f);

    // 화살표: 디자인의 삼각형(border-left/right 5px, border-top 6px = 10×6), 오른쪽 여백은 필드 안쪽 여백과 같게
    juce::Path tri;
    const float right = (float) width - (float) m.pad;
    const float cy = (float) height * 0.5f;
    tri.addTriangle (right - 10.0f, cy - 3.0f, right, cy - 3.0f, right - 5.0f, cy + 3.0f);
    g.setColour (enabled ? col::inkSub : col::disabled);
    g.fillPath (tri);
}

juce::Font MeonLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return Fonts::get (400, metricsForField (box.getHeight()).fontPx);
}

void MeonLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label, juce::Drawable*)
{
    const auto m = metricsForField (box.getHeight());
    label.setBounds (m.pad, 0, box.getWidth() - m.pad * 2 - 10 - 8, box.getHeight());
    label.setFont (getComboBoxFont (box));
    label.setColour (juce::Label::textColourId, box.isEnabled() ? col::ink : col::disabled);
}

juce::PopupMenu::Options MeonLookAndFeel::getOptionsForComboBoxPopupMenu (juce::ComboBox& box, juce::Label& label)
{
    // 목록은 필드에 붙지 않고 popupGap 만큼 띄워서 연다 (위로 열릴 때도 같은 간격)
    return juce::LookAndFeel_V2::getOptionsForComboBoxPopupMenu (box, label)
             .withTargetScreenArea (box.getScreenBounds().expanded (0, metric::popupGap));
}

//==============================================================================
void MeonLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    // 창 자체는 투명(PopupMenu::backgroundColourId 가 투명)이므로 모서리 바깥은 비어 있다
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
    g.setColour (col::white);
    g.fillRoundedRectangle (r, (float) metric::radiusButton);
    g.setColour (col::border);
    g.drawRoundedRectangle (r.reduced (0.5f), (float) metric::radiusButton, 1.0f);
}

void MeonLookAndFeel::drawPopupMenuItemWithOptions (juce::Graphics& g, const juce::Rectangle<int>& area, bool isHighlighted,
                                                    const juce::PopupMenu::Item& item, const juce::PopupMenu::Options& options)
{
    const auto m = metricsForMenu (options);

    if (item.isSeparator)
    {
        g.setColour (col::cardBorder);
        g.fillRect (area.reduced (m.pad, 0).withHeight (1).withY (area.getCentreY()));
        return;
    }

    // 호버/선택 이동: 창 안쪽 여백만큼 들여 쓴 둥근 #F5F5F5 배경
    if (isHighlighted && item.isEnabled)
    {
        g.setColour (col::panel);
        g.fillRoundedRectangle (area.reduced (metric::popupPad, 0).toFloat(), (float) metric::popupItemRadius);
    }

    // 글자 x 위치는 필드의 글자와 같게 (창 가장자리에서 필드 안쪽 여백만큼). 현재 값은 포인트색 + 세미볼드
    g.setColour (! item.isEnabled ? col::disabled : (item.isTicked ? col::accent : col::ink));
    g.setFont (Fonts::get (item.isTicked ? 600 : 400, m.fontPx));
    g.drawText (item.text, area.reduced (m.pad, 0), juce::Justification::centredLeft, true);
}

void MeonLookAndFeel::getIdealPopupMenuItemSizeWithOptions (const juce::String& text, bool isSeparator, int /*standardMenuItemHeight*/,
                                                            int& idealWidth, int& idealHeight, const juce::PopupMenu::Options& options)
{
    if (isSeparator)
    {
        idealWidth = 50;
        idealHeight = 9;
        return;
    }
    const auto m = metricsForMenu (options);
    idealHeight = m.itemH;
    idealWidth = (int) Fonts::get (600, m.fontPx).getStringWidthFloat (text) + m.pad * 2;
}

void MeonLookAndFeel::drawPopupMenuUpDownArrow (juce::Graphics& g, int width, int height, bool isScrollUpArrow)
{
    // 목록이 화면보다 길 때의 스크롤 화살표: 그라디언트 없이 흰 배경 + 삼각형
    g.setColour (col::white);
    g.fillRect (metric::popupPad, 0, width - metric::popupPad * 2, height);

    juce::Path p;
    const float hw = (float) width * 0.5f;
    const float y1 = (float) height * (isScrollUpArrow ? 0.62f : 0.38f);
    const float y2 = (float) height * (isScrollUpArrow ? 0.38f : 0.62f);
    p.addTriangle (hw - 5.0f, y1, hw + 5.0f, y1, hw, y2);
    g.setColour (col::inkSub);
    g.fillPath (p);
}

juce::Font MeonLookAndFeel::getPopupMenuFont()
{
    return Fonts::get (400, 15.0f);
}

//==============================================================================
void MeonLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int width, int height, juce::TextEditor& te)
{
    g.setColour (te.isEnabled() ? te.findColour (juce::TextEditor::backgroundColourId) : col::panel);
    g.fillRoundedRectangle (juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height), (float) metric::radiusButton);
}

void MeonLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int width, int height, juce::TextEditor& te)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
    const bool focused = te.hasKeyboardFocus (true) && ! te.isReadOnly();
    g.setColour (! te.isEnabled() ? col::cardBorder : (focused ? col::accent : col::border));
    g.drawRoundedRectangle (r, (float) metric::radiusButton, 1.0f);
}

juce::CaretComponent* MeonLookAndFeel::createCaretComponent (juce::Component* keyFocusOwner)
{
    auto* caret = new juce::CaretComponent (keyFocusOwner);
    caret->setColour (juce::CaretComponent::caretColourId, col::accent);
    return caret;
}

//==============================================================================
void MeonLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                        const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearHorizontal)
    {
        juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, 0, 0, style, slider);
        return;
    }

    const bool enabled = slider.isEnabled();
    const float cy = (float) y + (float) height * 0.5f;
    const float thumb = (float) (int) slider.getProperties().getWithDefault ("meonThumb", 16);
    const float trackX = (float) x + thumb * 0.5f;
    const float trackW = (float) width - thumb;

    // 트랙 4px
    juce::Rectangle<float> track (trackX, cy - 2.0f, trackW, 4.0f);
    g.setColour (slider.findColour (juce::Slider::backgroundColourId));
    g.fillRoundedRectangle (track, 2.0f);

    // 채움
    const float pos = juce::jlimit (trackX, trackX + trackW, sliderPos);
    g.setColour (enabled ? col::accent : col::border);
    g.fillRoundedRectangle (juce::Rectangle<float> (trackX, cy - 2.0f, pos - trackX, 4.0f), 2.0f);

    // 핸들 16×16 정사각 (모서리 3)
    juce::Rectangle<float> handle (pos - thumb * 0.5f, cy - thumb * 0.5f, thumb, thumb);
    g.setColour (col::white);
    g.fillRoundedRectangle (handle, 3.0f);
    g.setColour (enabled ? col::inkSub : col::border);
    g.drawRoundedRectangle (handle.reduced (0.5f), 3.0f, 1.0f);
}

//==============================================================================
void MeonLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar& /*scrollbar*/, int x, int y, int width, int height,
                                     bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                                     bool /*isMouseOver*/, bool /*isMouseDown*/)
{
    juce::Rectangle<float> thumb;
    if (isScrollbarVertical)
        thumb = juce::Rectangle<float> ((float) x + 2.0f, (float) thumbStartPosition, (float) width - 4.0f, (float) thumbSize);
    else
        thumb = juce::Rectangle<float> ((float) thumbStartPosition, (float) y + 2.0f, (float) thumbSize, (float) height - 4.0f);

    if (thumbSize > 0)
    {
        g.setColour (col::border);
        g.fillRoundedRectangle (thumb, 3.0f);
    }
}

//==============================================================================
juce::Font MeonLookAndFeel::getLabelFont (juce::Label& label)
{
    return label.getFont();
}

juce::Font MeonLookAndFeel::getTextButtonFont (juce::TextButton&, int /*buttonHeight*/)
{
    return Fonts::get (600, 15.0f);
}

} // namespace meon
