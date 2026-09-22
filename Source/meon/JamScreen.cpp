#include "JamScreen.h"

namespace meon
{

//==============================================================================
juce::String JamScreen::koreanTime (const juce::Time& t)
{
    const int h = t.getHours();
    const int h12 = (h % 12 == 0) ? 12 : h % 12;
    return (h < 12 ? TXT ("오전 ") : TXT ("오후 ")) + juce::String (h12) + ":" + juce::String (t.getMinutes()).paddedLeft ('0', 2);
}

//==============================================================================
class JamScreen::MyPanel : public juce::Component
{
public:
    explicit MyPanel (JamScreen& o) : owner (o), plugin (o.isPlugin()), muteButton ({}, MeonButton::Style::Secondary)
    {
        meter.setCornerRadius (3.0f);
        muteButton.setFont (plugin ? 15.0f : 17.0f, 600);
        muteButton.setBadge ("M");
        muteButton.setBadgeMetrics (plugin ? 20.0f : 24.0f, plugin ? 11.0f : 12.0f, plugin ? 10.0f : 12.0f);
        muteButton.onClick = [this] { owner.getEditor().getSession().setMyMuted (! owner.getEditor().getSession().isMyMuted()); update(); };
        addAndMakeVisible (meter);
        addAndMakeVisible (muteButton);
        update();
    }

    void update()
    {
        const bool muted = owner.getEditor().getSession().isMyMuted();
        muteButton.setLabel (muted ? TXT ("마이크 꺼짐") : TXT ("마이크 켜짐"));
        muteButton.setStyle (muted ? MeonButton::Style::Primary : MeonButton::Style::Secondary);
        meter.setDisabledLook (false);
        if (muted)
            meter.reset();
        resized();
        repaint();
    }

    void pushLevel (float db, double dt, double nowMs)
    {
        meter.push (db, dt);
        if (db >= peakDb || nowMs - peakAtMs > 1500.0)
        {
            peakDb = db;
            peakAtMs = nowMs;
        }
        if (! plugin)
            repaint (labelArea);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (plugin ? 14 : 20, plugin ? 12 : 18);
        const int btnH = plugin ? 42 : 52;
        const int btnW = muteButton.getIdealWidth (plugin ? 16 : 22);
        muteButton.setBounds (r.getRight() - btnW, r.getCentreY() - btnH / 2, btnW, btnH);
        const int nameW = plugin ? 96 : 150;
        auto mid = r.withTrimmedLeft (nameW + (plugin ? 16 : 24)).withTrimmedRight (btnW + (plugin ? 16 : 24));
        if (plugin)
        {
            meter.setBounds (mid.withSizeKeepingCentre (mid.getWidth(), 12));
            labelArea = {};
        }
        else
        {
            const int labelH = 15, meterH = 14, gap = 8;
            const int totalH = labelH + gap + meterH;
            const int y = mid.getCentreY() - totalH / 2;
            labelArea = juce::Rectangle<int> (mid.getX(), y, mid.getWidth(), labelH);
            meter.setBounds (mid.getX(), y + labelH + gap, mid.getWidth(), meterH);
        }
    }

    void paint (juce::Graphics& g) override
    {
        const bool muted = owner.getEditor().getSession().isMyMuted();
        auto r = getLocalBounds().toFloat();
        g.setColour (muted ? col::panel : col::white);
        g.fillRoundedRectangle (r, (float) metric::radiusCard);
        g.setColour (muted ? col::accent : col::cardBorder);
        g.drawRoundedRectangle (r.reduced (0.5f), (float) metric::radiusCard, 1.0f);

        auto inner = getLocalBounds().reduced (plugin ? 14 : 20, plugin ? 12 : 18);
        auto nameFont = Fonts::get (700, plugin ? 18.0f : 22.0f);
        auto subFont = Fonts::get (500, plugin ? 12.0f : 13.0f);
        const int nameH = (int) std::ceil (nameFont.getHeight()), subH = (int) std::ceil (subFont.getHeight());
        const int gap = plugin ? 2 : 3;
        const int y = inner.getCentreY() - (nameH + gap + subH) / 2;
        g.setColour (col::ink);
        g.setFont (nameFont);
        g.drawText (owner.getEditor().getSession().getDisplayName(), juce::Rectangle<int> (inner.getX(), y, plugin ? 96 : 150, nameH), juce::Justification::centredLeft, true);
        g.setColour (col::inkSub);
        g.setFont (subFont);
        g.drawText (TXT ("나"), juce::Rectangle<int> (inner.getX(), y + nameH + gap, plugin ? 96 : 150, subH), juce::Justification::centredLeft, false);

        if (! plugin)
        {
            g.setFont (Fonts::get (500, 12.0f));
            g.setColour (col::inkSub);
            g.drawText (TXT ("내 입력 레벨"), labelArea, juce::Justification::centredLeft, false);
            juce::String right = muted ? TXT ("마이크가 꺼져 있어 아무도 못 들어요")
                                       : TXT ("최고 ") + (peakDb <= -60.0f ? juce::String ("-60") : juce::String ((int) std::lround (peakDb))) + " dB";
            g.drawText (right, labelArea, juce::Justification::centredRight, false);
        }
    }

private:
    JamScreen& owner;
    const bool plugin;
    LevelBar meter;
    MeonButton muteButton;
    juce::Rectangle<int> labelArea;
    float peakDb = -100.0f;
    double peakAtMs = 0.0;
};

//==============================================================================
class JamScreen::MemberCard : public juce::Component
{
public:
    MemberCard (JamScreen& o, int slotIn) : owner (o), plugin (o.isPlugin()), slot (slotIn), muteButton (TXT ("뮤트"))
    {
        meter.setCornerRadius (3.0f);
        volume.setColour (juce::Slider::backgroundColourId, col::cardBorder);
        volume.getProperties().set ("meonThumb", plugin ? 14 : 16);
        volume.onValueChange = [this]
        {
            if (! syncing && haveMember)
            {
                owner.getEditor().getSession().setMemberGain (slot, (float) volume.getValue());
                member.gain = (float) volume.getValue();
                repaint (volTextArea);
            }
        };
        muteButton.setFont (plugin ? 13.0f : 15.0f, 600);
        muteButton.setBadge (juce::String (slot + 1));
        muteButton.setBadgeMetrics (plugin ? 18.0f : 22.0f, plugin ? 11.0f : 12.0f, plugin ? 7.0f : 9.0f);
        muteButton.onClick = [this]
        {
            if (haveMember && member.connected)
                owner.getEditor().getSession().setMemberMuted (slot, ! member.muted);
        };
        addAndMakeVisible (meter);
        addAndMakeVisible (volume);
        addAndMakeVisible (muteButton);
    }

