#include "LobbyScreens.h"

namespace meon
{

//==============================================================================
HomeScreen::HomeScreen (MeonEditor& e)
    : editor (e), plugin (e.isPluginMode()),
      settingsButton (TXT ("설정")),
      createButton (TXT ("방 만들기"), MeonButton::Style::Primary),
      joinButton (TXT ("코드로 입장"), MeonButton::Style::Secondary)
{
    settingsButton.setFont (13.0f, 500);
    settingsButton.setColourOverride (col::white, col::inkBody, col::border, col::panel);
    if (! plugin)
    {
        settingsButton.setBadge (MeonEditor::cmdKeyLabel (","));
        settingsButton.setBadgeMetrics (20.0f, 11.0f, 8.0f);
    }
    settingsButton.onClick = [this] { editor.openSettings(); };

    createButton.setFont (plugin ? 19.0f : 22.0f, 700);
    createButton.setSubtitleFont (plugin ? 13.0f : 14.0f, 400);
    createButton.setSubtitle (plugin ? TXT ("초대 코드를 받아요") : TXT ("초대 코드를 받아 친구에게 보내요"));
    createButton.setCornerRadius (8.0f);
    createButton.onClick = [this]
    {
        editor.getSession().createRoom();
        editor.go (MeonEditor::Screen::Create);
    };

    joinButton.setFont (plugin ? 19.0f : 22.0f, 700);
    joinButton.setSubtitleFont (plugin ? 13.0f : 14.0f, 400);
    joinButton.setSubtitle (plugin ? TXT ("6자리 코드 입력") : TXT ("받은 6자리 코드를 입력해요"));
    joinButton.setCornerRadius (8.0f);
    joinButton.onClick = [this] { editor.go (MeonEditor::Screen::Join); };

    addAndMakeVisible (settingsButton);
    addAndMakeVisible (createButton);
    addAndMakeVisible (joinButton);

    editor.getSession().addListener (this);
    editor.getSession().setServerPingInterval (5000);
    updateState();
    startTimer (1000);
}

HomeScreen::~HomeScreen()
{
    editor.getSession().removeListener (this);
}

void HomeScreen::updateState()
{
    const bool ok = editor.getSession().isServerConnected();
    createButton.setEnabled (ok);
    joinButton.setEnabled (ok);
    repaint();
}

void HomeScreen::resized()
{
    auto r = getLocalBounds();
    const int sbW = settingsButton.getIdealWidth (plugin ? 13 : 14);
    settingsButton.setBounds (r.getRight() - (plugin ? 18 : 24) - sbW, plugin ? 16 : 20, sbW, plugin ? 32 : 34);

    const int footerH = (plugin ? 16 : 16) + (plugin ? 20 : 28);
    footer = r.removeFromBottom (footerH);
    centre = r;

    const float logoPx = plugin ? 34.0f : 44.0f;
    const int logoH = (int) std::ceil (Fonts::get (700, logoPx).getHeight());
    const int greetH = (int) std::ceil (Fonts::get (400, plugin ? 14.0f : 16.0f).getHeight());
    const int btnW = plugin ? 220 : 260, btnH = plugin ? 84 : 96, gap = plugin ? 14 : 16;
    const int contentH = logoH + (plugin ? 10 : 14) + greetH + (plugin ? 40 : 52) + btnH;
    const int top = centre.getY() + (centre.getHeight() - contentH) / 2;
    const int btnY = top + logoH + (plugin ? 10 : 14) + greetH + (plugin ? 40 : 52);
    const int cx = centre.getCentreX();
    createButton.setBounds (cx - gap / 2 - btnW, btnY, btnW, btnH);
    joinButton.setBounds (cx + gap / 2, btnY, btnW, btnH);
}

void HomeScreen::paint (juce::Graphics& g)
{
    ScreenBase::paint (g);
    auto& session = editor.getSession();

    const float logoPx = plugin ? 34.0f : 44.0f;
    const int logoH = (int) std::ceil (Fonts::get (700, logoPx).getHeight());
    const int greetH = (int) std::ceil (Fonts::get (400, plugin ? 14.0f : 16.0f).getHeight());
    const int btnH = plugin ? 84 : 96;
    const int contentH = logoH + (plugin ? 10 : 14) + greetH + (plugin ? 40 : 52) + btnH;
    const int top = centre.getY() + (centre.getHeight() - contentH) / 2;

    drawLogo (g, juce::Rectangle<float> ((float) centre.getX(), (float) top, (float) centre.getWidth(), (float) logoH), logoPx, 0.22f);

    g.setFont (Fonts::get (400, plugin ? 14.0f : 16.0f));
    g.setColour (col::inkSub);
    g.drawText (session.getDisplayName() + TXT ("님, 오늘도 좋은 합주 되세요"),
                juce::Rectangle<int> (centre.getX(), top + logoH + (plugin ? 10 : 14), centre.getWidth(), greetH), juce::Justification::centred, false);

    // 하단: 서버 상태 (참고용 핑)
    const bool ok = session.isServerConnected();
    juce::String text;
    if (ok)
    {
        const float ping = session.getServerPingMs();
        text = TXT ("서버 연결됨 · 핑 ") + (ping >= 0.0f ? juce::String ((int) std::lround (ping)) + " ms" : juce::String (TXT ("측정 중")));
        if (plugin)
        {
            const double sr = editor.getProcessor().getSampleRate();
            const int bs = editor.getProcessor().getBlockSize();
            text += TXT (" · ") + editor.getHostDescription() + " " + juce::String (sr / 1000.0, sr >= 1000.0 && std::fmod (sr, 1000.0) == 0.0 ? 0 : 1)
                    + " kHz / " + juce::String (bs) + TXT (" 샘플");
        }
    }
    else
    {
        text = session.getServerState() == MeonSession::ServerState::Connecting ? TXT ("서버에 연결 중…") : TXT ("서버에 연결 중…");
    }

    auto font = Fonts::get (400, plugin ? 12.0f : 13.0f);
    const float tw = font.getStringWidthFloat (text);
    const float dot = 7.0f;
    const float totalW = dot + 8.0f + tw;
    const float x = (float) footer.getCentreX() - totalW * 0.5f;
    const float cy = (float) footer.getY() + 8.0f;
    g.setColour (ok ? col::accent : col::disabled);
    g.fillEllipse (x, cy - dot * 0.5f, dot, dot);
    g.setColour (col::disabled);
    g.setFont (font);
    g.drawText (text, juce::Rectangle<float> (x + dot + 8.0f, (float) footer.getY(), tw + 4.0f, 16.0f), juce::Justification::centredLeft, false);
}

//==============================================================================
CreateRoomScreen::CreateRoomScreen (MeonEditor& e)
    : editor (e), plugin (e.isPluginMode()),
      homeButton (TXT ("← 홈")),
      copyButton (TXT ("코드 복사")),
      enterButton (TXT ("입장하기"), MeonButton::Style::Primary)
{
    homeButton.setFont (13.0f, 500);
    homeButton.setColourOverride (col::white, col::inkBody, col::border, col::panel);
    homeButton.onClick = [this]
    {
        editor.getSession().leaveRoom();   // 입장 전에 나가면 방은 사라진다
        editor.go (MeonEditor::Screen::Home);
    };

    copyButton.setFont (plugin ? 15.0f : 16.0f, 500);
    copyButton.setBadge (MeonEditor::cmdKeyLabel ("C"));
    copyButton.setBadgeMetrics (plugin ? 20.0f : 22.0f, 11.0f, plugin ? 9.0f : 10.0f);
    copyButton.onClick = [this] { copyCode(); };

    enterButton.setFont (plugin ? 16.0f : 17.0f, 600);
    enterButton.onClick = [this]
    {
        if (editor.getSession().isInRoom())
            editor.go (MeonEditor::Screen::Jam);
    };

    addAndMakeVisible (homeButton);
    addAndMakeVisible (copyButton);
    addAndMakeVisible (enterButton);

    note = plugin ? juce::String() : TXT ("나 포함 최대 5명까지 들어올 수 있어요");
    editor.getSession().addListener (this);
    updateState();
}

CreateRoomScreen::~CreateRoomScreen()
{
    editor.getSession().removeListener (this);
}

void CreateRoomScreen::joinFailed (MeonSession::JoinFailure)
{
    failed = true;
    note = TXT ("방을 만들지 못했어요. 홈으로 돌아가 다시 시도해 주세요");
    updateState();
}

void CreateRoomScreen::updateState()
{
    enterButton.setEnabled (editor.getSession().isInRoom());
    repaint();
}

bool CreateRoomScreen::handleShortcut (const juce::KeyPress& k)
{
    if (k.getModifiers().isCommandDown() && juce::CharacterFunctions::toUpperCase (k.getTextCharacter()) == 'C')
    {
        copyCode();
        return true;
    }
    if (k == juce::KeyPress::returnKey && editor.getSession().isInRoom())
    {
        editor.go (MeonEditor::Screen::Jam);
        return true;
    }
    return false;
}

void CreateRoomScreen::copyCode()
{
    editor.copyRoomCodeToClipboard();
    copyButton.setLabel (TXT ("복사됨"));
    juce::Component::SafePointer<CreateRoomScreen> safe (this);
    juce::Timer::callAfterDelay (1500, [safe] { if (safe != nullptr) { safe->copyButton.setLabel (TXT ("코드 복사")); safe->resized(); } });
    resized();
}

void CreateRoomScreen::resized()
{
    auto r = getLocalBounds();
    homeButton.setBounds (plugin ? 18 : 24, plugin ? 16 : 20, homeButton.getIdealWidth (plugin ? 13 : 14), plugin ? 32 : 34);

    // 세로 중앙 정렬 블록
    const float titlePx = plugin ? 24.0f : 30.0f, subPx = plugin ? 14.0f : 16.0f, codePx = plugin ? 60.0f : 80.0f;
    const int titleH = (int) std::ceil (Fonts::get (700, titlePx).getHeight());
    const int subH = (int) std::ceil (Fonts::get (400, subPx).getHeight());
    const int labelH = (int) std::ceil (Fonts::get (500, plugin ? 12.0f : 13.0f).getHeight());
    const int codeH = (int) std::ceil (codePx);   // line-height 1
    const int cardPadX = plugin ? 44 : 56, cardPadY = plugin ? 26 : 36, cardGap = plugin ? 16 : 20;
    const int copyH = plugin ? 40 : 44;
    const int cardH = cardPadY * 2 + labelH + cardGap + codeH + cardGap + copyH;
    const int codeW = (int) std::ceil (spacedTextWidth (editor.getSession().getDisplayRoomCode(), Fonts::get (700, codePx), codePx, 0.14f));
    const int cardW = juce::jmax (codeW, copyButton.getIdealWidth (plugin ? 20 : 24)) + cardPadX * 2;
    const int enterH = plugin ? 48 : 52, enterW = plugin ? 280 : 320;
    const int noteH = note.isEmpty() ? 0 : 16;
    const int contentH = titleH + (plugin ? 10 : 12) + subH + (plugin ? 28 : 44) + cardH + (plugin ? 26 : 36) + enterH + (noteH > 0 ? 16 + noteH : 0);
    const int top = r.getY() + (r.getHeight() - contentH) / 2;
    const int cx = r.getCentreX();

    titleArea = juce::Rectangle<int> (r.getX(), top, r.getWidth(), titleH + (plugin ? 10 : 12) + subH);
    const int cardY = titleArea.getBottom() + (plugin ? 28 : 44);
    card = juce::Rectangle<int> (cx - cardW / 2, cardY, cardW, cardH);
    const int copyW = copyButton.getIdealWidth (plugin ? 20 : 24);
    copyButton.setBounds (cx - copyW / 2, card.getBottom() - cardPadY - copyH, copyW, copyH);
    enterButton.setBounds (cx - enterW / 2, card.getBottom() + (plugin ? 26 : 36), enterW, enterH);
}

void CreateRoomScreen::paint (juce::Graphics& g)
{
    ScreenBase::paint (g);
    const float titlePx = plugin ? 24.0f : 30.0f, subPx = plugin ? 14.0f : 16.0f, codePx = plugin ? 60.0f : 80.0f;
    const int titleH = (int) std::ceil (Fonts::get (700, titlePx).getHeight());

    g.setColour (col::ink);
    g.setFont (Fonts::get (700, titlePx));
    g.drawText (TXT ("방이 만들어졌어요"), titleArea.withHeight (titleH), juce::Justification::centred, false);
    g.setColour (col::inkSub);
    g.setFont (Fonts::get (400, subPx));
    g.drawText (TXT ("이 코드를 함께 연주할 사람에게 보내주세요"), titleArea.withTrimmedTop (titleH + (plugin ? 10 : 12)), juce::Justification::centred, false);

    g.setColour (col::border);
    g.drawRoundedRectangle (card.toFloat().reduced (0.5f), (float) metric::radiusWindow, 1.0f);

    const int cardPadY = plugin ? 26 : 36, cardGap = plugin ? 16 : 20;
    const int labelH = (int) std::ceil (Fonts::get (500, plugin ? 12.0f : 13.0f).getHeight());
    drawSpacedText (g, TXT ("초대 코드"), Fonts::get (500, plugin ? 12.0f : 13.0f), plugin ? 12.0f : 13.0f, 0.06f, col::inkSub,
                    juce::Rectangle<float> ((float) card.getX(), (float) (card.getY() + cardPadY), (float) card.getWidth(), (float) labelH));
    const int codeY = card.getY() + cardPadY + labelH + cardGap;
    drawSpacedText (g, editor.getSession().getDisplayRoomCode(), Fonts::get (700, codePx), codePx, 0.14f, col::ink,
                    juce::Rectangle<float> ((float) card.getX(), (float) codeY, (float) card.getWidth(), codePx));

    if (note.isNotEmpty())
    {
        g.setFont (Fonts::get (400, 13.0f));
        g.setColour (failed ? col::accent : col::disabled);
        g.drawText (note, juce::Rectangle<int> (0, enterButton.getBottom() + 16, getWidth(), 16), juce::Justification::centred, false);
    }
}

//==============================================================================
JoinRoomScreen::JoinRoomScreen (MeonEditor& e)
    : editor (e), plugin (e.isPluginMode()),
      homeButton (TXT ("← 홈")),
      enterButton (TXT ("입장하기"), MeonButton::Style::Primary),
      newRoomButton (TXT ("새 방 만들기")),
      code (e.isPluginMode() ? 56 : 68, e.isPluginMode() ? 68 : 84, e.isPluginMode() ? 10 : 12, e.isPluginMode() ? 30.0f : 36.0f)
{
    homeButton.setFont (13.0f, 500);
    homeButton.setColourOverride (col::white, col::inkBody, col::border, col::panel);
    homeButton.onClick = [this] { editor.go (MeonEditor::Screen::Home); };

    enterButton.setFont (plugin ? 16.0f : 17.0f, 600);
    enterButton.setBadge ("Enter");
    enterButton.setBadgeMetrics (plugin ? 20.0f : 22.0f, 11.0f, plugin ? 9.0f : 10.0f);
    enterButton.onClick = [this] { submit(); };

    newRoomButton.setFont (plugin ? 12.0f : 13.0f, 500);
    newRoomButton.onClick = [this]
    {
        editor.getSession().createRoom();
        editor.go (MeonEditor::Screen::Create);
    };

    code.onChanged = [this] { if (state != State::Checking) state = State::Idle; updateState(); };
    code.onSubmit = [this] { submit(); };

    addAndMakeVisible (homeButton);
    addAndMakeVisible (code);
    addAndMakeVisible (enterButton);
    addChildComponent (newRoomButton);

    editor.getSession().addListener (this);
    updateState();
}

JoinRoomScreen::~JoinRoomScreen()
{
    editor.getSession().removeListener (this);
}

void JoinRoomScreen::parentHierarchyChanged()
{
    if (isShowing())
    {
        juce::Component::SafePointer<JoinRoomScreen> safe (this);
        juce::Timer::callAfterDelay (60, [safe] { if (safe != nullptr) safe->code.grabKeyboardFocus(); });
    }
}

bool JoinRoomScreen::handleShortcut (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::returnKey)
    {
        submit();
        return true;
    }
    return false;
}

