#include "SettingsScreen.h"

namespace meon
{

//==============================================================================
SettingsScreen::LinkLabel::LinkLabel (const juce::String& t, const juce::String& u, float p)
    : juce::Button (t), text (t), url (u), px (p)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus (false);
    onClick = [this] { juce::URL (url).launchInDefaultBrowser(); };
}

int SettingsScreen::LinkLabel::getIdealWidth() const
{
    return (int) std::ceil (Fonts::get (400, px).getStringWidthFloat (text)) + 2;
}

void SettingsScreen::LinkLabel::paintButton (juce::Graphics& g, bool over, bool)
{
    auto font = Fonts::get (400, px);
    g.setFont (font);
    g.setColour (over ? col::accentDown : col::accent);
    g.drawText (text, getLocalBounds(), juce::Justification::centredLeft, false);
    if (over)
        g.fillRect (0, getHeight() - 2, getIdealWidth() - 2, 1);
}

//==============================================================================
SettingsScreen::Content::Content (SettingsScreen& o)
    : owner (o), plugin (o.plugin),
      nickInput (o.plugin ? 14.0f : 15.0f),
      saveButton (TXT ("저장")), logButton (TXT ("로그 폴더 열기")),
      licenseLink (o.plugin ? TXT ("전문 보기") : TXT ("라이선스 전문 보기"), "https://www.gnu.org/licenses/gpl-3.0.html", o.plugin ? 13.0f : 14.0f),
      sourceLink (TXT ("소스 코드"), "https://github.com/unzzonzz/meon", 14.0f)
{
    meter.setCornerRadius (3.0f);
    nickInput.setIndents (12, 0);
    nickInput.setInputRestrictions (12);
    nickInput.setText (owner.editor.getSettings().getNickname(), juce::dontSendNotification);
    nickInput.onReturnKey = [this] { saveNickname(); };
    saveButton.setFont (plugin ? 13.0f : 14.0f, 500);
    saveButton.onClick = [this] { saveNickname(); };
    logButton.setFont (plugin ? 13.0f : 14.0f, 500);
    logButton.onClick = []
    {
        auto folder = MeonSettings::getLogFolder();
        folder.createDirectory();
        folder.revealToUser();
    };

    if (! plugin)
    {
        addAndMakeVisible (inCombo);
        addAndMakeVisible (outCombo);
        addAndMakeVisible (bufCombo);
        addAndMakeVisible (meter);
        inCombo.onChange = [this] { applyDevices(); };
        outCombo.onChange = [this] { applyDevices(); };
        bufCombo.onChange = [this] { applyBuffer(); };
    }
    addAndMakeVisible (nickInput);
    addAndMakeVisible (saveButton);
    addAndMakeVisible (logButton);
    addAndMakeVisible (licenseLink);
    if (! plugin)
        addAndMakeVisible (sourceLink);
    refreshDevices();
}

void SettingsScreen::Content::refreshDevices()
{
    if (plugin)
        return;
    auto* dm = owner.editor.deviceManager();
    if (dm == nullptr)
        return;
    updating = true;
    auto* type = dm->getCurrentDeviceTypeObject();
    auto setup = dm->getAudioDeviceSetup();
    const juce::String none = TXT ("장치를 찾을 수 없음");

    auto fill = [&none] (juce::ComboBox& box, const juce::StringArray& names, const juce::String& current)
    {
        box.clear (juce::dontSendNotification);
        box.setEnabled (! names.isEmpty());
        box.setTextWhenNothingSelected (none);
        for (int i = 0; i < names.size(); ++i)
            box.addItem (names[i], i + 1);
        const int idx = names.indexOf (current);
        if (idx >= 0)
            box.setSelectedId (idx + 1, juce::dontSendNotification);
    };
    fill (inCombo, type != nullptr ? type->getDeviceNames (true) : juce::StringArray(), setup.inputDeviceName);
    fill (outCombo, type != nullptr ? type->getDeviceNames (false) : juce::StringArray(), setup.outputDeviceName);

    bufCombo.clear (juce::dontSendNotification);
    bufferChoices.clear();
    const double sr = setup.sampleRate > 0.0 ? setup.sampleRate : 48000.0;
    int current = setup.bufferSize;
    if (auto* dev = dm->getCurrentAudioDevice())
    {
        current = dev->getCurrentBufferSizeSamples();
        for (int size : dev->getAvailableBufferSizes())
            if (size >= 32 && size <= 1024)
                bufferChoices.add (size);
    }
    if (bufferChoices.isEmpty())
        bufferChoices = { 64, 128, 256, 512 };
    for (int i = 0; i < bufferChoices.size(); ++i)
        bufCombo.addItem (juce::String (bufferChoices[i]) + TXT (" 샘플 · ") + juce::String (bufferChoices[i] / sr * 1000.0, 1) + " ms", i + 1);
    const int bi = bufferChoices.indexOf (current);
    if (bi >= 0)
        bufCombo.setSelectedId (bi + 1, juce::dontSendNotification);
    bufCombo.setEnabled (dm->getCurrentAudioDevice() != nullptr);
    updating = false;
}

