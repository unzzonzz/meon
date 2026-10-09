#include "JamScreen.h"
#include "MeonParts.h"
#include "MeonEmoji.h"

namespace meon
{

//==============================================================================
/** 채팅 글 속 링크 (http://, https://, www. 로 시작해서 공백·이모지 전까지). 끝의 문장부호는 뺀다. */
static TextRanges findLinks (const juce::String& text)
{
    TextRanges links;
    const int n = text.length();
    auto startsWith = [&text] (int i, const char* prefix)
    {
        return text.substring (i, i + (int) std::strlen (prefix)).equalsIgnoreCase (prefix);
    };

    for (int i = 0; i < n;)
    {
        int prefix = 0;
        if (startsWith (i, "https://"))     prefix = 8;
        else if (startsWith (i, "http://")) prefix = 7;
        else if (startsWith (i, "www.") && (i == 0 || ! (juce::CharacterFunctions::isLetterOrDigit (text[i - 1]) || text[i - 1] == '.' || text[i - 1] == '/')))
            prefix = 4;
        if (prefix == 0)
        {
            ++i;
            continue;
        }

        int end = i + prefix;
        while (end < n && ! juce::CharacterFunctions::isWhitespace (text[end])
               && text[end] != '<' && text[end] != '>' && text[end] != '"' && emoji::clusterEnd (text, end) == end)
            ++end;

        // 끝에 붙은 문장부호 ("…봐요 https://a.com." 의 마침표 등). 괄호는 짝이 안 맞을 때만 뺀다.
        const juce::String trailing (TXT (".,!?;:'\")]}…"));
        while (end > i + prefix && trailing.containsChar (text[end - 1]))
        {
            if (text[end - 1] == ')')
            {
                const auto body = text.substring (i, end);
                if (body.retainCharacters ("(").length() >= body.retainCharacters (")").length())
                    break;
            }
            --end;
        }

        const auto rest = text.substring (i + prefix, end);
        if (rest.isNotEmpty() && (prefix != 4 || rest.containsChar ('.')))
            links.push_back ({ i, end });
        i = juce::jmax (end, i + 1);
    }
    return links;
}

/** 브라우저에 넘길 주소. www. 는 https:// 를 붙이고, 한글 등은 퍼센트 인코딩한다 (macOS 는 ASCII 주소만 연다). */
static juce::String browserUrl (const juce::String& link)
{
    const auto full = link.startsWithIgnoreCase ("www.") ? "https://" + link : link;
    const juce::String safe ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._~:/?#[]@!$&'()*+,;=%");
    juce::String out;
    for (auto p = full.getCharPointer(); ! p.isEmpty();)
    {
        const auto c = p.getAndAdvance();
        if (c < 0x80 && safe.containsChar (c))
        {
            out += juce::String::charToString (c);
            continue;
        }
        const auto ch = juce::String::charToString (c);
        for (auto* b = ch.toRawUTF8(); *b != 0; ++b)
            out += "%" + juce::String::toHexString ((int) (juce::uint8) *b).paddedLeft ('0', 2).toUpperCase();
    }
    return out;
}

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

