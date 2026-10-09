// MEON - 이모지 처리
// macOS 는 CoreText 가 대체 글꼴(Apple Color Emoji)과 줄바꿈을 알아서 처리한다 (MeonTheme 의 *WithEmoji 함수).
// Windows 의 Pretendard 는 GDI 메모리 글꼴이라 JUCE 가 DirectWrite 배치를 못 쓰고 자체 배치로 떨어지는데,
// 이 배치는 대체 글꼴이 없고(이모지가 안 보임) 공백에서만 줄을 바꾸며, 이모지(UTF-16 서로게이트 쌍)를 두 글자로 센다.
// 그래서 Windows 에서는 여기서 직접 줄을 나누고, 이모지는 DirectWrite 로 컬러 그림을 만들어 그린다.
#pragma once

#include "MeonTheme.h"

namespace meon::emoji
{

/** CoreText 가 이모지·줄바꿈을 처리하는 플랫폼인가 (아니면 아래 직접 배치를 쓴다) */
#if JUCE_MAC
constexpr bool nativeLayout = true;
#else
constexpr bool nativeLayout = false;
#endif

/** text[start] 에서 이모지 묶음(피부색·ZWJ·국기·키캡 포함)이 시작하면 그 끝 위치, 아니면 start */
int clusterEnd (const juce::String& text, int start);

/** 이모지 한 묶음이 차지하는 폭 (px). em 은 글꼴 크기(px). */
inline float advanceForEm (float em) { return em * 1.2f; }

/** 이모지를 컬러로 그린다. em 은 둘레 글자 크기, x 는 칸 왼쪽, slot 은 칸 폭, baseline 은 글자 기준선. */
void draw (juce::Graphics& g, const juce::String& cluster, float em, float x, float slot, float baseline);

/** 이모지를 정해진 폭으로 재고, 이모지 묶음을 한 글자로 세는 글꼴 (Windows 입력창용).
    이모지 글자 자체는 그리지 않는다 (빈 윤곽선) — 그리는 쪽에서 draw() 로 덧그린다. */
juce::Typeface::Ptr makeEmojiAwareTypeface (juce::Typeface::Ptr base);

//==============================================================================
/** 직접 배치 (Windows) */
struct Piece
{
    juce::String text;
    bool isEmoji = false;
    float x = 0.0f, width = 0.0f;
    int start = 0;   // 원래 문자열에서의 글자 위치
};

struct Line
{
    std::vector<Piece> pieces;
    float width = 0.0f;   // 줄 끝 공백 제외
};

/** maxWidth <= 0 이면 줄바꿈 없이 한 줄 ('\n' 은 공백으로 바꿔 넘긴다).
    한글·한자는 글자 사이 어디서나, 그 밖의 단어는 공백에서 나누고, 한 줄보다 긴 단어는 글자 단위로 자른다. */
std::vector<Line> layout (const juce::String& text, const juce::Font& font, float maxWidth);

/** 배치한 줄들의 높이 (줄 간격 lineHeightPx) */
float height (const std::vector<Line>& lines, const juce::Font& font, float lineHeightPx);

/** links 범위는 linkColour 와 밑줄로 그린다 */
void drawLines (juce::Graphics& g, const std::vector<Line>& lines, const juce::Font& font, juce::Colour colour,
                juce::Rectangle<float> area, float lineHeightPx, juce::Justification justification,
                const TextRanges& links = {}, juce::Colour linkColour = {});

/** drawLines 와 같은 인자로 그렸을 때 p 가 links 의 몇 번째 위에 있는지 (없으면 -1) */
int hitTestLink (const std::vector<Line>& lines, const juce::Font& font, juce::Rectangle<float> area, float lineHeightPx,
                 juce::Justification justification, const TextRanges& links, juce::Point<float> p);

} // namespace meon::emoji
