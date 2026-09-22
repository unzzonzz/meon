#include "MeonWidgets.h"

namespace meon
{

//==============================================================================
float Badge::width (const juce::String& text, float fontPx, float minWidth, float padX)
{
    auto font = Fonts::get (600, fontPx);
    return juce::jmax (minWidth, font.getStringWidthFloat (text) + padX * 2.0f);
}

void Badge::draw (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& text,
                  juce::Colour border, juce::Colour bg, juce::Colour ink, float fontPx)
{
    g.setColour (bg);
    g.fillRoundedRectangle (area, 3.0f);
    g.setColour (border);
    g.drawRoundedRectangle (area.reduced (0.5f), 3.0f, 1.0f);
    g.setColour (ink);
    g.setFont (Fonts::get (600, fontPx));
    g.drawText (text, area, juce::Justification::centred, false);
}

//==============================================================================
MeonButton::MeonButton (const juce::String& labelIn, Style styleIn)
    : juce::Button (labelIn), style (styleIn), label (labelIn)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus (false);
}

void MeonButton::setStyle (Style s)                  { style = s; repaint(); }
void MeonButton::setLabel (const juce::String& t)    { label = t; setButtonText (t); repaint(); }
void MeonButton::setSubtitle (const juce::String& t) { subtitle = t; repaint(); }
void MeonButton::setBadge (const juce::String& t)    { badge = t; repaint(); }
void MeonButton::setFont (float px, int weight)      { fontPx = px; fontWeight = weight; repaint(); }
void MeonButton::setSubtitleFont (float px, int w)   { subPx = px; subWeight = w; repaint(); }
void MeonButton::setSubtitleInk (juce::Colour c)     { subInkOverride = c; repaint(); }
void MeonButton::setBadgeMetrics (float h, float px, float gap) { badgeH = h; badgePx = px; badgeGap = gap; repaint(); }
void MeonButton::setCornerRadius (float r)           { radius = r; repaint(); }
void MeonButton::setLeftAligned (bool l, int pad)    { leftAlign = l; leftPad = pad; repaint(); }

void MeonButton::setColourOverride (juce::Colour bg, juce::Colour ink, juce::Colour border, juce::Colour hoverBg)
{
    hasOverride = true; oBg = bg; oInk = ink; oBorder = border; oHover = hoverBg; repaint();
}

void MeonButton::clearColourOverride()
{
    hasOverride = false; repaint();
}

int MeonButton::getIdealWidth (int horizontalPadding) const
{
    auto font = Fonts::get (fontWeight, fontPx);
    float w = font.getStringWidthFloat (label);
    if (badge.isNotEmpty())
        w += badgeGap + Badge::width (badge, badgePx);
    return (int) std::ceil (w) + horizontalPadding * 2;
}

void MeonButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat();
    const bool enabled = isEnabled();
    const bool hot = enabled && (over || down);

    juce::Colour bg, ink, border, badgeBorder, badgeBg, badgeInk, subInk;

    switch (style)
    {
        case Style::Primary:
            bg = ! enabled ? col::disabled : (hot ? col::accentDown : col::accent);
            ink = col::white; border = juce::Colours::transparentBlack;
            badgeBorder = col::white; badgeBg = bg; badgeInk = col::white;
            subInk = col::white;
            break;
        case Style::Dark:
            bg = ! enabled ? col::disabled : col::ink;
            ink = col::white; border = juce::Colours::transparentBlack;
            badgeBorder = col::white; badgeBg = bg; badgeInk = col::white;
            subInk = col::white;
            break;
        case Style::Secondary:
        default:
            bg = hot ? col::panel : col::white;
            ink = enabled ? col::ink : col::disabled;
            border = enabled ? col::border : col::cardBorder;
            badgeBorder = col::cardBorder; badgeBg = col::badgeBg; badgeInk = col::inkSub;
            subInk = col::inkSub;
            break;
    }

    if (hasOverride)
    {
        bg = (hot && ! oHover.isTransparent()) ? oHover : oBg;
        ink = oInk; border = oBorder;
        if (style != Style::Secondary) { badgeBorder = col::white; badgeBg = bg; badgeInk = ink; }
    }

    g.setColour (bg);
    g.fillRoundedRectangle (r, radius);
    if (! border.isTransparent())
    {
        g.setColour (border);
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
    }

    auto font = Fonts::get (fontWeight, fontPx);
    const float labelW = font.getStringWidthFloat (label);
    const float badgeW = badge.isNotEmpty() ? Badge::width (badge, badgePx) : 0.0f;
    const float totalW = labelW + (badge.isNotEmpty() ? badgeGap + badgeW : 0.0f);

    if (subtitle.isEmpty())
    {
        float x = leftAlign ? r.getX() + (float) leftPad : r.getCentreX() - totalW * 0.5f;
        g.setColour (ink);
        g.setFont (font);
        g.drawText (label, juce::Rectangle<float> (x, r.getY(), labelW + 4.0f, r.getHeight()), juce::Justification::centredLeft, false);
        if (badge.isNotEmpty())
            Badge::draw (g, { x + labelW + badgeGap, r.getCentreY() - badgeH * 0.5f, badgeW, badgeH }, badge, badgeBorder, badgeBg, badgeInk, badgePx);
    }
    else
    {
        auto subFont = Fonts::get (subWeight, subPx);
        const float h1 = font.getHeight(), h2 = subFont.getHeight(), gap = 4.0f;
        const float totalH = h1 + gap + h2;
        const float y = r.getCentreY() - totalH * 0.5f;
        g.setColour (ink);
        g.setFont (font);
        g.drawText (label, r.withY (y).withHeight (h1), juce::Justification::centred, false);
        g.setColour (subInkOverride.isTransparent() ? subInk : subInkOverride);
        g.setFont (subFont);
        g.drawText (subtitle, r.withY (y + h1 + gap).withHeight (h2), juce::Justification::centred, false);
    }
}

