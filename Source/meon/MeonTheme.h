// MEON - 디자인 토큰 (색, 글꼴, 치수)
// 값은 디자인 명세(MEON.dc.html 공통 스펙)에서 그대로 가져온다. 임의로 추가한 값은 주석에 [임의] 표시.
#pragma once

#include <JuceHeader.h>

/** 한국어 등 UTF-8 리터럴을 juce::String 으로 (MSVC 는 /utf-8 옵션 필요) */
#define TXT(s) juce::String (juce::CharPointer_UTF8 (s))

namespace meon
{

namespace col
{
    const juce::Colour accent      { 0xFFFF5A3Du };  // 포인트 (유일한 컬러)
    const juce::Colour accentDown  { 0xFFE44A2Fu };  // 포인트 눌림
    const juce::Colour warnBg      { 0xFFFFEDE9u };  // 경고 배경
    const juce::Colour warnBorder  { 0xFFF5CEC5u };  // 경고 테두리
    const juce::Colour ink         { 0xFF1A1A1Au };  // 본문 텍스트
    const juce::Colour inkSub      { 0xFF767676u };  // 보조 텍스트
    const juce::Colour inkBody     { 0xFF4A4A4Au };  // 본문(설명) 텍스트
    const juce::Colour disabled    { 0xFFAAAAAAu };  // 비활성
    const juce::Colour panel       { 0xFFF5F5F5u };  // 패널 / 비활성 카드
    const juce::Colour border      { 0xFFD5D5D5u };  // 테두리 · 입력 필드
    const juce::Colour cardBorder  { 0xFFE0E0E0u };  // 카드 테두리 · 구분선
    const juce::Colour meterTrack  { 0xFFECECECu };  // 레벨 미터 트랙
    const juce::Colour white       { 0xFFFFFFFFu };
    const juce::Colour badgeBg     { 0xFFF7F7F7u };  // 단축키 배지 배경
    const juce::Colour progressOff { 0xFFDDDDDDu };  // 진행 막대 (이후 단계)
    const juce::Colour titleBar    { 0xFFF0F0F0u };
    const juce::Colour titleInk    { 0xFF8A8A8Au };
}

namespace metric
{
    constexpr int   radiusButton = 6;    // 버튼·입력
    constexpr int   radiusCard   = 8;    // 카드
    constexpr int   radiusWindow = 10;   // 창
    constexpr int   fadeMs       = 120;  // 전환 페이드
    constexpr float pingWarnMs   = 18.0f;
    constexpr int   maxMembers   = 5;    // 나 포함
    constexpr int   maxOthers    = 4;    // 멤버 카드 수
}

/** Pretendard 글꼴 캐시. 에디터가 SharedResourcePointer 로 소유한다. */
class FontCache
{
public:
    FontCache();
    ~FontCache();

    juce::Typeface::Ptr get (int weight) const;
    static FontCache* instance();

private:
    juce::Typeface::Ptr regular, medium, semiBold, bold;
};

struct Fonts
{
    /** weight: 400 / 500 / 600 / 700,  px: CSS font-size (em 높이) */
    static juce::Font get (int weight, float px);
};

/** 여러 줄 문단을 CSS line-height 처럼 그린다 (lineHeightPx = 줄 간격 픽셀). */
void drawParagraph (juce::Graphics& g, const juce::String& text, const juce::Font& font, juce::Colour colour,
                    juce::Rectangle<float> area, float lineHeightPx,
                    juce::Justification justification = juce::Justification::topLeft);

/** 문단이 차지할 높이를 계산한다. */
float paragraphHeight (const juce::String& text, const juce::Font& font, float width, float lineHeightPx);

/** 텍스트 폭 */
float textWidth (const juce::Font& font, const juce::String& text);

/** MEON 로고 텍스트 (자간 넓게, O 만 포인트 색). fontPx 는 글자 크기. */
void drawLogo (juce::Graphics& g, juce::Rectangle<float> area, float fontPx, float letterSpacingEm,
               juce::Justification justification = juce::Justification::centred);
float logoWidth (float fontPx, float letterSpacingEm);

/** 자간(letter-spacing, em 단위)이 있는 한 줄 텍스트 */
void drawSpacedText (juce::Graphics& g, const juce::String& text, const juce::Font& font, float fontPx, float letterSpacingEm,
                     juce::Colour colour, juce::Rectangle<float> area, juce::Justification justification = juce::Justification::centred);
float spacedTextWidth (const juce::String& text, const juce::Font& font, float fontPx, float letterSpacingEm);

} // namespace meon
