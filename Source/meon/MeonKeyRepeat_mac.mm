// MEON - 플러그인에서 키를 꾹 누를 때 반복 입력 (macOS). 설명은 MeonKeyRepeat.h
#import <Cocoa/Cocoa.h>   // JuceHeader 의 using namespace juce 보다 먼저 (Point 등 이름 충돌)
#include "MeonKeyRepeat.h"

#if JUCE_MAC

namespace meon
{

namespace
{
    class Repeater : private juce::Timer
    {
    public:
        static Repeater& get() { static Repeater r; return r; }

        void note (juce::Component& c)
        {
            if (synthesizing || juce::JUCEApplicationBase::isStandaloneApp())   // 독립 앱은 OS 가 반복을 보내준다
                return;

            NSEvent* ev = [NSApp currentEvent];
            if (ev == nil || [ev type] != NSEventTypeKeyDown)
                return;

            if ([ev isARepeat])   // 호스트가 반복을 넘겨주고 있다 → 만들 필요 없음
            {
                stop();
                return;
            }

            if (last != nil && [ev timestamp] == [last timestamp] && [ev keyCode] == [last keyCode])
                return;   // 같은 키 이벤트로 여러 번 불림

            stop();

            const auto key = [ev keyCode];
            const bool command = ([ev modifierFlags] & (NSEventModifierFlagCommand | NSEventModifierFlagControl)) != 0;
            // Return(36)·키패드 Enter(76)·Esc(53)·Tab(48) 은 반복하지 않는다 (전송·나가기·포커스 이동)
            if (command || key == 36 || key == 76 || key == 53 || key == 48)
                return;

            last = [ev retain];
            target = &c;
            startedMs = juce::Time::getMillisecondCounter();
            // 키를 뗀 순간을 알려면 앱으로 들어오는 키 이벤트를 직접 본다.
            // (Logic 은 keyUp 을 플러그인에 안 넘길 수 있고, CGEventSourceKeyState 는 입력 모니터링 권한이 없으면 늘 false)
            monitor = [NSEvent addLocalMonitorForEventsMatchingMask: NSEventMaskKeyDown | NSEventMaskKeyUp
                                                           handler: ^NSEvent* (NSEvent* e) { return Repeater::get().onEvent (e); }];
            startTimer (juce::roundToInt ([NSEvent keyRepeatDelay] * 1000.0) + 60);   // 진짜 반복이 오면 그쪽이 먼저 오도록 약간 늦게
        }

        void stop()
        {
            stopTimer();
            if (monitor != nil)
            {
                [NSEvent removeMonitor: monitor];
                monitor = nil;
            }
            if (last != nil)
            {
                [last release];
                last = nil;
            }
            target = nullptr;
        }

        ~Repeater() override { stop(); }   // 플러그인이 내려갈 때 모니터를 남기지 않는다

        NSEvent* onEvent (NSEvent* e)
        {
            if (last == nil || [e keyCode] != [last keyCode])
                return e;

            if ([e type] == NSEventTypeKeyUp)
            {
                stop();
                return e;
            }

            if ([e isARepeat])
            {
                // 진짜 반복이 앱까지는 온다 → 만들지 않고 이걸 입력창에 직접 넘긴다 (호스트가 가로채지 않게 소비)
                stopTimer();
                send (e);
                return nil;
            }
            return e;
        }

    private:
        NSEvent* last = nil;
        id monitor = nil;
        juce::uint32 startedMs = 0;
        juce::Component::SafePointer<juce::Component> target;
        bool synthesizing = false;

        void send (NSEvent* e)
        {
            auto* c = target.getComponent();
            if (auto* peer = c != nullptr ? c->getPeer() : nullptr)
            {
                const juce::ScopedValueSetter<bool> svs (synthesizing, true);
                [(NSView*) peer->getNativeHandle() keyDown: e];
            }
        }

        void timerCallback() override
        {
            auto* c = target.getComponent();
            auto* peer = c != nullptr ? c->getPeer() : nullptr;
            // keyUp 을 끝내 못 보는 경우를 대비해 30초가 지나면 멈춘다
            if (last == nil || peer == nullptr || ! c->hasKeyboardFocus (false)
                || juce::Time::getMillisecondCounter() - startedMs > 30000)
            {
                stop();
                return;
            }

            NSView* view = (NSView*) peer->getNativeHandle();
            NSEvent* rep = [NSEvent keyEventWithType: NSEventTypeKeyDown
                                            location: [last locationInWindow]
                                       modifierFlags: [last modifierFlags]
                                           timestamp: [[NSProcessInfo processInfo] systemUptime]
                                        windowNumber: [[view window] windowNumber]
                                             context: nil
                                          characters: [last characters]
                         charactersIgnoringModifiers: [last charactersIgnoringModifiers]
                                           isARepeat: YES
                                             keyCode: [last keyCode]];
            if (rep == nil)
            {
                stop();
                return;
            }

            send (rep);

            const int interval = juce::jmax (15, juce::roundToInt ([NSEvent keyRepeatInterval] * 1000.0));
            if (getTimerInterval() != interval)
                startTimer (interval);
        }
    };
}

void KeyRepeat::noteKeyEvent (juce::Component& target) { Repeater::get().note (target); }
void KeyRepeat::stop()                                   { Repeater::get().stop(); }

} // namespace meon

#endif
