#include "MeonTheme.h"
#include "MeonEmoji.h"

namespace meon
{

static FontCache* gFontCache = nullptr;

FontCache::FontCache()
{
#if JUCE_WINDOWS
    // JUCE 7 의 Windows 메모리 글꼴은 이름표 4번(전체 이름)으로 GDI 에서 다시 찾는다.
    // Medium/SemiBold 는 전체 이름이 곧 패밀리 이름이라 그대로 되지만, Regular/Bold 는
    // 'Pretendard Regular'/'Pretendard Bold' 라는 패밀리가 없어서 다른 글꼴로 대체된다.
    // 그래서 네 파일을 모두 프로세스에 등록해 둔 채로(memoryFonts 가 살아 있는 동안 유지),
    // Regular/Bold 는 패밀리 'Pretendard' + 스타일로 다시 만든다.
    memoryFonts.add (juce::Typeface::createSystemTypefaceFor (BinaryData::PretendardRegular_ttf,  BinaryData::PretendardRegular_ttfSize));
    memoryFonts.add (juce::Typeface::createSystemTypefaceFor (BinaryData::PretendardBold_ttf,     BinaryData::PretendardBold_ttfSize));
    medium   = juce::Typeface::createSystemTypefaceFor (BinaryData::PretendardMedium_ttf,   BinaryData::PretendardMedium_ttfSize);
    semiBold = juce::Typeface::createSystemTypefaceFor (BinaryData::PretendardSemiBold_ttf, BinaryData::PretendardSemiBold_ttfSize);
    regular  = juce::Typeface::createSystemTypefaceFor (juce::Font ("Pretendard", "Regular", 16.0f));
    bold     = juce::Typeface::createSystemTypefaceFor (juce::Font ("Pretendard", "Bold", 16.0f));
#else
    regular  = juce::Typeface::createSystemTypefaceFor (BinaryData::PretendardRegular_otf,  BinaryData::PretendardRegular_otfSize);
    medium   = juce::Typeface::createSystemTypefaceFor (BinaryData::PretendardMedium_otf,   BinaryData::PretendardMedium_otfSize);
    semiBold = juce::Typeface::createSystemTypefaceFor (BinaryData::PretendardSemiBold_otf, BinaryData::PretendardSemiBold_otfSize);
    bold     = juce::Typeface::createSystemTypefaceFor (BinaryData::PretendardBold_otf,     BinaryData::PretendardBold_otfSize);
#endif
    if constexpr (! emoji::nativeLayout)
    {
        inputRegular  = emoji::makeEmojiAwareTypeface (regular);
        inputMedium   = emoji::makeEmojiAwareTypeface (medium);
        inputSemiBold = emoji::makeEmojiAwareTypeface (semiBold);
        inputBold     = emoji::makeEmojiAwareTypeface (bold);
    }
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

juce::Typeface::Ptr FontCache::getForInput (int weight) const
{
    if (weight >= 700) return inputBold;
    if (weight >= 600) return inputSemiBold;
    if (weight >= 500) return inputMedium;
    return inputRegular;
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

juce::Font Fonts::forInput (int weight, float px)
{
    if (auto* cache = FontCache::instance())
        if (auto tf = cache->getForInput (weight))
            return juce::Font (tf).withPointHeight (px);

    return get (weight, px);
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

void drawParagraphWithEmoji (juce::Graphics& g, const juce::String& text, const juce::Font& font, juce::Colour colour,
                             juce::Rectangle<float> area, float lineHeightPx, juce::Justification justification)
{
    if constexpr (! emoji::nativeLayout)
    {
        emoji::drawLines (g, emoji::layout (text, font, area.getWidth()), font, colour, area, lineHeightPx, justification);
        return;
    }

    juce::AttributedString as;
    as.setJustification (justification);
    as.append (text, font, colour);
    as.setLineSpacing (juce::jmax (0.0f, lineHeightPx - font.getHeight()));

    // macOS 의 CTFrame 은 영역에 다 들어가지 않는 마지막 줄을 아예 빼 버린다. 위쪽 정렬이면 아래로 여유를 준다.
    if (justification.getOnlyVerticalFlags() == juce::Justification::top)
        area = area.withHeight (area.getHeight() + lineHeightPx * 2.0f);

    as.draw (g, area);   // macOS: CoreText 로 그려 컬러 이모지가 나온다. 그 외: TextLayout 으로 그린다.
}

float paragraphHeightWithEmoji (const juce::String& text, const juce::Font& font, float width, float lineHeightPx)
{
    if constexpr (! emoji::nativeLayout)
        return emoji::height (emoji::layout (text, font, width), font, lineHeightPx);

    return paragraphHeight (text, font, width, lineHeightPx);
}

float lineWidthWithEmoji (const juce::String& text, const juce::Font& font)
{
    if constexpr (! emoji::nativeLayout)
    {
        const auto lines = emoji::layout (text, font, 0.0f);
        return lines.empty() ? 0.0f : lines.front().width;
    }

    juce::AttributedString as;
    as.append (text, font, juce::Colours::black);
    as.setWordWrap (juce::AttributedString::none);
    juce::TextLayout layout;
    layout.createLayout (as, 1.0e6f);
    return layout.getNumLines() > 0 ? layout.getLine (0).getLineBoundsX().getLength() : 0.0f;
}

void drawLineWithEmoji (juce::Graphics& g, const juce::String& text, const juce::Font& font, juce::Colour colour,
                        juce::Rectangle<float> area, juce::Justification justification)
{
    auto shown = text.replaceCharacters ("\r\n", "  ");
    if (lineWidthWithEmoji (shown, font) > area.getWidth())
    {
        // 들어가는 가장 긴 앞부분 + '…' (글자 수로 이분 탐색)
        const juce::String ellipsis = juce::String::charToString ((juce::juce_wchar) 0x2026);
        int lo = 0, hi = shown.length();
        while (lo < hi)
        {
            const int mid = (lo + hi + 1) / 2;
            if (lineWidthWithEmoji (shown.substring (0, mid).trimEnd() + ellipsis, font) <= area.getWidth())
                lo = mid;
            else
                hi = mid - 1;
        }
        shown = shown.substring (0, lo).trimEnd() + ellipsis;
    }

    if constexpr (! emoji::nativeLayout)
    {
        emoji::drawLines (g, emoji::layout (shown, font, 0.0f), font, colour, area, font.getHeight(), justification);
        return;
    }

    juce::AttributedString as;
    as.setJustification (justification);
    as.setWordWrap (juce::AttributedString::none);
    as.append (shown, font, colour);
    as.draw (g, area);
}

float textWidth (const juce::Font& font, const juce::String& text)
{
    return font.getStringWidthFloat (text);
}

// 로고 SVG(images/meon_logo.svg)의 viewBox 크기. 로고 높이는 이 viewBox 높이 기준이다.
static constexpr float logoViewW = 396.5f;
static constexpr float logoViewH = 152.5f;

static const juce::Path& logoPath()
{
    static const juce::Path path = []
    {
        juce::Path p;
        if (auto xml = juce::XmlDocument::parse (juce::String::fromUTF8 (BinaryData::meon_logo_svg, BinaryData::meon_logo_svgSize)))
            if (auto* e = xml->getChildByName ("path"))
                p = juce::Drawable::parseSVGPath (e->getStringAttribute ("d"));
        p.setUsingNonZeroWinding (false); // fill-rule="evenodd"
        return p;
    }();
    return path;
}

float logoWidth (float heightPx)
{
    return heightPx * logoViewW / logoViewH;
}

void drawLogo (juce::Graphics& g, juce::Rectangle<float> area, float heightPx, juce::Colour colour, juce::Justification justification)
{
    const float w = logoWidth (heightPx);
    float x = area.getX();
    if (justification.testFlags (juce::Justification::horizontallyCentred))
        x = area.getCentreX() - w * 0.5f;
    else if (justification.testFlags (juce::Justification::right))
        x = area.getRight() - w;

    float y = area.getCentreY() - heightPx * 0.5f;
    if (justification.testFlags (juce::Justification::top))
        y = area.getY();
    else if (justification.testFlags (juce::Justification::bottom))
        y = area.getBottom() - heightPx;

    const float s = heightPx / logoViewH;
    g.setColour (colour);
    g.fillPath (logoPath(), juce::AffineTransform::scale (s).translated (x, y));
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