void SettingsScreen::Content::applyDevices()
{
    if (updating || plugin)
        return;
    auto* dm = owner.editor.deviceManager();
    if (dm == nullptr)
        return;
    auto setup = dm->getAudioDeviceSetup();
    if (inCombo.getSelectedId() > 0)  setup.inputDeviceName = inCombo.getText();
    if (outCombo.getSelectedId() > 0) setup.outputDeviceName = outCombo.getText();
    setup.useDefaultInputChannels = true;
    setup.useDefaultOutputChannels = true;
    dm->setAudioDeviceSetup (setup, true);   // 합주 중에도 즉시 적용 (스트림 재시작)
    if (owner.editor.saveSettingsIfNeeded)
        owner.editor.saveSettingsIfNeeded();
    refreshDevices();
}

void SettingsScreen::Content::applyBuffer()
{
    if (updating || plugin)
        return;
    auto* dm = owner.editor.deviceManager();
    const int idx = bufCombo.getSelectedId() - 1;
    if (dm == nullptr || idx < 0 || idx >= bufferChoices.size())
        return;
    auto setup = dm->getAudioDeviceSetup();
    setup.bufferSize = bufferChoices[idx];
    dm->setAudioDeviceSetup (setup, true);
    if (owner.editor.saveSettingsIfNeeded)
        owner.editor.saveSettingsIfNeeded();
    refreshDevices();
}

void SettingsScreen::Content::saveNickname()
{
    auto name = nickInput.getText().trim();
    if (! MeonSettings::isValidNickname (name))
    {
        nickInput.setText (owner.editor.getSettings().getNickname(), juce::dontSendNotification);
        return;
    }
    owner.editor.getSettings().setNickname (name);
    owner.editor.getSession().applyNicknameChange();
    saveButton.setLabel (TXT ("저장됨"));
    juce::Component::SafePointer<Content> safe (this);
    juce::Timer::callAfterDelay (1500, [safe] { if (safe != nullptr) safe->saveButton.setLabel (TXT ("저장")); });
}