    int getSlot() const { return slot; }

    void update (const MeonSession::Member& m)
    {
        member = m;
        haveMember = true;
        const bool off = ! m.connected;
        syncing = true;
        volume.setEnabled (! off);
        if (off)
            volume.setValue (0.0, juce::dontSendNotification);
        else if (! volume.isMouseButtonDown())
            volume.setValue (m.gain, juce::dontSendNotification);
        syncing = false;

        muteButton.setLabel ((m.muted && ! off) ? TXT ("소리 켜기") : TXT ("뮤트"));
        muteButton.setStyle ((m.muted && ! off) ? MeonButton::Style::Dark : MeonButton::Style::Secondary);
        if (off)
            muteButton.setColourOverride (col::white, col::disabled, col::cardBorder, col::white);
        else
            muteButton.clearColourOverride();
        meter.setDisabledLook (off);
        if (off || m.muted)
            meter.reset();
        resized();
        repaint();
    }

    void pushLevel (float db, double dt)
    {
        if (haveMember && member.connected && ! member.muted)
            meter.push (db, dt);
    }

    void resized() override
    {
        const int padX = plugin ? 14 : 20, padY = plugin ? 12 : 18;
        auto r = getLocalBounds().reduced (padX, padY);
        auto nameFont = Fonts::get (700, plugin ? 18.0f : 24.0f);
        const int headerH = (int) std::ceil (nameFont.getHeight());
        const int muteH = plugin ? 30 : 38;
        const int meterH = plugin ? 10 : 12, volH = 16;
        const int muteW = muteButton.getIdealWidth (plugin ? 10 : 14);
        muteButton.setBounds (r.getX(), r.getBottom() - muteH, muteW, muteH);

        // 헤더와 뮤트 줄 사이에 미터와 볼륨 줄을 균등 배치
        const int avail = (r.getBottom() - muteH) - (r.getY() + headerH);
        const int gap = juce::jmax (8, (avail - meterH - volH) / 3);
        const int meterY = r.getY() + headerH + gap;
        meter.setBounds (r.getX(), meterY, r.getWidth(), meterH);
        const int volY = meterY + meterH + gap;
        const int labelW = plugin ? 0 : (int) std::ceil (Fonts::get (500, 13.0f).getStringWidthFloat (TXT ("볼륨"))) + 14;
        const int textW = plugin ? 58 : 66;
        volume.setBounds (r.getX() + labelW, volY, r.getWidth() - labelW - textW - (plugin ? 10 : 14), volH);
        volTextArea = juce::Rectangle<int> (r.getRight() - textW, volY, textW, volH);
        volLabelArea = juce::Rectangle<int> (r.getX(), volY, labelW, volH);
        statusArea = juce::Rectangle<int> (muteButton.getRight() + (plugin ? 8 : 10), muteButton.getY(), r.getRight() - muteButton.getRight() - 10, muteH);
    }