        // 파트 아이콘 + 닉네임 / 아래 줄 "나 · 파트" (닉네임 첫 글자에 맞춰 들여씀)
        auto& session = owner.getEditor().getSession();
        const Part part = session.getMyPart();
        auto inner = getLocalBounds().reduced (plugin ? 14 : 20, plugin ? 12 : 18);
        auto nameFont = Fonts::get (700, plugin ? 18.0f : 24.0f);
        auto subFont = Fonts::get (500, plugin ? 12.0f : 13.0f);
        const int nameH = (int) std::ceil (nameFont.getHeight()), subH = (int) std::ceil (subFont.getHeight());
        const int gap = plugin ? 2 : 4;
        const int boxW = plugin ? 96 : 150;
        const float iconSize = plugin ? 22.0f : 28.0f;
        const int indent = part == Part::None ? 0 : (int) iconSize + (plugin ? 6 : 8);
        const int y = inner.getCentreY() - (nameH + gap + subH) / 2;
        parts::drawIcon (g, { (float) inner.getX(), (float) y, iconSize, (float) nameH }, iconSize, part, col::ink);
        g.setColour (col::ink);
        g.setFont (nameFont);
        g.drawText (session.getDisplayName(), juce::Rectangle<int> (inner.getX() + indent, y, boxW - indent, nameH), juce::Justification::centredLeft, true);
        g.setColour (col::inkSub);
        g.setFont (subFont);
        const juce::String sub = part == Part::None ? TXT ("나") : TXT ("나 · ") + parts::label (part);
        g.drawText (sub, juce::Rectangle<int> (inner.getX() + indent, y + nameH + gap, boxW - indent, subH), juce::Justification::centredLeft, true);

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
    MemberCard (JamScreen& o, int slotIn) : owner (o), plugin (o.isPlugin()), slot (slotIn), muteButton (TXT ("뮤트")), resetButton (TXT ("0 dB로"))
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
                updateResetButton();
                repaint (volTextArea);
            }
        };
        resetButton.setFont (plugin ? 12.0f : 13.0f, 500);
        resetButton.onClick = [this]
        {
            if (! haveMember || ! member.connected)
                return;
            owner.getEditor().getSession().setMemberGain (slot, 1.0f);
            member.gain = 1.0f;
            syncing = true;
            volume.setValue (1.0, juce::dontSendNotification);
            syncing = false;
            updateResetButton();
            repaint (volTextArea);
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
        addAndMakeVisible (resetButton);
    }

    int getSlot() const { return slot; }

    /** 이미 0.0 dB 이거나 끊긴 멤버면 비활성 (#AAAAAA) */
    void updateResetButton()
    {
        const bool atZero = std::abs (juce::Decibels::gainToDecibels (volume.getValue(), -100.0)) < 0.05;
        resetButton.setEnabled (haveMember && member.connected && ! atZero);
    }

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
        updateResetButton();
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
        const int headerH = headerHeight();
        const int muteH = plugin ? 30 : 38;
        const int resetH = plugin ? 26 : 30;
        const int meterH = plugin ? 10 : 12, volH = resetH;
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
        const int rowGap = plugin ? 10 : 14;
        // 볼륨 줄: [볼륨] 슬라이더 · 값 · "0 dB로"
        const int resetW = resetButton.getIdealWidth (plugin ? 8 : 10);
        resetButton.setBounds (r.getRight() - resetW, volY, resetW, resetH);
        const int textRight = resetButton.getX() - (plugin ? 8 : 10);
        volume.setBounds (r.getX() + labelW, volY + (volH - 16) / 2, textRight - textW - rowGap - (r.getX() + labelW), 16);
        volTextArea = juce::Rectangle<int> (textRight - textW, volY, textW, volH);
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
        const int headerH = (int) std::ceil (nameFont.getHeight());   // 이름 줄 (핑도 이 줄 가운데)
        const juce::Colour ink = off ? col::disabled : col::ink;

        // 이름 줄: 파트 아이콘(끊기면 회색) + 닉네임. 앱은 아래 줄에 파트 이름, 플러그인은 같은 줄에
        const float iconSize = plugin ? 22.0f : 28.0f;
        const int iconGap = plugin ? 6 : 8;
        const bool hasPart = member.part != Part::None;
        parts::drawIcon (g, { (float) inner.getX(), (float) inner.getY(), iconSize, (float) headerH }, iconSize, member.part,
                         off ? col::disabled : col::ink);
        const int nameX = inner.getX() + (hasPart ? (int) iconSize + iconGap : 0);
        const int maxNameW = inner.getWidth() / 2;
        const int nameW = juce::jmin (maxNameW, (int) std::ceil (nameFont.getStringWidthFloat (member.displayName)) + 2);
        g.setColour (ink);
        g.setFont (nameFont);
        g.drawText (member.displayName, juce::Rectangle<int> (nameX, inner.getY(), maxNameW, headerH), juce::Justification::centredLeft, true);
        if (hasPart)
        {
            auto partFont = Fonts::get (500, plugin ? 12.0f : 13.0f);
            g.setColour (col::inkSub);
            g.setFont (partFont);
            if (plugin)
                g.drawText (parts::label (member.part), juce::Rectangle<int> (nameX + nameW + 6, inner.getY(), 80, headerH), juce::Justification::centredLeft, false);
            else
                g.drawText (parts::label (member.part), juce::Rectangle<int> (nameX, inner.getY() + headerH + 4, maxNameW, (int) std::ceil (partFont.getHeight())),
                            juce::Justification::centredLeft, false);
        }

        // 핑 (오른쪽). 18 ms 초과는 카드 테두리(노랑) + 하단 문구로만 표시 (글자에는 노랑 계열을 쓰지 않는다)
        juce::String pingText = (off || ! member.hasStats) ? TXT ("— ms") : juce::String ((int) std::lround (member.pingMs)) + " ms";
        auto pingFont = Fonts::get (600, plugin ? 13.0f : 15.0f);
        g.setFont (pingFont);
        g.setColour (off ? col::disabled : col::inkSub);
        const int pingW = (int) std::ceil (pingFont.getStringWidthFloat (pingText));
        g.drawText (pingText, juce::Rectangle<int> (inner.getRight() - pingW, inner.getY(), pingW, headerH), juce::Justification::centredRight, false);

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
        if (off && member.joinFailed)   { status = TXT ("연결 실패 · 네트워크 확인 필요"); }
        else if (off)                   { status = TXT ("연결 끊김 · 재연결 중"); }
        else if (member.pending)        { status = TXT ("연결 중…"); }
        else if (warn)                  { status = TXT ("지연이 조금 있어요"); }
        else if (member.muted)          { status = TXT ("소리 꺼짐"); }
        if (status.isNotEmpty())
        {
            g.setFont (Fonts::get (500, plugin ? 12.0f : 13.0f));
            g.setColour (col::inkSub);
            g.drawText (status, statusArea, juce::Justification::centredLeft, true);
        }
    }