void SettingsScreen::Content::layout (int width)
{
    sectionTitles.clear(); rowLabels.clear(); infoRows.clear(); texts.clear(); subTexts.clear(); dividers.clear();
    const int labelW = plugin ? 100 : 120;
    const int ctrlH = plugin ? 38 : 40;
    const float titlePx = plugin ? 14.0f : 15.0f;
    const int titleH = (int) std::ceil (Fonts::get (700, titlePx).getHeight());
    const int sectionGap = plugin ? 24 : 32;
    const int titleGap = plugin ? 12 : 14;
    int y = 0;

    auto sectionTitle = [&] (const juce::String& t)
    {
        sectionTitles.push_back ({ t, juce::Rectangle<int> (0, y, width, titleH) });
        y += titleH + titleGap;
    };
    auto divider = [&]
    {
        y += sectionGap;
        dividers.push_back (juce::Rectangle<int> (0, y, width, 1));
        y += 1 + sectionGap;
    };

    if (! plugin)
    {
        sectionTitle (TXT ("오디오 장치"));
        auto row = [&] (const juce::String& label, juce::Component& c)
        {
            rowLabels.push_back ({ label, juce::Rectangle<int> (0, y, labelW, ctrlH) });
            c.setBounds (labelW, y, width - labelW, ctrlH);
            y += ctrlH + 12;
        };
        row (TXT ("입력"), inCombo);
        row (TXT ("출력"), outCombo);
        row (TXT ("버퍼 크기"), bufCombo);
        rowLabels.push_back ({ TXT ("입력 레벨"), juce::Rectangle<int> (0, y, labelW, ctrlH) });
        meter.setBounds (labelW, y + ctrlH / 2 - 5, width - labelW - 12 - 56, 10);
        levelTextArea = juce::Rectangle<int> (width - 56, y, 56, ctrlH);
        y += ctrlH;
        divider();
    }

    sectionTitle (TXT ("닉네임"));
    {
        const int saveW = saveButton.getIdealWidth (plugin ? 16 : 18);
        nickInput.setBounds (0, y, width - saveW - (plugin ? 8 : 10), ctrlH);
        saveButton.setBounds (width - saveW, y, saveW, ctrlH);
        y += ctrlH;
        if (! plugin)
        {
            y += 14;
            subTexts.push_back ({ TXT ("합주 중에 바꾸면 다음 입장부터 적용돼요"), juce::Rectangle<int> (0, y, width, 16) });
            y += 16;
        }
    }
    divider();

    if (plugin)
    {
        sectionTitle (TXT ("오디오"));
        audioPanel = juce::Rectangle<int> (0, y, width, 14 * 2 + 18 + 6 + 16);
        y += audioPanel.getHeight();
        divider();
    }

    sectionTitle (TXT ("문제 해결"));
    {
        const int btnW = logButton.getIdealWidth (plugin ? 16 : 18);
        texts.push_back ({ plugin ? TXT ("연결이나 소리에 문제가 있을 때") : TXT ("연결이나 소리에 문제가 있을 때 로그를 보내주세요"), juce::Rectangle<int> (0, y, width - btnW - 16, 18) });
        subTexts.push_back ({ MeonSettings::getLogFolder().getFullPathName(), juce::Rectangle<int> (0, y + 18 + (plugin ? 3 : 4), width - btnW - 16, 16) });
        logButton.setBounds (width - btnW, y + (18 + 4 + 16) / 2 - ctrlH / 2, btnW, ctrlH);
        y += 18 + 4 + 16;
    }
    divider();

    sectionTitle (TXT ("앱 정보"));
    {
        const float px = plugin ? 13.0f : 14.0f;
        const int rowH = (int) std::ceil (Fonts::get (400, px).getHeight());
        const int rowGap = plugin ? 7 : 8;
        infoRows.push_back ({ TXT ("버전"), juce::Rectangle<int> (0, y, width, rowH) });
        y += rowH + rowGap;
        infoRows.push_back ({ TXT ("기반"), juce::Rectangle<int> (0, y, width, rowH) });
        y += rowH + rowGap;
        infoRows.push_back ({ TXT ("라이선스"), juce::Rectangle<int> (0, y, width, rowH) });
        auto font = Fonts::get (400, px);
        const juce::String prefix = plugin ? TXT ("GPLv3 · ") : TXT ("GPLv3 — 소스 코드 공개 의무가 적용됩니다. ");
        int lx = labelW + (int) std::ceil (font.getStringWidthFloat (prefix));
        licenseLink.setBounds (lx, y, licenseLink.getIdealWidth(), rowH);
        if (! plugin)
        {
            lx = licenseLink.getRight() + (int) std::ceil (font.getStringWidthFloat (TXT (" · ")));
            sourceLink.setBounds (lx, y, sourceLink.getIdealWidth(), rowH);
        }
        y += rowH;
    }
    setSize (width, y);
}

void SettingsScreen::Content::paint (juce::Graphics& g)
{
    const float titlePx = plugin ? 14.0f : 15.0f;
    const int labelW = plugin ? 100 : 120;
    for (auto& t : sectionTitles)
        drawSpacedText (g, t.first, Fonts::get (700, titlePx), titlePx, plugin ? 0.0f : 0.02f, col::ink, t.second.toFloat(), juce::Justification::centredLeft);
    g.setColour (col::cardBorder);
    for (auto& d : dividers)
        g.fillRect (d);
    g.setFont (Fonts::get (400, 14.0f));
    g.setColour (col::inkSub);
    for (auto& r : rowLabels)
        g.drawText (r.first, r.second, juce::Justification::centredLeft, false);
    if (! plugin)
    {
        g.setFont (Fonts::get (500, 13.0f));
        const float db = meter.getShownDb();
        g.drawText (db <= -60.0f ? juce::String ("-60 dB") : juce::String ((int) std::lround (db)) + " dB", levelTextArea, juce::Justification::centredRight, false);
    }
    g.setColour (col::ink);
    g.setFont (Fonts::get (400, plugin ? 14.0f : 15.0f));
    for (auto& t : texts)
        g.drawText (t.first, t.second, juce::Justification::centredLeft, true);
    g.setColour (col::disabled);
    g.setFont (Fonts::get (400, plugin ? 12.0f : 13.0f));
    for (auto& t : subTexts)
        g.drawText (t.first, t.second, juce::Justification::centredLeft, true);

    if (plugin && ! audioPanel.isEmpty())
    {
        g.setColour (col::panel);
        g.fillRoundedRectangle (audioPanel.toFloat(), (float) metric::radiusCard);
        g.setColour (col::cardBorder);
        g.drawRoundedRectangle (audioPanel.toFloat().reduced (0.5f), (float) metric::radiusCard, 1.0f);
        auto inner = audioPanel.reduced (16, 14);
        g.setColour (col::inkBody);
        g.setFont (Fonts::get (400, 14.0f));
        g.drawText (TXT ("장치와 버퍼는 DAW 설정을 따릅니다"), inner.removeFromTop (18), juce::Justification::centredLeft, false);
        inner.removeFromTop (6);
        const double sr = owner.editor.getProcessor().getSampleRate();
        const int bs = owner.editor.getProcessor().getBlockSize();
        const double ms = sr > 0.0 ? bs / sr * 1000.0 : 0.0;
        g.setColour (col::inkSub);
        g.setFont (Fonts::get (400, 13.0f));
        g.drawText (owner.editor.getHostDescription() + TXT (" · ") + juce::String (sr / 1000.0, (sr >= 1000.0 && std::fmod (sr, 1000.0) == 0.0) ? 0 : 1) + " kHz"
                    + TXT (" · ") + juce::String (bs) + TXT (" 샘플") + " (" + juce::String (ms, 1) + " ms)",
                    inner.removeFromTop (16), juce::Justification::centredLeft, false);
    }

    // 앱 정보
    const float px = plugin ? 13.0f : 14.0f;
    auto font = Fonts::get (400, px);
    g.setFont (font);
    int i = 0;
    for (auto& r : infoRows)
    {
        g.setColour (col::inkSub);
        g.drawText (r.first, r.second.withWidth (labelW), juce::Justification::centredLeft, false);
        g.setColour (col::inkBody);
        juce::String value;
        if (i == 0)      value = owner.editor.getVersionText();
        else if (i == 1) value = TXT ("SonoBus 기반 수정판");
        else             value = plugin ? TXT ("GPLv3 · ") : TXT ("GPLv3 — 소스 코드 공개 의무가 적용됩니다. ");
        g.drawText (value, r.second.withTrimmedLeft (labelW), juce::Justification::centredLeft, false);
        if (i == 2 && ! plugin)
            g.drawText (TXT (" · "), juce::Rectangle<int> (licenseLink.getRight(), r.second.getY(), sourceLink.getX() - licenseLink.getRight(), r.second.getHeight()), juce::Justification::centredLeft, false);
        ++i;
    }
}