//==============================================================================
MeonTextEditor::MeonTextEditor (float fontPx, int weight)
{
    setFont (Fonts::get (weight, fontPx));
    setIndents (14, 0);
    setJustification (juce::Justification::centredLeft);
    setSelectAllWhenFocused (false);
    setScrollbarsShown (false);
    setPopupMenuEnabled (false);
    setColour (juce::TextEditor::backgroundColourId, col::white);
    setColour (juce::TextEditor::textColourId, col::ink);
}

void MeonTextEditor::setPlaceholder (const juce::String& text)
{
    setTextToShowWhenEmpty (text, col::disabled);
}

//==============================================================================
LevelBar::LevelBar()
{
    setInterceptsMouseClicks (false, false);
}

float LevelBar::dbToPosition (float db)
{
    // 디자인 눈금(-60, -40, -20, -12, -6, 0)이 균등 간격으로 놓이도록 구간별 선형 보간
    static const float anchorsDb[]  = { -60.0f, -40.0f, -20.0f, -12.0f, -6.0f, 0.0f };
    static const float anchorsPos[] = {   0.0f,   0.2f,   0.4f,   0.6f,  0.8f, 1.0f };
    if (db <= anchorsDb[0]) return 0.0f;
    if (db >= 0.0f) return 1.0f;
    for (int i = 1; i < 6; ++i)
        if (db <= anchorsDb[i])
        {
            const float t = (db - anchorsDb[i - 1]) / (anchorsDb[i] - anchorsDb[i - 1]);
            return anchorsPos[i - 1] + t * (anchorsPos[i] - anchorsPos[i - 1]);
        }
    return 1.0f;
}

void LevelBar::push (float db, double dtSeconds)
{
    const float decayPerSec = 60.0f / 0.3f;   // 감쇠 300 ms 로 -60 dB 까지
    if (db >= shownDb)
        shownDb = db;
    else
        shownDb = juce::jmax (db, shownDb - (float) dtSeconds * decayPerSec);

    if (db >= -0.1f)
        clipUntilMs = juce::Time::getMillisecondCounterHiRes() + 1500.0;

    repaint();
}

void LevelBar::reset()
{
    shownDb = -100.0f;
    clipUntilMs = 0.0;
    repaint();
}

void LevelBar::setDisabledLook (bool disabled)
{
    disabledLook = disabled;
    repaint();
}

void LevelBar::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::meterTrack);
    g.fillRoundedRectangle (r, radius);

    const float pos = disabledLook ? 0.0f : dbToPosition (shownDb);
    if (pos > 0.0f)
    {
        g.setColour (disabledLook ? col::border : col::accent);
        g.fillRoundedRectangle (r.withWidth (juce::jmax (radius * 2.0f, r.getWidth() * pos)), radius);
    }

    if (! disabledLook && juce::Time::getMillisecondCounterHiRes() < clipUntilMs)
    {
        g.setColour (col::ink);
        g.fillRect (r.removeFromRight (r.getWidth() * 0.06f));
    }
}