private:
    JamScreen& owner;
    const bool plugin;
    const int slot;
    LevelBar meter;
    VolumeSlider volume;
    MeonButton muteButton, resetButton;
    MeonSession::Member member;
    bool haveMember = false, syncing = false;
    juce::Rectangle<int> volTextArea, volLabelArea, statusArea;

    /** 머리 영역 높이: 앱은 이름 줄 + 4 + 파트 이름 줄, 플러그인은 이름 줄 하나 */
    int headerHeight() const
    {
        const int nameH = (int) std::ceil (Fonts::get (700, plugin ? 18.0f : 24.0f).getHeight());
        return plugin ? nameH : nameH + 4 + (int) std::ceil (Fonts::get (500, 13.0f).getHeight());
    }
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
        explicit List (JamScreen& o) : owner (o), plugin (o.isPlugin()) { setInterceptsMouseClicks (true, false); }   // 휠·트랙패드 스크롤이 Viewport 로 가려면 목록이 마우스를 받아야 한다

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
            const int textH = (int) std::ceil (paragraphHeightWithEmoji (m.text, Fonts::get (400, msgPx), (float) width, msgPx * 1.5f));
            return nameH + (plugin ? 2 : 3) + textH;
        }

        void paint (juce::Graphics& g) override
        {
            forEachItem ([&] (const MeonSession::ChatMessage& m, juce::Rectangle<int> item)
            {
                if (m.kind == MeonSession::ChatMessage::System)
                {
                    drawLineWithEmoji (g, koreanTime (m.time) + TXT (" · ") + m.text, Fonts::get (400, plugin ? 11.0f : 12.0f), col::disabled,
                                       item.toFloat(), juce::Justification::centred);
                    return false;
                }
                const bool mine = m.kind == MeonSession::ChatMessage::Mine;
                const int nameH = (int) std::ceil (Fonts::get (600, plugin ? 12.0f : 13.0f).getHeight());
                drawLineWithEmoji (g, mine ? TXT ("나") : m.from, Fonts::get (600, plugin ? 12.0f : 13.0f), col::inkSub,
                                   item.withHeight (nameH).toFloat(), mine ? juce::Justification::centredRight : juce::Justification::centredLeft);
                const float msgPx = plugin ? 13.0f : 15.0f;
                drawParagraphWithEmoji (g, m.text, Fonts::get (400, msgPx), col::ink, textArea (m, item), msgPx * 1.5f, textJustification (m),
                                        findLinks (m.text), col::inkSub);   // 링크 글자색은 inkSub (디자인 토큰)
                return false;
            });
        }

        void mouseMove (const juce::MouseEvent& e) override
        {
            setMouseCursor (linkAt (e.position).isNotEmpty() ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        }

        void mouseExit (const juce::MouseEvent&) override { setMouseCursor (juce::MouseCursor::NormalCursor); }

        void mouseUp (const juce::MouseEvent& e) override
        {
            if (e.mouseWasDraggedSinceMouseDown() || e.mods.isPopupMenu())
                return;
            if (auto link = linkAt (e.position); link.isNotEmpty())
            {
                setMouseCursor (juce::MouseCursor::NormalCursor);
                owner.requestOpenLink (link);
            }
        }

    private:
        /** 메시지마다 fn (메시지, 그 메시지 영역) 을 부른다. fn 이 true 를 돌려주면 멈춘다. */
        template <typename Fn>
        void forEachItem (Fn&& fn) const
        {
            const int pad = plugin ? 12 : 16, gap = plugin ? 11 : 14;
            const int width = getWidth() - pad * 2;
            int y = pad;
            for (auto& m : owner.getEditor().getSession().getChat())
            {
                const int h = itemHeight (m, width);
                if (fn (m, juce::Rectangle<int> (pad, y, width, h)))
                    return;
                y += h + gap;
            }
        }

        juce::Rectangle<float> textArea (const MeonSession::ChatMessage&, juce::Rectangle<int> item) const
        {
            const int nameH = (int) std::ceil (Fonts::get (600, plugin ? 12.0f : 13.0f).getHeight());
            return juce::Rectangle<float> ((float) item.getX(), (float) (item.getY() + nameH + (plugin ? 2 : 3)), (float) item.getWidth(), (float) (item.getHeight() - nameH));
        }

        static juce::Justification textJustification (const MeonSession::ChatMessage& m)
        {
            return m.kind == MeonSession::ChatMessage::Mine ? juce::Justification::topRight : juce::Justification::topLeft;
        }

        /** p 위에 있는 링크 글 (없으면 빈 문자열) */
        juce::String linkAt (juce::Point<float> p) const
        {
            juce::String found;
            forEachItem ([&] (const MeonSession::ChatMessage& m, juce::Rectangle<int> item)
            {
                if (item.getY() > p.y)
                    return true;
                if (m.kind == MeonSession::ChatMessage::System || ! item.toFloat().contains (p))
                    return false;
                const auto links = findLinks (m.text);
                const float msgPx = plugin ? 13.0f : 15.0f;
                const int k = hitTestParagraphLink (m.text, Fonts::get (400, msgPx), textArea (m, item), msgPx * 1.5f, textJustification (m), links, p);
                if (k >= 0)
                    found = m.text.substring (links[(size_t) k].getStart(), links[(size_t) k].getEnd());
                return true;
            });
            return found;
        }

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
        input.grabKeyboardFocus();   // 전송 버튼을 눌러 보냈어도 이어서 입력할 수 있게
    }

    JamScreen& owner;
    const bool plugin;
    List list;
    juce::Viewport viewport;
    MeonButton collapseButton, sendButton;
    MeonTextEditor input;
};