    void paint (juce::Graphics& g) override
    {
        const bool off = haveMember && ! member.connected;
        const bool warn = haveMember && member.connected && member.hasStats && member.pingMs > metric::pingWarnMs;
        auto r = getLocalBounds().toFloat();
        g.setColour (off ? col::panel : col::white);
        g.fillRoundedRectangle (r, (float) metric::radiusCard);
        g.setColour (warn ? col::accent : col::cardBorder);
        g.drawRoundedRectangle (r.reduced (0.5f), (float) metric::radiusCard, 1.0f);

        const int padX = plugin ? 14 : 20, padY = plugin ? 12 : 18;
        auto inner = getLocalBounds().reduced (padX, padY);
        auto nameFont = Fonts::get (700, plugin ? 18.0f : 24.0f);
        const int headerH = (int) std::ceil (nameFont.getHeight());
        const float dot = plugin ? 8.0f : 9.0f;
        const juce::Colour ink = off ? col::disabled : col::ink;

        // 이름 줄: 점 + 이름
        g.setColour (off ? col::disabled : col::accent);
        g.fillEllipse ((float) inner.getX(), (float) inner.getY() + (float) headerH * 0.5f - dot * 0.5f, dot, dot);
        g.setColour (ink);
        g.setFont (nameFont);
        const int nameX = inner.getX() + (int) dot + (plugin ? 8 : 10);
        g.drawText (member.displayName, juce::Rectangle<int> (nameX, inner.getY(), inner.getWidth() / 2, headerH), juce::Justification::centredLeft, true);

        // 핑 + 지연 배지 (오른쪽)
        juce::String pingText = (off || ! member.hasStats) ? TXT ("— ms") : juce::String ((int) std::lround (member.pingMs)) + " ms";
        auto pingFont = Fonts::get (600, plugin ? 13.0f : 15.0f);
        g.setFont (pingFont);
        g.setColour (off ? col::disabled : (warn ? col::accent : col::inkBody));
        const int pingW = (int) std::ceil (pingFont.getStringWidthFloat (pingText));
        g.drawText (pingText, juce::Rectangle<int> (inner.getRight() - pingW, inner.getY(), pingW, headerH), juce::Justification::centredRight, false);
        if (warn)
        {
            auto badgeFont = Fonts::get (600, plugin ? 11.0f : 12.0f);
            const float bw = badgeFont.getStringWidthFloat (TXT ("지연")) + (plugin ? 12.0f : 16.0f);
            const float bh = badgeFont.getHeight() + (plugin ? 4.0f : 6.0f);
            juce::Rectangle<float> badge ((float) (inner.getRight() - pingW) - (plugin ? 6.0f : 8.0f) - bw, (float) inner.getY() + (float) headerH * 0.5f - bh * 0.5f, bw, bh);
            g.setColour (col::accent);
            g.fillRoundedRectangle (badge, 4.0f);
            g.setColour (col::white);
            g.setFont (badgeFont);
            g.drawText (TXT ("지연"), badge, juce::Justification::centred, false);
        }

        // 볼륨 라벨 / 값
        if (! plugin)
        {
            g.setFont (Fonts::get (500, 13.0f));
            g.setColour (col::inkSub);
            g.drawText (TXT ("볼륨"), volLabelArea, juce::Justification::centredLeft, false);
        }
        g.setFont (Fonts::get (600, plugin ? 13.0f : 14.0f));
        g.setColour (ink);
        g.drawText (off ? TXT ("—") : VolumeSlider::dbText (volume.getValue()), volTextArea, juce::Justification::centredRight, false);

        // 상태 문구
        juce::String status;
        juce::Colour statusInk = col::inkSub;
        if (off && member.joinFailed)   { status = TXT ("연결 실패 · 네트워크 확인 필요"); statusInk = col::inkSub; }
        else if (off)                   { status = TXT ("연결 끊김 · 재연결 중"); }
        else if (member.pending)        { status = TXT ("연결 중…"); }
        else if (warn)                  { status = TXT ("지연이 조금 있어요"); statusInk = col::accent; }
        else if (member.muted)          { status = TXT ("소리 꺼짐"); }
        if (status.isNotEmpty())
        {
            g.setFont (Fonts::get (500, plugin ? 12.0f : 13.0f));
            g.setColour (statusInk);
            g.drawText (status, statusArea, juce::Justification::centredLeft, true);
        }
    }

private:
    JamScreen& owner;
    const bool plugin;
    const int slot;
    LevelBar meter;
    VolumeSlider volume;
    MeonButton muteButton;
    MeonSession::Member member;
    bool haveMember = false, syncing = false;
    juce::Rectangle<int> volTextArea, volLabelArea, statusArea;
};

//==============================================================================
class JamScreen::EmptySlot : public juce::Component
{
public:
    explicit EmptySlot (bool p) : plugin (p) { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        juce::Path p;
        p.addRoundedRectangle (r, (float) metric::radiusCard);
        const float dashes[] = { 4.0f, 3.0f };
        juce::Path dashed;
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, p, dashes, 2);
        g.setColour (col::border);
        g.fillPath (dashed);
        g.setColour (col::disabled);
        g.setFont (Fonts::get (500, plugin ? 13.0f : 15.0f));
        g.drawText (TXT ("빈 자리"), getLocalBounds(), juce::Justification::centred, false);
    }
private:
    const bool plugin;
};

//==============================================================================
class JamScreen::AlonePanel : public juce::Component
{
public:
    explicit AlonePanel (JamScreen& o) : owner (o), plugin (o.isPlugin()), copyButton (TXT ("코드 복사"), MeonButton::Style::Primary)
    {
        copyButton.setFont (plugin ? 15.0f : 17.0f, 600);
        copyButton.setBadge (MeonEditor::cmdKeyLabel ("C"));
        copyButton.setBadgeMetrics (plugin ? 20.0f : 22.0f, 11.0f, plugin ? 10.0f : 12.0f);
        copyButton.onClick = [this] { owner.copyCode(); };
        addAndMakeVisible (copyButton);
    }

    void setCopied (bool copied)
    {
        copyButton.setLabel (copied ? TXT ("복사됨") : TXT ("코드 복사"));
        resized();
    }

    void resized() override
    {
        const float titlePx = plugin ? 17.0f : 20.0f, codePx = plugin ? 62.0f : 92.0f;
        const int titleH = (int) std::ceil (Fonts::get (600, titlePx).getHeight());
        const int subH = plugin ? 0 : (int) std::ceil (Fonts::get (400, 15.0f).getHeight()) + 10;
        const int codeH = (int) codePx;
        const int btnH = plugin ? 42 : 48;
        const int noteH = plugin ? 0 : 18 + 16;
        const int total = titleH + subH + (plugin ? 18 : 34) + codeH + (plugin ? 20 : 30) + btnH + noteH;
        top = getHeight() / 2 - total / 2;
        const int btnW = copyButton.getIdealWidth (plugin ? 22 : 28);
        copyButton.setBounds (getWidth() / 2 - btnW / 2, top + titleH + subH + (plugin ? 18 : 34) + codeH + (plugin ? 20 : 30), btnW, btnH);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (col::cardBorder);
        g.drawRoundedRectangle (r.reduced (0.5f), (float) metric::radiusCard, 1.0f);

        const float titlePx = plugin ? 17.0f : 20.0f, codePx = plugin ? 62.0f : 92.0f;
        const int titleH = (int) std::ceil (Fonts::get (600, titlePx).getHeight());
        int y = top;
        g.setColour (col::ink);
        g.setFont (Fonts::get (600, titlePx));
        g.drawText (TXT ("아직 혼자예요"), juce::Rectangle<int> (0, y, getWidth(), titleH), juce::Justification::centred, false);
        y += titleH;
        if (! plugin)
        {
            const int subH = (int) std::ceil (Fonts::get (400, 15.0f).getHeight());
            g.setColour (col::inkSub);
            g.setFont (Fonts::get (400, 15.0f));
            g.drawText (TXT ("이 코드를 보내면 바로 들어올 수 있어요"), juce::Rectangle<int> (0, y + 10, getWidth(), subH), juce::Justification::centred, false);
            y += 10 + subH;
        }
        y += plugin ? 18 : 34;
        drawSpacedText (g, owner.getEditor().getSession().getDisplayRoomCode(), Fonts::get (700, codePx), codePx, 0.14f, col::ink,
                        juce::Rectangle<float> (0.0f, (float) y, (float) getWidth(), codePx));
        if (! plugin)
        {
            g.setColour (col::disabled);
            g.setFont (Fonts::get (400, 13.0f));
            g.drawText (TXT ("나 포함 최대 5명"), juce::Rectangle<int> (0, copyButton.getBottom() + 18, getWidth(), 16), juce::Justification::centred, false);
        }
    }

private:
    JamScreen& owner;
    const bool plugin;
    MeonButton copyButton;
    int top = 0;
};