//==============================================================================
VolumeSlider::VolumeSlider()
{
    setSliderStyle (juce::Slider::LinearHorizontal);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setNormalisableRange (juce::NormalisableRange<double> (0.0, 2.0, 0.0, 0.69));
    setValue (1.0, juce::dontSendNotification);
    setDoubleClickReturnValue (true, 1.0);
    setScrollWheelEnabled (false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus (false);
}

juce::String VolumeSlider::dbText (double gain)
{
    if (gain <= 0.001)
        return "-60 dB";
    double db = juce::Decibels::gainToDecibels (gain);
    db = juce::jlimit (-60.0, 6.0, db);
    return (db > 0.05 ? "+" : "") + juce::String (db, 1) + " dB";
}

//==============================================================================
MeonCheckbox::MeonCheckbox() : juce::Button ("checkbox")
{
    setClickingTogglesState (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus (false);
}

void MeonCheckbox::paintButton (juce::Graphics& g, bool, bool)
{
    auto r = getLocalBounds().toFloat();
    const bool on = getToggleState();
    g.setColour (on ? col::accent : col::white);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (on ? col::accent : col::border);
    g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);

    if (on)
    {
        juce::Path p;
        const float w = r.getWidth();
        p.startNewSubPath (r.getX() + w * 0.26f, r.getY() + w * 0.52f);
        p.lineTo (r.getX() + w * 0.44f, r.getY() + w * 0.70f);
        p.lineTo (r.getX() + w * 0.76f, r.getY() + w * 0.32f);
        g.setColour (col::white);
        g.strokePath (p, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

//==============================================================================
WarningBox::WarningBox()
{
    setInterceptsMouseClicks (false, true);
}

void WarningBox::setTitle (const juce::String& t) { title = t; repaint(); }

void WarningBox::setLines (const juce::StringArray& l, float lineHeight)
{
    lines = l;
    lineHeightMul = lineHeight;
    repaint();
}

float WarningBox::bodyHeight (int width) const
{
    auto bodyFont = Fonts::get (400, 14.0f);
    const float lineH = 14.0f * lineHeightMul;
    const float textW = (float) (width - getTextLeft() - padX);
    float h = 0.0f;
    for (auto& line : lines)
        h += paragraphHeight (line, bodyFont, textW, lineH);
    return h;
}

int WarningBox::getButtonsTop() const
{
    auto titleFont = Fonts::get (600, 16.0f);
    float y = (float) padY + titleFont.getHeight() + 6.0f + bodyHeight (getWidth());
    return (int) std::ceil (y) + 6;
}

int WarningBox::getPreferredHeight (int width) const
{
    auto titleFont = Fonts::get (600, 16.0f);
    float h = (float) padY + titleFont.getHeight() + 6.0f + bodyHeight (width);
    if (buttonsH > 0)
        h += 6.0f + (float) buttonsH;
    return (int) std::ceil (h) + padY;
}

void WarningBox::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::warnBg);
    g.fillRoundedRectangle (r, (float) metric::radiusCard);
    g.setColour (col::warnBorder);
    g.drawRoundedRectangle (r.reduced (0.5f), (float) metric::radiusCard, 1.0f);

    // 아이콘 "!"
    juce::Rectangle<float> icon ((float) padX, (float) padY, (float) iconSize, (float) iconSize);
    g.setColour (col::accent);
    g.fillRoundedRectangle (icon, 4.0f);
    g.setColour (col::white);
    g.setFont (Fonts::get (700, 14.0f));
    g.drawText ("!", icon, juce::Justification::centred, false);

    auto titleFont = Fonts::get (600, 16.0f);
    const float x = (float) getTextLeft();
    const float textW = (float) getWidth() - x - (float) padX;
    float y = (float) padY;
    g.setColour (col::ink);
    g.setFont (titleFont);
    g.drawText (title, juce::Rectangle<float> (x, y, textW, titleFont.getHeight()), juce::Justification::centredLeft, false);
    y += titleFont.getHeight() + 6.0f;

    auto bodyFont = Fonts::get (400, 14.0f);
    const float lineH = 14.0f * lineHeightMul;
    for (auto& line : lines)
    {
        float h = paragraphHeight (line, bodyFont, textW, lineH);
        drawParagraph (g, line, bodyFont, col::inkBody, juce::Rectangle<float> (x, y, textW, h + 2.0f), lineH);
        y += h;
    }
}

//==============================================================================
CodeInput::CodeInput (int w, int h, int g, float px) : boxW (w), boxH (h), gap (g), fontPx (px)
{
    setWantsKeyboardFocus (true);
    setMouseCursor (juce::MouseCursor::IBeamCursor);
}

bool CodeInput::isAllowedChar (juce::juce_wchar c)
{
    return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

void CodeInput::setErrorState (bool isError)
{
    error = isError;
    repaint();
}

void CodeInput::clear()
{
    code.clear();
    error = false;
    repaint();
    if (onChanged) onChanged();
}

void CodeInput::mouseDown (const juce::MouseEvent&)
{
    grabKeyboardFocus();
}

bool CodeInput::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::backspaceKey || k == juce::KeyPress::deleteKey)
    {
        if (code.isNotEmpty())
            code = code.dropLastCharacters (1);
        error = false;
        repaint();
        if (onChanged) onChanged();
        return true;
    }

    if (k == juce::KeyPress::returnKey)
    {
        if (onSubmit) onSubmit();
        return true;
    }

    if (k.getModifiers().isCommandDown())
    {
        const auto ch = juce::CharacterFunctions::toUpperCase (k.getTextCharacter());
        if (ch == 'V')
        {
            auto clip = juce::SystemClipboard::getTextFromClipboard().toUpperCase();
            for (int i = 0; i < clip.length() && code.length() < 6; ++i)
                if (isAllowedChar (clip[i]))
                    code += juce::String::charToString (clip[i]);
            error = false;
            repaint();
            if (onChanged) onChanged();
            return true;
        }
        return false;
    }

    const auto ch = k.getTextCharacter();
    if (ch != 0 && code.length() < 6)
    {
        const auto up = juce::CharacterFunctions::toUpperCase (ch);
        if (isAllowedChar (up))
        {
            code += juce::String::charToString (up);
            error = false;
            repaint();
            if (onChanged) onChanged();
            return true;
        }
    }
    return false;
}

void CodeInput::paint (juce::Graphics& g)
{
    const bool focused = hasKeyboardFocus (true);
    auto font = Fonts::get (700, fontPx);
    for (int i = 0; i < 6; ++i)
    {
        juce::Rectangle<float> box ((float) (i * (boxW + gap)), 0.0f, (float) boxW, (float) boxH);
        const bool cursor = ! error && focused && i == code.length();
        g.setColour (col::white);
        g.fillRoundedRectangle (box, (float) metric::radiusButton);
        g.setColour (error ? col::accent : (cursor ? col::accent : col::border));
        g.drawRoundedRectangle (box.reduced (0.5f), (float) metric::radiusButton, 1.0f);

        if (i < code.length())
        {
            g.setColour (col::ink);
            g.setFont (font);
            g.drawText (code.substring (i, i + 1), box, juce::Justification::centred, false);
        }
        else if (cursor)
        {
            g.setColour (col::accent);
            g.fillRect (juce::Rectangle<float> (box.getCentreX() - 0.5f, box.getY() + box.getHeight() * 0.28f, 1.5f, box.getHeight() * 0.44f));
        }
    }
}

//==============================================================================
ProgressSteps::ProgressSteps (int t, int c, int barWidth) : total (t), current (c), barW (barWidth)
{
    setInterceptsMouseClicks (false, false);
}

void ProgressSteps::setCurrent (int c)
{
    current = c;
    repaint();
}

int ProgressSteps::getPreferredWidth() const
{
    auto font = Fonts::get (500, 13.0f);
    return total * barW + (total - 1) * 6 + 14 + (int) font.getStringWidthFloat (juce::String (current) + " / " + juce::String (total)) + 2;
}

void ProgressSteps::paint (juce::Graphics& g)
{
    const float cy = (float) getHeight() * 0.5f;
    float x = 0.0f;
    for (int i = 0; i < total; ++i)
    {
        g.setColour (i < current ? col::accent : col::progressOff);
        g.fillRoundedRectangle (juce::Rectangle<float> (x, cy - 2.0f, (float) barW, 4.0f), 2.0f);
        x += (float) barW + 6.0f;
    }
    x += 8.0f;
    g.setColour (col::inkSub);
    g.setFont (Fonts::get (500, 13.0f));
    g.drawText (juce::String (current) + " / " + juce::String (total),
                juce::Rectangle<float> (x, 0.0f, (float) getWidth() - x, (float) getHeight()), juce::Justification::centredLeft, false);
}

//==============================================================================
TextLabel::TextLabel (const juce::String& t, float p, int w, juce::Colour c, juce::Justification j)
    : text (t), px (p), weight (w), ink (c), just (j)
{
    setInterceptsMouseClicks (false, false);
}

void TextLabel::setText (const juce::String& t)  { if (text != t) { text = t; repaint(); } }
void TextLabel::setFont (float p, int w)          { px = p; weight = w; repaint(); }
void TextLabel::setInk (juce::Colour c)           { ink = c; repaint(); }
void TextLabel::setJustification (juce::Justification j) { just = j; repaint(); }
void TextLabel::setLineHeight (float mul)         { lineHeightMul = mul; repaint(); }

float TextLabel::getTextWidth() const
{
    return Fonts::get (weight, px).getStringWidthFloat (text);
}

void TextLabel::paint (juce::Graphics& g)
{
    auto font = Fonts::get (weight, px);
    if (lineHeightMul > 0.0f)
    {
        drawParagraph (g, text, font, ink, getLocalBounds().toFloat(), px * lineHeightMul, just);
        return;
    }
    g.setColour (ink);
    g.setFont (font);
    g.drawText (text, getLocalBounds(), just, false);
}

} // namespace meon