//==============================================================================
/** 채팅을 접었을 때 멤버 아래에 나오는 최신 메시지 한 줄. 누르면 채팅이 열린다 (마우스로 여는 유일한 곳, 단축키 C). */
class JamScreen::ChatPeek : public juce::Button
{
public:
    explicit ChatPeek (JamScreen& o) : juce::Button ("chatPeek"), owner (o), plugin (o.isPlugin())
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setWantsKeyboardFocus (false);
        onClick = [this] { owner.toggleChat(); };
    }

    static int height (bool plugin) { return plugin ? 36 : 44; }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour ((over || down) && ! plugin ? juce::Colour (0xFFEEEEEEu) : col::panel);
        g.fillRoundedRectangle (r, (float) metric::radiusCard);
        g.setColour (col::cardBorder);
        g.drawRoundedRectangle (r.reduced (0.5f), (float) metric::radiusCard, 1.0f);

        const float padX = plugin ? 12.0f : 16.0f, gap = plugin ? 10.0f : 12.0f;
        auto inner = r.reduced (padX, 0.0f);

        // 시스템 메시지는 건너뛰고 가장 최근 대화 한 줄
        const MeonSession::ChatMessage* last = nullptr;
        const auto& msgs = owner.getEditor().getSession().getChat();
        for (auto it = msgs.rbegin(); it != msgs.rend(); ++it)
            if (it->kind != MeonSession::ChatMessage::System) { last = &*it; break; }

        // 오른쪽: 시간 (· 눌러서 채팅 열기)
        auto hintFont = Fonts::get (400, plugin ? 11.0f : 12.0f);
        juce::String hint = last != nullptr ? relativeTime (last->time) : juce::String();
        if (! plugin)
            hint = hint.isEmpty() ? TXT ("눌러서 채팅 열기") : hint + TXT (" · 눌러서 채팅 열기");
        if (hint.isNotEmpty())
        {
            const float hw = hintFont.getStringWidthFloat (hint) + 2.0f;
            g.setColour (col::disabled);
            g.setFont (hintFont);
            g.drawText (hint, inner.removeFromRight (hw), juce::Justification::centredRight, false);
            inner.removeFromRight (gap);
        }

        if (last == nullptr)
        {
            g.setColour (col::disabled);
            g.setFont (Fonts::get (400, plugin ? 14.0f : 15.0f));
            g.drawText (TXT ("아직 채팅이 없어요"), inner, juce::Justification::centredLeft, true);
            return;
        }

        auto nameFont = Fonts::get (600, plugin ? 13.0f : 14.0f);
        const juce::String name = last->kind == MeonSession::ChatMessage::Mine ? TXT ("나") : last->from;
        const float nw = juce::jmin (lineWidthWithEmoji (name, nameFont) + 2.0f, inner.getWidth() * 0.4f);
        drawLineWithEmoji (g, name, nameFont, col::inkSub, inner.removeFromLeft (nw));
        inner.removeFromLeft (gap);

        drawLineWithEmoji (g, last->text, Fonts::get (400, plugin ? 14.0f : 15.0f), col::ink, inner);   // 넘치면 말줄임
    }