//==============================================================================
class JamScreen::ChatPanel : public juce::Component
{
public:
    explicit ChatPanel (JamScreen& o)
        : owner (o), plugin (o.isPlugin()), list (o),
          collapseButton (TXT ("접기")), sendButton (TXT ("전송")), input (plugin ? 13.0f : 14.0f)
    {
        collapseButton.setFont (plugin ? 11.0f : 12.0f, 500);
        collapseButton.setColourOverride (col::white, col::inkSub, col::border, col::panel);
        collapseButton.setBadge ("C");
        collapseButton.setBadgeMetrics (plugin ? 16.0f : 18.0f, plugin ? 10.0f : 11.0f, plugin ? 6.0f : 7.0f);
        collapseButton.onClick = [this] { owner.toggleChat(); };
        sendButton.setFont (plugin ? 12.0f : 14.0f, 500);
        sendButton.onClick = [this] { send(); };
        input.setIndents (plugin ? 10 : 12, 0);
        input.setPlaceholder (TXT ("메시지 입력"));
        input.onReturnKey = [this] { send(); };
        input.onEscapeKey = [this] { owner.requestLeave(); };
        viewport.setViewedComponent (&list, false);
        viewport.setScrollBarsShown (true, false, false, false);
        viewport.setScrollBarThickness (6);
        addAndMakeVisible (collapseButton);
        addAndMakeVisible (viewport);
        addAndMakeVisible (input);
        addAndMakeVisible (sendButton);
    }

    void refresh()
    {
        list.rebuild (viewport.getMaximumVisibleWidth());
        viewport.setViewPosition (0, juce::jmax (0, list.getHeight() - viewport.getViewHeight()));
        repaint();
    }

    void focusInput() { input.grabKeyboardFocus(); }

    void resized() override
    {
        const int headerH = plugin ? 40 : 48;
        const int rowPadY = plugin ? 10 : 12, rowPadX = plugin ? 12 : 16, inputH = plugin ? 34 : 40, sendW = plugin ? 46 : 56, gap = plugin ? 6 : 8;
        auto r = getLocalBounds();
        auto header = r.removeFromTop (headerH);
        const int cbW = collapseButton.getIdealWidth (plugin ? 8 : 10);
        collapseButton.setBounds (header.getRight() - rowPadX - cbW, header.getCentreY() - (plugin ? 12 : 14), cbW, plugin ? 24 : 28);
        auto bottom = r.removeFromBottom (rowPadY * 2 + inputH);
        auto row = bottom.reduced (rowPadX, rowPadY);
        sendButton.setBounds (row.removeFromRight (sendW));
        row.removeFromRight (gap);
        input.setBounds (row);
        viewport.setBounds (r);
        list.rebuild (viewport.getMaximumVisibleWidth());
        viewport.setViewPosition (0, juce::jmax (0, list.getHeight() - viewport.getViewHeight()));
    }

    void paint (juce::Graphics& g) override
    {
        const int headerH = plugin ? 40 : 48;
        const int rowPadY = plugin ? 10 : 12, inputH = plugin ? 34 : 40;
        g.fillAll (col::white);
        g.setColour (col::cardBorder);
        g.fillRect (0, 0, 1, getHeight());                               // 왼쪽 테두리
        g.fillRect (0, headerH - 1, getWidth(), 1);                      // 헤더 구분선
        g.fillRect (0, getHeight() - rowPadY * 2 - inputH, getWidth(), 1);
        g.setColour (col::ink);
        g.setFont (Fonts::get (600, plugin ? 13.0f : 15.0f));
        g.drawText (TXT ("채팅"), juce::Rectangle<int> (plugin ? 12 : 16, 0, 100, headerH), juce::Justification::centredLeft, false);
    }

private:
    class List : public juce::Component
    {
    public:
        explicit List (JamScreen& o) : owner (o), plugin (o.isPlugin()) { setInterceptsMouseClicks (false, false); }

        void rebuild (int width)
        {
            const int pad = plugin ? 12 : 16, gap = plugin ? 11 : 14;
            auto& chat = owner.getEditor().getSession().getChat();
            int h = pad;
            for (auto& m : chat)
                h += itemHeight (m, width - pad * 2) + gap;
            h += pad - gap;
            setSize (width, juce::jmax (h, 1));
            repaint();
        }

