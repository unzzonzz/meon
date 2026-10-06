#include "MeonParts.h"
#include <array>

namespace meon
{

namespace parts
{

juce::String key (Part p)
{
    switch (p)
    {
        case Part::Vocal:  return "vocal";
        case Part::Guitar: return "guitar";
        case Part::Bass:   return "bass";
        case Part::Drums:  return "drums";
        case Part::Keys:   return "keys";
        case Part::Other:  return "other";
        default:           return {};
    }
}

Part fromKey (const juce::String& k)
{
    for (int i = 0; i < count; ++i)
        if (key (at (i)) == k)
            return at (i);
    return Part::None;
}

juce::String label (Part p)
{
    switch (p)
    {
        case Part::Vocal:  return TXT ("보컬");
        case Part::Guitar: return TXT ("기타");
        case Part::Bass:   return TXT ("베이스");
        case Part::Drums:  return TXT ("드럼");
        case Part::Keys:   return TXT ("키보드");
        case Part::Other:  return TXT ("그 외");
        default:           return {};
    }
}

juce::juce_wchar code (Part p)
{
    static const char codes[] = "vgbdko";
    return p == Part::None ? 0 : (juce::juce_wchar) codes[(int) p];
}

Part fromCode (juce::juce_wchar c)
{
    for (int i = 0; i < count; ++i)
        if (code (at (i)) == c)
            return at (i);
    return Part::None;
}

//==============================================================================
namespace
{
    /** 24 그리드 기준 도형. stroke 는 선으로, fill 은 면(+같은 선)으로 그린다. */
    struct Shape { juce::Path stroke, fill; };

    juce::Path svg (const char* d) { return juce::Drawable::parseSVGPath (juce::String (d)); }

    void roundRect (juce::Path& p, float x, float y, float w, float h, float r) { p.addRoundedRectangle (x, y, w, h, r); }

    Shape build (Part part)
    {
        Shape s;
        auto& st = s.stroke;
        switch (part)
        {
            case Part::Vocal:
                roundRect (st, 9.0f, 2.5f, 6.0f, 11.0f, 3.0f);
                st.addPath (svg ("M5.5 11a6.5 6.5 0 0 0 13 0M12 17.5v4M8.5 21.5h7"));
                break;

            case Part::Guitar:
                roundRect (st, 10.6f, 2.0f, 2.8f, 3.6f, 0.9f);
                st.addPath (svg ("M12 5.6v3.9"));
                st.addPath (svg ("M12 9.5C14.2 9.5 15.6 10.8 15.6 12.5C15.6 13.7 14.8 14.2 14.8 15.1C14.8 16 17.3 16.6 17.3 19"
                                 "C17.3 21.2 15 22.5 12 22.5C9 22.5 6.7 21.2 6.7 19C6.7 16.6 9.2 16 9.2 15.1C9.2 14.2 8.4 13.7 8.4 12.5"
                                 "C8.4 10.8 9.8 9.5 12 9.5Z"));
                st.addEllipse (12.0f - 1.5f, 16.7f - 1.5f, 3.0f, 3.0f);
                st.addPath (svg ("M10.6 20.1h2.8"));
                st.applyTransform (juce::AffineTransform::rotation (juce::MathConstants<float>::pi / 4.0f, 12.0f, 12.25f));
                break;

            case Part::Bass:
                roundRect (st, 10.8f, 1.2f, 2.4f, 3.8f, 0.8f);
                st.addPath (svg ("M12 5v9.2"));
                st.addPath (svg ("M9.5 13.5c-.8 0-1 1-.7 2.1.2.8-1.3 1.4-1.3 3.4 0 2.3 2 3.5 4.5 3.5s4.5-1.2 4.5-3.5"
                                 "c0-1.8-1.3-2.4-1.1-3.4.3-1.4.8-2.8-.2-2.8-.9 0-1.2 1.7-3.2 1.7-1.5 0-1.7-1-2.5-1z"));
                st.addPath (svg ("M10.3 18.6h3.4"));
                st.applyTransform (juce::AffineTransform::rotation (juce::MathConstants<float>::pi / 4.0f, 12.0f, 12.0f));
                break;

            case Part::Drums:
                st.addEllipse (4.0f, 13.0f - 2.6f, 16.0f, 5.2f);
                st.addPath (svg ("M4 13v5c0 1.44 3.58 2.6 8 2.6s8-1.16 8-2.6v-5"));
                st.addPath (svg ("M7 3.5l9.6 9M17 3.5l-9.6 9"));
                break;

            case Part::Keys:
                roundRect (st, 2.5f, 5.0f, 19.0f, 14.0f, 2.0f);
                st.addPath (svg ("M7.25 12.5V19M12 12.5V19M16.75 12.5V19"));
                roundRect (s.fill, 6.0f, 5.0f, 2.5f, 7.5f, 0.6f);
                roundRect (s.fill, 10.75f, 5.0f, 2.5f, 7.5f, 0.6f);
                roundRect (s.fill, 15.5f, 5.0f, 2.5f, 7.5f, 0.6f);
                break;

            case Part::Other:
                st.addPath (svg ("M9 17.5V5.5l11-2v12"));
                st.addEllipse (6.5f - 2.5f, 17.5f - 2.5f, 5.0f, 5.0f);
                st.addEllipse (17.5f - 2.5f, 15.5f - 2.5f, 5.0f, 5.0f);
                break;

            default:
                break;
        }
        return s;
    }