void JoinRoomScreen::submit()
{
    if (! code.isComplete() || state == State::Checking || ! editor.getSession().isServerConnected())
        return;
    state = State::Checking;
    updateState();
    editor.getSession().joinRoom (code.getCode());
}

void JoinRoomScreen::joinFailed (MeonSession::JoinFailure f)
{
    switch (f)
    {
        case MeonSession::JoinFailure::InvalidCode: state = State::Wrong; break;
        case MeonSession::JoinFailure::RoomFull:    state = State::Full;  break;
        default:                                     state = State::Error; break;
    }
    updateState();
}

void JoinRoomScreen::updateState()
{
    code.setErrorState (state == State::Wrong || state == State::Full || state == State::Error);
    enterButton.setEnabled (code.isComplete() && state != State::Checking && editor.getSession().isServerConnected());
    newRoomButton.setVisible (state == State::Full);
    resized();
    repaint();
}

void JoinRoomScreen::resized()
{
    auto r = getLocalBounds();
    homeButton.setBounds (plugin ? 18 : 24, plugin ? 16 : 20, homeButton.getIdealWidth (plugin ? 13 : 14), plugin ? 32 : 34);

    const float titlePx = plugin ? 24.0f : 30.0f;
    const int titleH = (int) std::ceil (Fonts::get (700, titlePx).getHeight());
    const int subH = plugin ? 0 : (int) std::ceil (Fonts::get (400, 16.0f).getHeight()) + 12;
    const int boxesH = code.getPreferredHeight();
    const int msgH = plugin ? 56 : 64;
    const int enterH = plugin ? 48 : 52, enterW = plugin ? 280 : 320;
    const int contentH = titleH + subH + (plugin ? 28 : 44) + boxesH + (plugin ? 16 : 20) + msgH + (plugin ? 0 : 8) + enterH;
    const int top = r.getY() + (r.getHeight() - contentH) / 2;
    const int cx = r.getCentreX();

    int y = top + titleH + subH + (plugin ? 28 : 44);
    code.setBounds (cx - code.getPreferredWidth() / 2, y, code.getPreferredWidth(), boxesH);
    y += boxesH + (plugin ? 16 : 20);
    messageArea = juce::Rectangle<int> (r.getX(), y, r.getWidth(), msgH);
    y += msgH + (plugin ? 0 : 8);
    enterButton.setBounds (cx - enterW / 2, y, enterW, enterH);

    if (newRoomButton.isVisible())
    {
        // 메시지 상자 안 오른쪽에 버튼
        const float px = plugin ? 14.0f : 15.0f;
        auto font = Fonts::get (500, px);
        const int iconSz = plugin ? 18 : 20, padX = plugin ? 18 : 20, gap = plugin ? 12 : 14;
        const int textW = (int) std::ceil (font.getStringWidthFloat (TXT ("이 방은 5명이 모두 찼어요")));
        const int btnW = newRoomButton.getIdealWidth (plugin ? 11 : 12), btnH = plugin ? 28 : 30;
        const int boxW = padX * 2 + iconSz + gap + textW + gap + btnW;
        const int boxX = cx - boxW / 2;
        newRoomButton.setBounds (boxX + boxW - padX - btnW, messageArea.getCentreY() - btnH / 2, btnW, btnH);
    }
}