        int itemHeight (const MeonSession::ChatMessage& m, int width) const
        {
            if (m.kind == MeonSession::ChatMessage::System)
                return (int) std::ceil (Fonts::get (400, plugin ? 11.0f : 12.0f).getHeight());
            const float msgPx = plugin ? 13.0f : 15.0f;
            const int nameH = (int) std::ceil (Fonts::get (600, plugin ? 12.0f : 13.0f).getHeight());
            const int textH = (int) std::ceil (paragraphHeight (m.text, Fonts::get (400, msgPx), (float) width, msgPx * 1.5f));
            return nameH + (plugin ? 2 : 3) + textH;
        }

        void paint (juce::Graphics& g) override
        {
            const int pad = plugin ? 12 : 16, gap = plugin ? 11 : 14;
            const int width = getWidth() - pad * 2;
            auto& chat = owner.getEditor().getSession().getChat();
            int y = pad;
            for (auto& m : chat)
            {
                const int h = itemHeight (m, width);
                if (m.kind == MeonSession::ChatMessage::System)
                {
                    g.setColour (col::disabled);
                    g.setFont (Fonts::get (400, plugin ? 11.0f : 12.0f));
                    g.drawText (koreanTime (m.time) + TXT (" · ") + m.text, juce::Rectangle<int> (pad, y, width, h), juce::Justification::centred, true);
                }
                else
                {
                    const bool mine = m.kind == MeonSession::ChatMessage::Mine;
                    const int nameH = (int) std::ceil (Fonts::get (600, plugin ? 12.0f : 13.0f).getHeight());
                    g.setColour (mine ? col::accent : col::inkSub);
                    g.setFont (Fonts::get (600, plugin ? 12.0f : 13.0f));
                    g.drawText (mine ? TXT ("나") : m.from, juce::Rectangle<int> (pad, y, width, nameH), mine ? juce::Justification::centredRight : juce::Justification::centredLeft, true);
                    const float msgPx = plugin ? 13.0f : 15.0f;
                    drawParagraph (g, m.text, Fonts::get (400, msgPx), col::ink,
                                   juce::Rectangle<float> ((float) pad, (float) (y + nameH + (plugin ? 2 : 3)), (float) width, (float) (h - nameH)),
                                   msgPx * 1.5f, mine ? juce::Justification::topRight : juce::Justification::topLeft);
                }
                y += h + gap;
            }
        }

    private:
        JamScreen& owner;
        const bool plugin;
    };

    void send()
    {
        auto text = input.getText().trim();
        if (text.isEmpty())
            return;
        owner.sendChatText (text);
        input.clear();
    }

    JamScreen& owner;
    const bool plugin;
    List list;
    juce::Viewport viewport;
    MeonButton collapseButton, sendButton;
    MeonTextEditor input;
};

//==============================================================================
class JamScreen::ChatRail : public juce::Component
{
public:
    explicit ChatRail (JamScreen& o) : owner (o), plugin (o.isPlugin()), button ("C")
    {
        button.setFont (plugin ? 11.0f : 12.0f, 600);
        button.setColourOverride (col::white, col::inkBody, col::border, col::panel);
        button.onClick = [this] { owner.toggleChat(); };
        addAndMakeVisible (button);
    }

    void setUnread (bool u) { unread = u; repaint(); }

    void resized() override
    {
        const int sz = plugin ? 30 : 36;
        button.setBounds (getWidth() / 2 - sz / 2, plugin ? 12 : 14, sz, sz);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (col::white);
        g.setColour (col::cardBorder);
        g.fillRect (0, 0, 1, getHeight());

        const int gap = plugin ? 8 : 10;
        auto font = Fonts::get (500, plugin ? 12.0f : 13.0f);
        const juce::String text = TXT ("채팅");
        const float tw = font.getStringWidthFloat (text) + 4.0f;
        const float th = font.getHeight();
        // 세로 쓰기 (writing-mode: vertical-rl)
        juce::Graphics::ScopedSaveState ss (g);
        const float cx = (float) getWidth() * 0.5f;
        const float top = (float) (button.getBottom() + gap);
        g.addTransform (juce::AffineTransform::rotation (juce::MathConstants<float>::halfPi, cx, top));
        g.setColour (col::inkSub);
        g.setFont (font);
        g.drawText (text, juce::Rectangle<float> (cx, top - th * 0.5f, tw + 8.0f, th), juce::Justification::centredLeft, false);
        ss.~ScopedSaveState();

        if (unread)
        {
            const float d = plugin ? 7.0f : 8.0f;
            g.setColour (col::accent);
            g.fillEllipse ((float) getWidth() * 0.5f - d * 0.5f, top + tw + 8.0f + (float) gap, d, d);
        }
    }

private:
    JamScreen& owner;
    const bool plugin;
    MeonButton button;
    bool unread = false;
};

//==============================================================================
class JamScreen::LeaveDialog : public juce::Component
{
public:
    explicit LeaveDialog (JamScreen& o)
        : owner (o), plugin (o.isPlugin()),
          stayButton (TXT ("머무르기")), leaveButton (TXT ("나가기"), MeonButton::Style::Primary)
    {
        stayButton.setFont (plugin ? 15.0f : 16.0f, 500);
        leaveButton.setFont (plugin ? 15.0f : 16.0f, 600);
        stayButton.onClick = [this] { owner.cancelLeave(); };
        leaveButton.onClick = [this] { owner.doLeave(); };
        addAndMakeVisible (stayButton);
        addAndMakeVisible (leaveButton);
        setWantsKeyboardFocus (false);
    }

    void setQuitMode (bool q)
    {
        quit = q;
        leaveButton.setLabel (q ? TXT ("나가고 종료") : TXT ("나가기"));
        resized();
        repaint();
    }

    void mouseDown (const juce::MouseEvent&) override {}   // 뒤 화면 클릭 막기 (스크림 없음)