//==============================================================================
SettingsScreen::SettingsScreen (MeonEditor& e)
    : editor (e), plugin (e.isPluginMode()), closeButton (TXT ("닫기")), content (*this)
{
    closeButton.setFont (plugin ? 12.0f : 13.0f, 500);
    closeButton.onClick = [this] { editor.closeSettings(); };
    addAndMakeVisible (closeButton);
    viewport.setViewedComponent (&content, false);
    viewport.setScrollBarsShown (true, false, false, false);
    viewport.setScrollBarThickness (8);
    addAndMakeVisible (viewport);
    if (auto* dm = editor.deviceManager())
        dm->addChangeListener (this);
    lastTickMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (30);
}

SettingsScreen::~SettingsScreen()
{
    if (auto* dm = editor.deviceManager())
        dm->removeChangeListener (this);
}

void SettingsScreen::changeListenerCallback (juce::ChangeBroadcaster*)
{
    content.refreshDevices();
}

void SettingsScreen::timerCallback()
{
    if (plugin)
        return;
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double dt = juce::jlimit (0.0, 0.2, (now - lastTickMs) * 0.001);
    lastTickMs = now;
    const int ch = editor.getSettings().getInputChannelStart();
    float db = editor.getSession().getMyInputLevelDb (ch);
    if (editor.getSettings().getInputChannelCount() == 2)
        db = juce::jmax (db, editor.getSession().getMyInputLevelDb (ch + 1));
    content.pushLevel (db, dt);
}

bool SettingsScreen::handleShortcut (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey)
    {
        editor.closeSettings();
        return true;
    }
    return false;
}

void SettingsScreen::resized()
{
    auto r = getLocalBounds();
    const int headerH = plugin ? 48 : 60;
    auto header = r.removeFromTop (headerH);
    const int cw = closeButton.getIdealWidth (plugin ? 14 : 16);
    closeButton.setBounds (header.getRight() - (plugin ? 14 : 20) - cw, header.getCentreY() - (plugin ? 15 : 17), cw, plugin ? 30 : 34);

    const int padY = plugin ? 28 : 36;
    const int w = plugin ? 560 : 720;
    content.layout (w);
    content.setSize (w, content.getHeight() + padY);   // 아래 여백
    r.removeFromTop (padY);                            // 위 여백
    viewport.setBounds (r.getCentreX() - w / 2, r.getY(), w + 12, r.getHeight());
}

void SettingsScreen::paint (juce::Graphics& g)
{
    ScreenBase::paint (g);
    const int headerH = plugin ? 48 : 60;
    g.setColour (col::cardBorder);
    g.fillRect (0, headerH - 1, getWidth(), 1);
    g.setColour (col::ink);
    g.setFont (Fonts::get (700, plugin ? 17.0f : 20.0f));
    g.drawText (TXT ("설정"), juce::Rectangle<int> (plugin ? 14 : 20, 0, 200, headerH), juce::Justification::centredLeft, false);
}

} // namespace meon
