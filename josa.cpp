/*
 * UTF-8 입력을 사용하는 C++11 포팅 버전.
 * 사용: Josa("사람", "와").execute() → "사람과"
 * 미지원 조사는 "undefined"로 처리하며, ㄹ 받침에는 '으로' 대신 '로'를 붙입니다.
 * word/wordClass는 공개 가변 속성: 결과 캐시 없이 매 호출의 최신 값을 사용합니다.
 */
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

class Josa {
public:
    std::string word;
    std::string wordClass; // class는 C++ 예약어이므로 이름을 변경합니다.

    Josa(std::string input, const std::string &inputClass)
        : word(std::move(input)), wordClass("undefined")
    {
        for (const auto &pair : pairs()) {
            if (inputClass == pair.base || inputClass == pair.alternate) {
                wordClass = pair.base;
                break;
            }
        }
    }

    std::string execute() const
    {
        const auto last = lastCodePoint(word);
        if (last < 0xAC00 || last > 0xD7A3) {
            throw std::invalid_argument("Word is not Hangul.");
        }
        const auto final = (last - 0xAC00) % 28;
        for (const auto &pair : pairs()) {
            if (wordClass == pair.base) {
                // 종성 인덱스 8(ㄹ)에만 적용하며 겹받침은 기존 규칙을 유지합니다.
                const bool alternate = final == 0 || (pair.baseLength == 6 && final == 8);
                return concatenate(alternate ? pair.alternate : pair.base,
                                   alternate ? 3 : pair.baseLength);
            }
        }
        return concatenate(wordClass.data(), wordClass.size());
    }

private:
    // 조사는 불변 리터럴입니다. std::string 테이블의 초기화/소멸과 길이 계산을 없앱니다.
    struct Pair { const char *base; const char *alternate; std::size_t baseLength; };
    static const Pair (&pairs())[5]
    {
        static const Pair values[] = {
            {"은", "는", 3}, {"이", "가", 3}, {"을", "를", 3}, {"과", "와", 3}, {"으로", "로", 6}
        };
        return values;
    }

    // 긴 단어도 중간 문자열 없이 최종 용량을 확보한 뒤 붙입니다.
    // 짧은 결과는 표준 라이브러리의 SSO(짧은 문자열 최적화)를 그대로 활용합니다.
    std::string concatenate(const char *suffix, std::size_t length) const
    {
        std::string result;
        if (length > result.max_size() || word.size() > result.max_size() - length) {
            throw std::length_error("Josa result is too long.");
        }
        result.reserve(word.size() + length);
        result.append(word);
        result.append(suffix, length);
        return result;
    }

    // 전체 UTF-8 검증은 유지합니다. 끝 음절만 보면 앞부분의 잘못된 입력을 놓칩니다.
    // 선두 바이트로 길이를 정하고, 연속 바이트/최소값/대리 코드/유니코드 상한을 검사합니다.
    // 입력 n바이트에 대해 O(n)이며 디코딩 과정에서 별도 문자열/배열을 할당하지 않습니다.
    static std::uint32_t lastCodePoint(const std::string &text)
    {
        // 원본은 빈 단어도 받침 없는 입력으로 처리합니다.
        if (text.empty()) return 0xAC00;
        std::uint32_t last = 0;
        std::size_t offset = 0;
        while (offset < text.size()) {
            const auto first = static_cast<unsigned char>(text[offset++]);
            std::uint32_t code, minimum;
            unsigned remaining;
            if (first < 0x80) {
                code = first; minimum = 0; remaining = 0;
            } else if (first >= 0xC2 && first <= 0xDF) {
                code = first & 0x1F; minimum = 0x80; remaining = 1;
            } else if (first >= 0xE0 && first <= 0xEF) {
                code = first & 0x0F; minimum = 0x800; remaining = 2;
            } else if (first >= 0xF0 && first <= 0xF4) {
                code = first & 0x07; minimum = 0x10000; remaining = 3;
            } else {
                throw std::invalid_argument("Word is not Hangul.");
            }
            while (remaining-- > 0) {
                if (offset == text.size()) throw std::invalid_argument("Word is not Hangul.");
                const auto next = static_cast<unsigned char>(text[offset++]);
                if ((next & 0xC0) != 0x80) throw std::invalid_argument("Word is not Hangul.");
                code = (code << 6) | (next & 0x3F);
            }
            if (code < minimum || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) {
                throw std::invalid_argument("Word is not Hangul.");
            }
            last = code;
        }
        return last;
    }
};