    void resized() override
    {
        const int w = plugin ? 380 : 460, padX = plugin ? 24 : 30, padY = plugin ? 22 : 28;
        const float titlePx = plugin ? 19.0f : 22.0f, bodyPx = plugin ? 14.0f : 15.0f;
        const int titleH = (int) std::ceil (Fonts::get (700, titlePx).getHeight());
        const int bodyH = (int) std::ceil (paragraphHeight (bodyText(), Fonts::get (400, bodyPx), (float) (w - padX * 2), bodyPx * 1.6f));
        const int btnH = plugin ? 40 : 44;
        const int h = padY * 2 + titleH + (plugin ? 10 : 12) + bodyH + (plugin ? 10 : 12) + (plugin ? 8 : 12) + btnH;
        card = juce::Rectangle<int> (getWidth() / 2 - w / 2, getHeight() / 2 - h / 2, w, h);
        const int leaveW = leaveButton.getIdealWidth (plugin ? 18 : 22), stayW = stayButton.getIdealWidth (plugin ? 18 : 22);
        const int by = card.getBottom() - padY - btnH;
        leaveButton.setBounds (card.getRight() - padX - leaveW, by, leaveW, btnH);
        stayButton.setBounds (leaveButton.getX() - (plugin ? 8 : 10) - stayW, by, stayW, btnH);
    }

    void paint (juce::Graphics& g) override
    {
        const int padX = plugin ? 24 : 30, padY = plugin ? 22 : 28;
        const float titlePx = plugin ? 19.0f : 22.0f, bodyPx = plugin ? 14.0f : 15.0f;
        g.setColour (col::white);
        g.fillRoundedRectangle (card.toFloat(), (float) metric::radiusWindow);
        g.setColour (col::ink);
        g.drawRoundedRectangle (card.toFloat().reduced (0.5f), (float) metric::radiusWindow, 1.0f);

        const int titleH = (int) std::ceil (Fonts::get (700, titlePx).getHeight());
        g.setFont (Fonts::get (700, titlePx));
        g.drawText (quit ? TXT ("방을 나가고 종료할까요") : TXT ("방을 나갈까요"), juce::Rectangle<int> (card.getX() + padX, card.getY() + padY, card.getWidth() - padX * 2, titleH), juce::Justification::centredLeft, false);
        drawParagraph (g, bodyText(), Fonts::get (400, bodyPx), col::inkBody,
                       juce::Rectangle<float> ((float) (card.getX() + padX), (float) (card.getY() + padY + titleH + (plugin ? 10 : 12)), (float) (card.getWidth() - padX * 2), 80.0f),
                       bodyPx * 1.6f);
    }

private:
    juce::String bodyText() const
    {
        return plugin ? TXT ("나가면 지금 합주에서 빠집니다. 플러그인은 열려 있어요.")
                      : TXT ("나가면 지금 합주에서 빠집니다. 같은 코드로 다시 들어올 수 있어요.");
    }

    JamScreen& owner;
    const bool plugin;
    MeonButton stayButton, leaveButton;
    juce::Rectangle<int> card;
    bool quit = false;
};

//==============================================================================
JamScreen::JamScreen (MeonEditor& e)
    : editor (e), plugin (e.isPluginMode()),
      copyButton (TXT ("복사")), settingsButton (TXT ("설정")), leaveButton (plugin ? TXT ("나가기") : TXT ("방 나가기"))
{
    copyButton.setFont (plugin ? 12.0f : 13.0f, 500);
    if (plugin) copyButton.setCornerRadius (5.0f);
    copyButton.onClick = [this] { copyCode(); };
    settingsButton.setFont (plugin ? 12.0f : 13.0f, 500);
    settingsButton.setColourOverride (col::white, col::inkBody, col::border, col::panel);
    settingsButton.onClick = [this] { editor.openSettings(); };
    leaveButton.setFont (plugin ? 12.0f : 13.0f, 500);
    leaveButton.setBadge ("Esc");
    leaveButton.setBadgeMetrics (plugin ? 18.0f : 20.0f, plugin ? 10.0f : 11.0f, plugin ? 7.0f : 8.0f);
    leaveButton.onClick = [this] { requestLeave(); };
    addAndMakeVisible (copyButton);
    addAndMakeVisible (settingsButton);
    addAndMakeVisible (leaveButton);

    me = std::make_unique<MyPanel> (*this);
    addAndMakeVisible (*me);
    alone = std::make_unique<AlonePanel> (*this);
    addChildComponent (*alone);
    chat = std::make_unique<ChatPanel> (*this);
    addChildComponent (*chat);
    rail = std::make_unique<ChatRail> (*this);
    addChildComponent (*rail);

    chatOpen = editor.getSettings().isChatOpen();
    editor.getSession().addListener (this);
    editor.getSession().setServerPingInterval (10000);
    rebuildCards();
    chat->refresh();
    lastTickMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (30);
}

JamScreen::~JamScreen()
{
    editor.getSession().removeListener (this);
}

bool JamScreen::effectiveChatOpen() const
{
    if (plugin && getWidth() > 0 && getWidth() < 800)
        return false;
    return chatOpen;
}

void JamScreen::setChatOpen (bool open)
{
    chatOpen = open;
    editor.getSettings().setChatOpen (open);
    if (open)
        editor.getSession().markChatRead();
    resized();
    chat->refresh();
    rail->setUnread (false);
}

void JamScreen::toggleChat()
{
    setChatOpen (! chatOpen);
}

void JamScreen::chatChanged()
{
    chat->refresh();
    if (effectiveChatOpen())
        editor.getSession().markChatRead();
    else
        rail->setUnread (editor.getSession().getUnreadCount() > 0);
}