void JoinRoomScreen::paint (juce::Graphics& g)
{
    ScreenBase::paint (g);
    auto r = getLocalBounds();
    const float titlePx = plugin ? 24.0f : 30.0f;
    const int titleH = (int) std::ceil (Fonts::get (700, titlePx).getHeight());
    const int subH = plugin ? 0 : (int) std::ceil (Fonts::get (400, 16.0f).getHeight()) + 12;
    const int top = code.getY() - (plugin ? 28 : 44) - subH - titleH;

    g.setColour (col::ink);
    g.setFont (Fonts::get (700, titlePx));
    g.drawText (TXT ("초대 코드를 입력하세요"), juce::Rectangle<int> (r.getX(), top, r.getWidth(), titleH), juce::Justification::centred, false);
    if (! plugin)
    {
        g.setColour (col::inkSub);
        g.setFont (Fonts::get (400, 16.0f));
        g.drawText (TXT ("받은 6자리 코드를 그대로 넣어주세요"), juce::Rectangle<int> (r.getX(), top + titleH + 12, r.getWidth(), subH - 12), juce::Justification::centred, false);
    }

    // 메시지 영역 (높이 고정 — 버튼 위치가 움직이지 않는다)
    if (state == State::Idle || state == State::Checking)
    {
        g.setFont (Fonts::get (400, plugin ? 13.0f : 14.0f));
        g.setColour (state == State::Checking ? col::inkSub : col::disabled);
        const auto text = state == State::Checking ? TXT ("코드를 확인하고 있어요…")
                                                   : (plugin ? TXT ("붙여넣기도 됩니다") : TXT ("붙여넣기도 됩니다 · 소문자는 자동으로 대문자로 바뀌어요"));
        g.drawText (text, messageArea, juce::Justification::centred, false);
        return;
    }

    const float px = plugin ? 14.0f : 15.0f;
    auto font = Fonts::get (500, px);
    juce::String text = state == State::Full ? TXT ("이 방은 5명이 모두 찼어요")
                      : state == State::Wrong ? TXT ("그런 방이 없어요. 코드를 다시 확인해 주세요")
                                              : TXT ("연결에 문제가 있어요. 잠시 후 다시 시도해 주세요");
    const int iconSz = plugin ? 18 : 20, padX = plugin ? 18 : 20, padY = plugin ? 12 : 14;
    const int gap = state == State::Full ? (plugin ? 12 : 14) : (plugin ? 10 : 12);
    const int textW = (int) std::ceil (font.getStringWidthFloat (text));
    int boxW = padX * 2 + iconSz + gap + textW;
    if (state == State::Full)
        boxW += gap + newRoomButton.getWidth();
    const int boxH = padY * 2 + (int) std::ceil (font.getHeight()) + (state == State::Full ? 6 : 0);
    juce::Rectangle<float> box ((float) (messageArea.getCentreX() - boxW / 2), (float) (messageArea.getCentreY() - boxH / 2), (float) boxW, (float) boxH);
    g.setColour (col::warnBg);
    g.fillRoundedRectangle (box, (float) metric::radiusCard);
    g.setColour (col::warnBorder);
    g.drawRoundedRectangle (box.reduced (0.5f), (float) metric::radiusCard, 1.0f);

    juce::Rectangle<float> icon (box.getX() + (float) padX, box.getCentreY() - (float) iconSz * 0.5f, (float) iconSz, (float) iconSz);
    g.setColour (col::accent);
    g.fillRoundedRectangle (icon, 4.0f);
    g.setColour (col::white);
    g.setFont (Fonts::get (700, plugin ? 12.0f : 13.0f));
    g.drawText ("!", icon, juce::Justification::centred, false);

    g.setColour (col::ink);
    g.setFont (font);
    g.drawText (text, juce::Rectangle<float> (icon.getRight() + (float) gap, box.getY(), (float) textW + 4.0f, box.getHeight()), juce::Justification::centredLeft, false);
}

} // namespace meon
