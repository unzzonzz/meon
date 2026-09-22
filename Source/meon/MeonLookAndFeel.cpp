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

    setColour (juce::PopupMenu::backgroundColourId, col::white);
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
void MeonLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                    int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
    const bool enabled = box.isEnabled();

    g.setColour (enabled ? col::white : col::panel);
    g.fillRoundedRectangle (r, (float) metric::radiusButton);

    g.setColour (enabled ? (box.hasKeyboardFocus (true) ? col::accent : col::border) : col::cardBorder);
    g.drawRoundedRectangle (r.reduced (0.5f), (float) metric::radiusButton, 1.0f);

    // 화살표: 아래 방향 작은 삼각형 [임의: 10×5, 오른쪽 여백 16]
    juce::Path tri;
    const float cx = (float) width - 16.0f - 5.0f;
    const float cy = (float) height * 0.5f;
    tri.addTriangle (cx - 5.0f, cy - 2.5f, cx + 5.0f, cy - 2.5f, cx, cy + 2.5f);
    g.setColour (enabled ? col::inkSub : col::disabled);
    g.fillPath (tri);
}

juce::Font MeonLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return Fonts::get (400, 16.0f);
}

void MeonLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label, juce::Drawable*)
{
    label.setBounds (14, 0, box.getWidth() - 14 - 36, box.getHeight());
    label.setFont (getComboBoxFont (box));
    label.setColour (juce::Label::textColourId, box.isEnabled() ? col::ink : col::disabled);
}

//==============================================================================
void MeonLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
    g.setColour (col::white);
    g.fillRoundedRectangle (r, (float) metric::radiusButton);
    g.setColour (col::border);
    g.drawRoundedRectangle (r.reduced (0.5f), (float) metric::radiusButton, 1.0f);
}

void MeonLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                         bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool /*hasSubMenu*/,
                                         const juce::String& text, const juce::String& /*shortcutKeyText*/,
                                         const juce::Drawable* /*icon*/, const juce::Colour* /*textColour*/)
{
    if (isSeparator)
    {
        g.setColour (col::cardBorder);
        g.fillRect (area.reduced (8, 0).withHeight (1).withY (area.getCentreY()));
        return;
    }

    auto r = area.reduced (4, 0);
    if (isHighlighted && isActive)
    {
        g.setColour (col::panel);
        g.fillRoundedRectangle (r.toFloat(), 4.0f);
    }

    g.setColour (! isActive ? col::disabled : (isTicked ? col::accent : col::ink));
    g.setFont (Fonts::get (isTicked ? 600 : 400, 15.0f));
    g.drawText (text, r.reduced (10, 0), juce::Justification::centredLeft, true);
}

juce::Font MeonLookAndFeel::getPopupMenuFont()
{
    return Fonts::get (400, 15.0f);
}

void MeonLookAndFeel::getIdealPopupMenuItemSize (const juce::String& text, bool isSeparator, int /*standardMenuItemHeight*/,
                                                 int& idealWidth, int& idealHeight)
{
    if (isSeparator)
    {
        idealWidth = 50;
        idealHeight = 9;
        return;
    }
    auto font = getPopupMenuFont();
    idealHeight = 36;
    idealWidth = (int) font.getStringWidthFloat (text) + 40;
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