private:
    static juce::String relativeTime (const juce::Time& t)
    {
        const auto mins = (int) ((juce::Time::getCurrentTime() - t).inMinutes());
        if (mins < 1)  return TXT ("방금");
        if (mins < 60) return juce::String (mins) + TXT ("분 전");
        return juce::String (mins / 60) + TXT ("시간 전");
    }

    JamScreen& owner;
    const bool plugin;
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

    void mouseDown (const juce::MouseEvent&) override {}   // 뒤 화면 클릭 막기

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
        // 스크림: 창 전체를 #1A1A1A 25% 로 덮는다 (반투명 없음 규칙의 유일한 예외)
        g.fillAll (col::ink.withAlpha (0.25f));
        g.setColour (col::white);   // 테두리 없음. 스크림만으로 화면과 구분한다
        g.fillRoundedRectangle (card.toFloat(), (float) metric::radiusWindow);
        g.setColour (col::ink);

        const int titleH = (int) std::ceil (Fonts::get (700, titlePx).getHeight());
        g.setFont (Fonts::get (700, titlePx));
        g.drawText (quit ? TXT ("방을 나가고 종료할까요") : TXT ("방을 나갈까요"), juce::Rectangle<int> (card.getX() + padX, card.getY() + padY, card.getWidth() - padX * 2, titleH), juce::Justification::centredLeft, false);
        drawParagraph (g, bodyText(), Fonts::get (400, bodyPx), col::inkSub,
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
/** 채팅 링크를 누르면 나오는 확인 창. 열기를 누르면 기본 브라우저로 연다. */
class JamScreen::LinkDialog : public juce::Component
{
public:
    LinkDialog (JamScreen& o, const juce::String& linkText)
        : owner (o), plugin (o.isPlugin()), link (linkText),
          cancelButton (TXT ("취소")), openButton (TXT ("열기"), MeonButton::Style::Primary)
    {
        // 너무 긴 주소는 앞부분만 보인다 (여는 주소는 그대로)
        shown = link.length() > 300 ? link.substring (0, 300) + juce::String::charToString ((juce::juce_wchar) 0x2026) : link;
        cancelButton.setFont (plugin ? 15.0f : 16.0f, 500);
        openButton.setFont (plugin ? 15.0f : 16.0f, 600);
        cancelButton.onClick = [this] { owner.cancelLink(); };
        openButton.onClick = [this] { owner.openLink(); };
        addAndMakeVisible (cancelButton);
        addAndMakeVisible (openButton);
        setWantsKeyboardFocus (false);
    }

    const juce::String& getLink() const { return link; }

    void mouseDown (const juce::MouseEvent&) override {}   // 뒤 화면 클릭 막기

    void resized() override
    {
        const int w = juce::jmin (plugin ? 380 : 460, getWidth() - 32), padX = plugin ? 24 : 30, padY = plugin ? 22 : 28;
        const float titlePx = plugin ? 19.0f : 22.0f;
        const int titleH = (int) std::ceil (Fonts::get (700, titlePx).getHeight());
        const int btnH = plugin ? 40 : 44;
        const int h = padY * 2 + titleH + (plugin ? 10 : 12) + bodyHeight (w - padX * 2) + (plugin ? 10 : 12) + (plugin ? 8 : 12) + btnH;
        card = juce::Rectangle<int> (getWidth() / 2 - w / 2, getHeight() / 2 - h / 2, w, h);
        const int openW = openButton.getIdealWidth (plugin ? 18 : 22), cancelW = cancelButton.getIdealWidth (plugin ? 18 : 22);
        const int by = card.getBottom() - padY - btnH;
        openButton.setBounds (card.getRight() - padX - openW, by, openW, btnH);
        cancelButton.setBounds (openButton.getX() - (plugin ? 8 : 10) - cancelW, by, cancelW, btnH);
    }

    void paint (juce::Graphics& g) override
    {
        const int padX = plugin ? 24 : 30, padY = plugin ? 22 : 28;
        const float titlePx = plugin ? 19.0f : 22.0f, bodyPx = plugin ? 14.0f : 15.0f;
        g.fillAll (col::ink.withAlpha (0.25f));   // 나가기 확인 창과 같은 스크림
        g.setColour (col::white);
        g.fillRoundedRectangle (card.toFloat(), (float) metric::radiusWindow);
        g.setColour (col::ink);

        const int titleH = (int) std::ceil (Fonts::get (700, titlePx).getHeight());
        g.setFont (Fonts::get (700, titlePx));
        g.drawText (TXT ("링크를 열까요"), juce::Rectangle<int> (card.getX() + padX, card.getY() + padY, card.getWidth() - padX * 2, titleH), juce::Justification::centredLeft, false);
        const float bodyW = (float) (card.getWidth() - padX * 2);
        drawParagraphWithEmoji (g, shown, Fonts::get (400, bodyPx), col::inkSub,
                                juce::Rectangle<float> ((float) (card.getX() + padX), (float) (card.getY() + padY + titleH + (plugin ? 10 : 12)), bodyW, (float) bodyHeight ((int) bodyW)),
                                bodyPx * 1.6f);
    }

private:
    int bodyHeight (int width) const
    {
        const float bodyPx = plugin ? 14.0f : 15.0f;
        return (int) std::ceil (paragraphHeightWithEmoji (shown, Fonts::get (400, bodyPx), (float) width, bodyPx * 1.6f));
    }

    JamScreen& owner;
    const bool plugin;
    const juce::String link;
    juce::String shown;
    MeonButton cancelButton, openButton;
    juce::Rectangle<int> card;
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
    settingsButton.setColourOverride (col::white, col::inkSub, col::border, col::panel);
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
    chatPeek = std::make_unique<ChatPeek> (*this);
    addChildComponent (*chatPeek);

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

bool JamScreen::isNarrow() const
{
    return plugin && getWidth() > 0 && getWidth() < 800;
}

bool JamScreen::effectiveChatOpen() const
{
    // 플러그인 창 폭이 800 미만이면 자동으로 접힌다. 그 상태에서 직접 열면 넓어질 때까지만 열어 둔다.
    if (isNarrow())
        return narrowChatOpen;
    return chatOpen;
}

void JamScreen::setChatOpen (bool open)
{
    if (isNarrow())
    {
        narrowChatOpen = open;
    }
    else
    {
        chatOpen = open;
        editor.getSettings().setChatOpen (open);
    }
    if (effectiveChatOpen())
        editor.getSession().markChatRead();
    resized();
    chat->refresh();
    chatPeek->repaint();
}

void JamScreen::toggleChat()
{
    setChatOpen (! effectiveChatOpen());
}

void JamScreen::chatChanged()
{
    chat->refresh();
    if (effectiveChatOpen())
        editor.getSession().markChatRead();
    chatPeek->repaint();
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
    cancelLink();
    showLeaveDialog (false);
}

void JamScreen::requestOpenLink (const juce::String& link)
{
    if (leaveDialog != nullptr || linkDialog != nullptr)
        return;
    linkDialog = std::make_unique<LinkDialog> (*this, link);
    addAndMakeVisible (*linkDialog);
    linkDialog->setBounds (getLocalBounds());
    linkDialog->toFront (false);
    editor.grabKeyboardFocus();   // Esc·Enter 를 handleShortcut 이 받도록
}

void JamScreen::cancelLink()
{
    if (linkDialog != nullptr)
    {
        linkDialog->setVisible (false);
        dialogTrash = std::move (linkDialog);
    }
}

void JamScreen::openLink()
{
    if (linkDialog == nullptr)
        return;
    const auto url = browserUrl (linkDialog->getLink());
    cancelLink();
    juce::URL (url).launchInDefaultBrowser();
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
    if (linkDialog != nullptr)
    {
        if (k == juce::KeyPress::escapeKey) cancelLink();
        else if (k == juce::KeyPress::returnKey) openLink();
        return true;   // 확인 창이 떠 있는 동안 다른 단축키는 막는다
    }
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
    if (chatPeek->isVisible() && now - lastPeekRepaintMs > 15000.0)   // "방금 / N분 전" 갱신
    {
        lastPeekRepaintMs = now;
        chatPeek->repaint();
    }
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

    // 채팅을 접으면 멤버 아래에 최신 메시지 한 줄 (상단 바에는 채팅 버튼 없음)
    if (isNarrow() != narrowSeen) { narrowSeen = isNarrow(); narrowChatOpen = false; }   // 폭 기준을 넘나들면 다시 자동 접힘
    const bool open = effectiveChatOpen();
    chatPeek->setVisible (! open);

    const float logoH = plugin ? 26.0f : 30.0f;
    int x = topBar.getX() + padX + (int) std::ceil (logoWidth (logoH)) + (plugin ? 14 : 20) + 1 + (plugin ? 14 : 20);
    if (! plugin)
        x += (int) std::ceil (Fonts::get (500, 13.0f).getStringWidthFloat (TXT ("초대 코드"))) + 12;
    const float codePx = plugin ? 17.0f : 22.0f;
    x += (int) std::ceil (spacedTextWidth (editor.getSession().getDisplayRoomCode(), Fonts::get (700, codePx), codePx, plugin ? 0.08f : 0.1f)) + (plugin ? 14 : 12);
    const int copyH = plugin ? 26 : 30;
    copyButton.setBounds (x, topBar.getCentreY() - copyH / 2, copyButton.getIdealWidth (plugin ? 10 : 12), copyH);

    // 오른쪽: 채팅 패널 (접으면 완전히 사라지고 멤버 영역이 전체 폭을 쓴다)
    chat->setVisible (open);
    if (open)
        chat->setBounds (r.removeFromRight (plugin ? 240 : 320));

    auto main = r.reduced (plugin ? 14 : 20);
    const int gap = plugin ? 12 : 16;
    if (! open)
    {
        chatPeek->setBounds (main.removeFromBottom (ChatPeek::height (plugin)));
        main.removeFromBottom (gap);
    }
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
    if (linkDialog != nullptr)
        linkDialog->setBounds (getLocalBounds());
}

bool JamScreen::serverTextHidden() const
{
    return plugin && getWidth() < 800;
}

juce::String JamScreen::getTooltip()
{
    if (serverTextHidden() && serverStatusArea.expanded (4).contains (getMouseXYRelative()))
        return editor.getSession().getServerStatusText (false);
    return {};
}

void JamScreen::paint (juce::Graphics& g)
{
    ScreenBase::paint (g);
    auto& session = editor.getSession();
    g.setColour (col::cardBorder);
    g.fillRect (topBar.getX(), topBar.getBottom() - 1, topBar.getWidth(), 1);

    const int padX = plugin ? 14 : 20;
    const float logoH = plugin ? 26.0f : 30.0f;
    float x = (float) (topBar.getX() + padX);
    drawLogo (g, juce::Rectangle<float> (x, (float) topBar.getY(), logoWidth (logoH), (float) topBar.getHeight()), logoH, col::ink, juce::Justification::centredLeft);
    x += logoWidth (logoH) + (plugin ? 14.0f : 20.0f);
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

    // 오른쪽 상태: 서버 연결 상태, 멤버 수 (서버 왕복 시간은 합주 지연이 아니므로 숫자를 보이지 않는다)
    const juce::String srvText = session.getServerStatusText (false);
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
    // 플러그인 창이 800 미만이면 점만 남기고 문구는 툴팁으로
    const bool dotOnly = serverTextHidden();
    const float sw = dotOnly ? 0.0f : f.getStringWidthFloat (srvText);
    if (! dotOnly)
        g.drawText (srvText, juce::Rectangle<float> (rx - sw, (float) topBar.getY(), sw + 4.0f, (float) topBar.getHeight()), juce::Justification::centredLeft, false);
    const float dot = plugin ? 6.0f : 7.0f;
    const float dotX = rx - sw - (dotOnly ? 0.0f : (plugin ? 6.0f : 7.0f)) - dot;
    g.setColour (col::accent);   // 점은 상태와 관계없이 포인트 색 고정 (디자인 결정)
    g.fillEllipse (dotX, (float) topBar.getCentreY() - dot * 0.5f, dot, dot);
    serverStatusArea = juce::Rectangle<float> (dotX, (float) topBar.getY(), rx - dotX, (float) topBar.getHeight()).toNearestInt();
}

} // namespace meon