    const Shape& shapeFor (Part p)
    {
        static const std::array<Shape, count> shapes = []
        {
            std::array<Shape, count> a;
            for (int i = 0; i < count; ++i)
                a[(size_t) i] = build (at (i));
            return a;
        }();
        return shapes[(size_t) juce::jlimit (0, count - 1, (int) p)];
    }
}

void drawIcon (juce::Graphics& g, juce::Rectangle<float> area, float size, Part p, juce::Colour colour)
{
    if (p == Part::None)
        return;
    const auto& shape = shapeFor (p);
    // 도형만 크기에 맞춰 늘리고, 선 굵기는 화면에서 1.5px 고정 (SVG 의 non-scaling-stroke)
    const auto t = juce::AffineTransform::scale (size / 24.0f)
                       .translated (area.getCentreX() - size * 0.5f, area.getCentreY() - size * 0.5f);
    const juce::PathStrokeType stroke (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    g.setColour (colour);
    if (! shape.fill.isEmpty())
    {
        auto f = shape.fill;
        f.applyTransform (t);
        g.fillPath (f);
        g.strokePath (f, stroke);
    }
    auto s = shape.stroke;
    s.applyTransform (t);
    g.strokePath (s, stroke);
}

} // namespace parts

//==============================================================================
class PartSelector::Cell : public juce::Button
{
public:
    Cell (PartSelector& o, Part p) : juce::Button (parts::key (p)), owner (o), part (p)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setWantsKeyboardFocus (false);
        onClick = [this] { owner.setSelected (part); if (owner.onChange) owner.onChange (part); };
    }

    void paintButton (juce::Graphics& g, bool, bool) override
    {
        const auto& st = owner.style;
        const bool on = owner.selected == part;
        auto r = getLocalBounds().toFloat();
        g.setColour (on ? col::accent : col::white);
        g.fillRoundedRectangle (r, (float) metric::radiusCard);
        g.setColour (on ? col::accent : col::border);
        g.drawRoundedRectangle (r.reduced (0.5f), (float) metric::radiusCard, 1.0f);

        // 아이콘 위 · 라벨 아래, 둘을 묶어 세로 가운데
        auto font = Fonts::get (st.labelWeight, st.labelPx);
        const float labelH = font.getHeight();
        const float totalH = st.iconSize + (float) st.iconLabelGap + labelH;
        const float top = r.getCentreY() - totalH * 0.5f;
        parts::drawIcon (g, { r.getX(), top, r.getWidth(), st.iconSize }, st.iconSize, part, col::ink);
        g.setColour (col::ink);
        g.setFont (font);
        g.drawText (parts::label (part), juce::Rectangle<float> (r.getX(), top + st.iconSize + (float) st.iconLabelGap, r.getWidth(), labelH),
                    juce::Justification::centred, false);
    }

private:
    PartSelector& owner;
    const Part part;
};

PartSelector::PartSelector (Style s) : style (s)
{
    for (int i = 0; i < parts::count; ++i)
        addAndMakeVisible (cells.add (new Cell (*this, parts::at (i))));
}

PartSelector::~PartSelector() = default;

void PartSelector::setSelected (Part p)
{
    selected = p;
    for (auto* c : cells)
        c->repaint();
}

void PartSelector::resized()
{
    // 6칸 균등 (grid repeat(6, 1fr))
    const float cellW = (float) (getWidth() - style.gap * (parts::count - 1)) / (float) parts::count;
    for (int i = 0; i < parts::count; ++i)
    {
        const int x0 = (int) std::round ((float) i * (cellW + (float) style.gap));
        const int x1 = (int) std::round ((float) i * (cellW + (float) style.gap) + cellW);
        cells[i]->setBounds (x0, 0, x1 - x0, getHeight());
    }
}

} // namespace meon
