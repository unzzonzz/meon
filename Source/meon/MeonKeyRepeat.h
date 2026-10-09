// MEON - 플러그인에서 키를 꾹 누를 때 반복 입력
// Logic 같은 호스트는 자동 반복 키(isARepeat)를 플러그인 창에 넘겨주지 않는다. 그래서 글자를 꾹 눌러도 한 번만 들어간다.
// 반복 이벤트가 앱까지는 오면 그걸 입력창에 직접 넘기고, 아예 안 오면 keyUp 이 올 때까지
// 시스템 설정의 반복 지연·간격대로 같은 키 이벤트를 만들어 보낸다.
// 키 이벤트를 그대로 보내므로 한글 입력기(조합)도 실제 반복과 똑같이 처리된다.
#pragma once

#include <JuceHeader.h>

namespace meon
{

struct KeyRepeat
{
   #if JUCE_MAC
    /** 입력창이 키 입력을 받을 때마다 호출. 지금 처리 중인 키가 새로 눌린 키면 반복을 준비한다. (플러그인만 동작) */
    static void noteKeyEvent (juce::Component& target);
    /** 반복을 멈춘다 (키를 뗐을 때, 포커스를 잃었을 때) */
    static void stop();
   #else
    static void noteKeyEvent (juce::Component&) {}
    static void stop() {}
   #endif
};

} // namespace meon
