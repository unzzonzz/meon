// MEON - 로컬 사용자 설정 (닉네임, 첫 실행 완료 여부, 입력 채널). 독립 앱과 플러그인이 같은 파일을 공유한다.
#pragma once

#include <JuceHeader.h>

namespace meon
{

class MeonSettings
{
public:
    MeonSettings();
    ~MeonSettings();

    juce::String getNickname() const;
    void setNickname (const juce::String& name);

    /** 첫 실행 흐름을 끝냈는지. 독립 앱과 플러그인은 따로 기록한다 (헤드폰 확인 단계가 다르므로). */
    bool isOnboardingDone (bool plugin) const;
    void setOnboardingDone (bool plugin, bool done);

    /** 독립 앱 입력 채널 (0 기반 시작 채널, 채널 수 1 또는 2) */
    int getInputChannelStart() const;
    int getInputChannelCount() const;
    void setInputChannels (int start, int count);

    bool isChatOpen() const;
    void setChatOpen (bool open);

    void save();

    static juce::File getSettingsFolder();
    static juce::File getLogFolder();

    /** 닉네임 규칙: 한글·영문·숫자 2–12자 */
    static bool isValidNickname (const juce::String& name);

private:
    std::unique_ptr<juce::PropertiesFile> props;
};

} // namespace meon
