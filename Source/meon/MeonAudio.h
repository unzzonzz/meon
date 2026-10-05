// MEON - 독립 앱 오디오 장치 도우미 (첫 실행 오디오 화면과 설정 화면이 같이 쓴다)
#pragma once

#include <JuceHeader.h>

namespace meon
{
namespace audio
{
    /** Windows: 쓸 수 있는 드라이버 종류 (ASIO → Windows 저지연 → 단독 사용 → 일반 순).
        다른 OS 에서는 비어 있다 (드라이버 고르기를 보여 주지 않는다). */
    juce::StringArray driverTypes (juce::AudioDeviceManager&);

    /** 드라이버 종류 이름을 화면에 보여 줄 한국어 이름으로. */
    juce::String driverLabel (const juce::String& typeName);

    /** 드라이버 종류를 바꾸고 그 드라이버의 기본 장치를 연다. */
    void setDriverType (juce::AudioDeviceManager&, const juce::String& typeName);

    /** Windows: 처음 실행할 때의 드라이버. ASIO 장치가 있으면 ASIO, 없으면 Windows 저지연. */
    juce::String preferredDriverType (juce::AudioDeviceManager&);

    /** 입력·출력 장치를 바꾼다. 입력과 출력을 따로 고를 수 없는 드라이버(ASIO)는 둘을 같은 장치로 맞춘다. */
    void setDevices (juce::AudioDeviceManager&, const juce::String& input, const juce::String& output);

    /** 버퍼 크기 선택지 네 개: 64/128/256/512 에 가장 가까운, 지금 장치가 실제로 지원하는 크기.
        장치가 없으면 64/128/256/512 그대로. */
    juce::Array<int> bufferChoices (juce::AudioDeviceManager&);

    /** 지금 장치에 실제로 적용된 버퍼 크기 (장치가 없으면 설정값). */
    int currentBufferSize (juce::AudioDeviceManager&);

    void setBufferSize (juce::AudioDeviceManager&, int size);

    /** 버퍼 크기를 장치 드라이버의 설정 창에서만 바꿀 수 있는지 (ASIO 는 보통 그렇다). */
    bool hasDriverPanel (juce::AudioDeviceManager&);
    void showDriverPanel (juce::AudioDeviceManager&);
}
}
