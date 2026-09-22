#include "MeonTheme.h"

namespace meon
{

static FontCache* gFontCache = nullptr;

FontCache::FontCache()
{
    regular  = juce::Typeface::createSystemTypefaceFor (BinaryData::PretendardRegular_otf,  BinaryData::PretendardRegular_otfSize);
    medium   = juce::Typeface::createSystemTypefaceFor (BinaryData::PretendardMedium_otf,   BinaryData::PretendardMedium_otfSize);
    semiBold = juce::Typeface::createSystemTypefaceFor (BinaryData::PretendardSemiBold_otf, BinaryData::PretendardSemiBold_otfSize);
    bold     = juce::Typeface::createSystemTypefaceFor (BinaryData::PretendardBold_otf,     BinaryData::PretendardBold_otfSize);
    gFontCache = this;
}

FontCache::~FontCache()
{
    if (gFontCache == this)
        gFontCache = nullptr;
}

juce::Typeface::Ptr FontCache::get (int weight) const
{
    if (weight >= 700) return bold;
    if (weight >= 600) return semiBold;
    if (weight >= 500) return medium;
    return regular;
}

FontCache* FontCache::instance()
{
    return gFontCache;
}

juce::Font Fonts::get (int weight, float px)
{
    if (auto* cache = FontCache::instance())
        if (auto tf = cache->get (weight))
            return juce::Font (tf).withPointHeight (px);

    return juce::Font (px * 1.2f, weight >= 600 ? juce::Font::bold : juce::Font::plain);
}

void drawParagraph (juce::Graphics& g, const juce::String& text, const juce::Font& font, juce::Colour colour,
                    juce::Rectangle<float> area, float lineHeightPx, juce::Justification justification)
{
    juce::AttributedString as;
    as.setJustification (justification);
    as.append (text, font, colour);
    as.setLineSpacing (juce::jmax (0.0f, lineHeightPx - font.getHeight()));

    juce::TextLayout layout;
    layout.createLayout (as, area.getWidth());
    layout.draw (g, area);
}

float paragraphHeight (const juce::String& text, const juce::Font& font, float width, float lineHeightPx)
{
    juce::AttributedString as;
    as.append (text, font, juce::Colours::black);
    as.setLineSpacing (juce::jmax (0.0f, lineHeightPx - font.getHeight()));
    juce::TextLayout layout;
    layout.createLayout (as, width);
    return layout.getHeight();
}

float textWidth (const juce::Font& font, const juce::String& text)
{
    return font.getStringWidthFloat (text);
}

float logoWidth (float fontPx, float letterSpacingEm)
{
    auto font = Fonts::get (700, fontPx);
    float w = 0.0f;
    const juce::String letters ("MEON");
    for (int i = 0; i < letters.length(); ++i)
        w += font.getStringWidthFloat (letters.substring (i, i + 1));
    return w + letterSpacingEm * fontPx * 3.0f;
}

void drawLogo (juce::Graphics& g, juce::Rectangle<float> area, float fontPx, float letterSpacingEm, juce::Justification justification)
{
    auto font = Fonts::get (700, fontPx);
    const juce::String letters ("MEON");
    const float total = logoWidth (fontPx, letterSpacingEm);
    float x = area.getX();
    if (justification.testFlags (juce::Justification::horizontallyCentred))
        x = area.getCentreX() - total * 0.5f;
    else if (justification.testFlags (juce::Justification::right))
        x = area.getRight() - total;

    g.setFont (font);
    for (int i = 0; i < letters.length(); ++i)
    {
        auto ch = letters.substring (i, i + 1);
        float w = font.getStringWidthFloat (ch);
        g.setColour (ch == "O" ? col::accent : col::ink);
        g.drawText (ch, juce::Rectangle<float> (x, area.getY(), w + 2.0f, area.getHeight()), juce::Justification::centredLeft, false);
        x += w + letterSpacingEm * fontPx;
    }
}

float spacedTextWidth (const juce::String& text, const juce::Font& font, float fontPx, float letterSpacingEm)
{
    float w = 0.0f;
    for (int i = 0; i < text.length(); ++i)
        w += font.getStringWidthFloat (text.substring (i, i + 1));
    return w + letterSpacingEm * fontPx * (float) juce::jmax (0, text.length() - 1);
}

void drawSpacedText (juce::Graphics& g, const juce::String& text, const juce::Font& font, float fontPx, float letterSpacingEm,
                     juce::Colour colour, juce::Rectangle<float> area, juce::Justification justification)
{
    const float total = spacedTextWidth (text, font, fontPx, letterSpacingEm);
    float x = area.getX();
    if (justification.testFlags (juce::Justification::horizontallyCentred))
        x = area.getCentreX() - total * 0.5f;
    else if (justification.testFlags (juce::Justification::right))
        x = area.getRight() - total;

    g.setFont (font);
    g.setColour (colour);
    for (int i = 0; i < text.length(); ++i)
    {
        auto ch = text.substring (i, i + 1);
        const float w = font.getStringWidthFloat (ch);
        g.drawText (ch, juce::Rectangle<float> (x, area.getY(), w + 2.0f, area.getHeight()), juce::Justification::centredLeft, false);
        x += w + letterSpacingEm * fontPx;
    }
}

} // namespace meon
