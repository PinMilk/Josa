/*
 * UTF-8 입력을 사용하는 C++11 포팅 버전.
 * 사용: Josa("사람", "와").execute() → "사람과"
 * 미지원 조사는 "undefined"로 처리하며, ㄹ 받침에는 '으로' 대신 '로'를 붙입니다.
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
            if (inputClass == pair.first || inputClass == pair.second) {
                wordClass = pair.first;
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
        const bool hasLast = (last - 0xAC00) % 28 != 0;
        for (const auto &pair : pairs()) {
            if (wordClass == pair.first) {
                // 종성 인덱스 8(ㄹ)에만 적용하며 겹받침은 기존 규칙을 유지합니다.
                if (wordClass == "으로" && (last - 0xAC00) % 28 == 8) return word + "로";
                return word + (hasLast ? pair.first : pair.second);
            }
        }
        return word + wordClass;
    }

private:
    static const std::pair<std::string, std::string> (&pairs())[5]
    {
        static const std::pair<std::string, std::string> values[] = {
            {"은", "는"}, {"이", "가"}, {"을", "를"}, {"과", "와"}, {"으로", "로"}
        };
        return values;
    }

    // 잘못된 UTF-8이나 한글 음절로 끝나지 않는 입력은 예외 처리합니다.
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