void JamScreen::sendChatText (const juce::String& text)
{
    editor.getSession().sendChat (text);
}

void JamScreen::copyCode()
{
    editor.copyRoomCodeToClipboard();
    copyButton.setLabel (TXT ("복사됨"));
    alone->setCopied (true);
    juce::Component::SafePointer<JamScreen> safe (this);
    juce::Timer::callAfterDelay (1500, [safe]
    {
        if (safe != nullptr)
        {
            safe->copyButton.setLabel (TXT ("복사"));
            safe->alone->setCopied (false);
            safe->resized();
        }
    });
    resized();
}

void JamScreen::requestLeave()
{
    if (leaveDialog != nullptr)
        return;
    showLeaveDialog (false);
}

void JamScreen::showLeaveDialog (bool quit)
{
    quitAfterLeave = quit;
    leaveDialog = std::make_unique<LeaveDialog> (*this);
    leaveDialog->setQuitMode (quit);
    addAndMakeVisible (*leaveDialog);
    leaveDialog->setBounds (getLocalBounds());
    leaveDialog->toFront (false);
    editor.grabKeyboardFocus();
}

void JamScreen::cancelLeave()
{
    if (leaveDialog != nullptr)
    {
        leaveDialog->setVisible (false);
        dialogTrash = std::move (leaveDialog);
    }
    quitAfterLeave = false;
}

void JamScreen::doLeave()
{
    const bool quit = quitAfterLeave;
    if (leaveDialog != nullptr)
    {
        leaveDialog->setVisible (false);
        dialogTrash = std::move (leaveDialog);
    }
    editor.getSession().leaveRoom();
    if (quit)
        if (auto* app = juce::JUCEApplicationBase::getInstance())
            app->systemRequestedQuit();
}

bool JamScreen::confirmLeaveForQuit()
{
    if (! editor.getSession().isInRoom())
        return false;
    if (leaveDialog != nullptr)
        leaveDialog->setQuitMode (true), quitAfterLeave = true;
    else
        showLeaveDialog (true);
    return true;
}

bool JamScreen::handleShortcut (const juce::KeyPress& k)
{
    auto& session = editor.getSession();
    if (k == juce::KeyPress::escapeKey)
    {
        if (leaveDialog != nullptr) cancelLeave();
        else requestLeave();
        return true;
    }
    if (k.getModifiers().isCommandDown())
    {
        if (juce::CharacterFunctions::toUpperCase (k.getTextCharacter()) == 'C')
        {
            copyCode();
            return true;
        }
        return false;
    }
    if (leaveDialog != nullptr)
        return false;

    const auto ch = juce::CharacterFunctions::toUpperCase (k.getTextCharacter());
    if (ch == 'M')
    {
        session.setMyMuted (! session.isMyMuted());
        me->update();
        return true;
    }
    if (ch == 'C')
    {
        toggleChat();
        return true;
    }
    if (ch >= '1' && ch <= '4')
    {
        const int slot = (int) (ch - '1');
        if (auto* m = session.getMemberInSlot (slot))
            if (m->connected)
                session.setMemberMuted (slot, ! m->muted);
        return true;
    }
    return false;
}

void JamScreen::rebuildCards()
{
    cards.clear();
    empties.clear();
    auto members = editor.getSession().getMembers();
    for (int slot = 0; slot < metric::maxOthers; ++slot)
    {
        const MeonSession::Member* found = nullptr;
        for (auto& m : members)
            if (m.slot == slot)
                found = &m;
        if (found != nullptr)
        {
            auto* card = cards.add (new MemberCard (*this, slot));
            card->update (*found);
            addAndMakeVisible (card);
        }
        else
        {
            auto* e = empties.add (new EmptySlot (plugin));
            e->getProperties().set ("slot", slot);
            addAndMakeVisible (e);
        }
    }
    resized();
    repaint (topBar);
}

void JamScreen::updateCards()
{
    for (auto* card : cards)
        if (auto* m = editor.getSession().getMemberInSlot (card->getSlot()))
            card->update (*m);
    me->update();
    repaint (topBar);
}

void JamScreen::timerCallback()
{
    dialogTrash = nullptr;
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double dt = juce::jlimit (0.0, 0.2, (now - lastTickMs) * 0.001);
    lastTickMs = now;
    auto& session = editor.getSession();
    me->pushLevel (session.getMySendLevelDb(), dt, now);
    for (auto* card : cards)
        card->pushLevel (session.getMemberLevelDb (card->getSlot()), dt);
}

void JamScreen::resized()
{
    auto r = getLocalBounds();
    const int topH = plugin ? 60 - 12 : 60;
    topBar = r.removeFromTop (topH);
    const int padX = plugin ? 14 : 20;

    // 상단 바 버튼들
    const int leaveW = leaveButton.getIdealWidth (plugin ? 12 : 14), setW = settingsButton.getIdealWidth (plugin ? 12 : 14);
    const int btnH = plugin ? 30 : 34;
    leaveButton.setBounds (topBar.getRight() - padX - leaveW, topBar.getCentreY() - btnH / 2, leaveW, btnH);
    settingsButton.setBounds (leaveButton.getX() - (plugin ? 10 : 12) - setW, topBar.getCentreY() - btnH / 2, setW, btnH);

    const float logoPx = plugin ? 15.0f : 17.0f;
    int x = topBar.getX() + padX + (int) std::ceil (logoWidth (logoPx, 0.18f)) + (plugin ? 14 : 20) + 1 + (plugin ? 14 : 20);
    if (! plugin)
        x += (int) std::ceil (Fonts::get (500, 13.0f).getStringWidthFloat (TXT ("초대 코드"))) + 12;
    const float codePx = plugin ? 17.0f : 22.0f;
    x += (int) std::ceil (spacedTextWidth (editor.getSession().getDisplayRoomCode(), Fonts::get (700, codePx), codePx, plugin ? 0.08f : 0.1f)) + (plugin ? 14 : 12);
    const int copyH = plugin ? 26 : 30;
    copyButton.setBounds (x, topBar.getCentreY() - copyH / 2, copyButton.getIdealWidth (plugin ? 10 : 12), copyH);

    // 오른쪽: 채팅 패널 또는 레일
    const bool open = effectiveChatOpen();
    chat->setVisible (open);
    rail->setVisible (! open);
    if (open)
        chat->setBounds (r.removeFromRight (plugin ? 240 : 320));
    else
        rail->setBounds (r.removeFromRight (plugin ? 46 : 56));

    auto main = r.reduced (plugin ? 14 : 20);
    const int gap = plugin ? 12 : 16;
    const int meH = plugin ? 12 * 2 + 42 : 18 * 2 + 52;
    me->setBounds (main.removeFromTop (meH));
    main.removeFromTop (gap);

    const bool isAlone = editor.getSession().isAlone();
    alone->setVisible (isAlone);
    for (auto* c : cards) c->setVisible (! isAlone);
    for (auto* e : empties) e->setVisible (! isAlone);
    if (isAlone)
    {
        alone->setBounds (main);
    }
    else
    {
        const int cw = (main.getWidth() - gap) / 2, chh = (main.getHeight() - gap) / 2;
        auto slotBounds = [&] (int slot)
        {
            return juce::Rectangle<int> (main.getX() + (slot % 2) * (cw + gap), main.getY() + (slot / 2) * (chh + gap), cw, chh);
        };
        for (auto* c : cards) c->setBounds (slotBounds (c->getSlot()));
        for (auto* e : empties) e->setBounds (slotBounds ((int) e->getProperties()["slot"]));
    }

    if (leaveDialog != nullptr)
        leaveDialog->setBounds (getLocalBounds());
}

void JamScreen::paint (juce::Graphics& g)
{
    ScreenBase::paint (g);
    auto& session = editor.getSession();
    g.setColour (col::cardBorder);
    g.fillRect (topBar.getX(), topBar.getBottom() - 1, topBar.getWidth(), 1);

    const int padX = plugin ? 14 : 20;
    const float logoPx = plugin ? 15.0f : 17.0f;
    float x = (float) (topBar.getX() + padX);
    drawLogo (g, juce::Rectangle<float> (x, (float) topBar.getY(), logoWidth (logoPx, 0.18f) + 4.0f, (float) topBar.getHeight()), logoPx, 0.18f, juce::Justification::centredLeft);
    x += logoWidth (logoPx, 0.18f) + (plugin ? 14.0f : 20.0f);
    g.setColour (col::cardBorder);
    g.fillRect (juce::Rectangle<float> (x, (float) topBar.getCentreY() - (plugin ? 10.0f : 12.0f), 1.0f, plugin ? 20.0f : 24.0f));
    x += 1.0f + (plugin ? 14.0f : 20.0f);
    if (! plugin)
    {
        auto f = Fonts::get (500, 13.0f);
        g.setFont (f);
        g.setColour (col::inkSub);
        const float w = f.getStringWidthFloat (TXT ("초대 코드"));
        g.drawText (TXT ("초대 코드"), juce::Rectangle<float> (x, (float) topBar.getY(), w + 4.0f, (float) topBar.getHeight()), juce::Justification::centredLeft, false);
        x += w + 12.0f;
    }
    const float codePx = plugin ? 17.0f : 22.0f;
    drawSpacedText (g, session.getDisplayRoomCode(), Fonts::get (700, codePx), codePx, plugin ? 0.08f : 0.1f, col::ink,
                    juce::Rectangle<float> (x, (float) topBar.getY(), 400.0f, (float) topBar.getHeight()), juce::Justification::centredLeft);

    // 오른쪽 상태: 서버 핑, 멤버 수
    const float ping = session.getServerPingMs();
    juce::String pingText = (plugin ? juce::String() : TXT ("서버 ")) + (ping >= 0.0f ? juce::String ((int) std::lround (ping)) + " ms" : TXT ("— ms"));
    juce::String countText = TXT ("멤버 ") + juce::String (session.getMemberCount()) + " / " + juce::String (metric::maxMembers);
    auto f = Fonts::get (500, plugin ? 12.0f : 13.0f);
    g.setFont (f);
    float rx = (float) settingsButton.getX() - (plugin ? 10.0f : 12.0f);
    if (! plugin)
    {
        g.setColour (col::cardBorder);
        g.fillRect (juce::Rectangle<float> (rx - 1.0f, (float) topBar.getCentreY() - 12.0f, 1.0f, 24.0f));
        rx -= 1.0f + 12.0f;
    }
    const float cw = f.getStringWidthFloat (countText);
    g.setColour (col::inkSub);
    g.drawText (countText, juce::Rectangle<float> (rx - cw, (float) topBar.getY(), cw + 4.0f, (float) topBar.getHeight()), juce::Justification::centredLeft, false);
    rx -= cw + (plugin ? 10.0f : 12.0f);
    const float pw = f.getStringWidthFloat (pingText);
    g.drawText (pingText, juce::Rectangle<float> (rx - pw, (float) topBar.getY(), pw + 4.0f, (float) topBar.getHeight()), juce::Justification::centredLeft, false);
    const float dot = plugin ? 6.0f : 7.0f;
    g.setColour (session.isServerConnected() ? col::accent : col::disabled);
    g.fillEllipse (rx - pw - (plugin ? 6.0f : 7.0f) - dot, (float) topBar.getCentreY() - dot * 0.5f, dot, dot);
}

} // namespace meon
