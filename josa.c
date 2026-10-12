/*
 * UTF-8 문자열에 받침 유무에 맞는 조사를 붙입니다.
 * 사용: char *result = josa_execute("사람", "와");
 * 성공 시 반환된 문자열은 호출자가 free()해야 합니다.
 * 실패 시 NULL을 반환하고 errno를 EINVAL 또는 ENOMEM으로 설정합니다.
 * 미지원 조사는 "undefined"로 처리하며, ㄹ 받침에는 '으로' 대신 '로'를 붙입니다.
 */
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* UTF-8 유효성을 검사하고 마지막 유니코드 코드 포인트를 구합니다. */
static int last_code_point(const char *text, uint32_t *last)
{
    const unsigned char *cursor = (const unsigned char *)text;
    /* 원본은 빈 단어도 받침 없는 입력으로 처리합니다. */
    if (*cursor == 0) {
        *last = 0xAC00;
        return 1;
    }

    while (*cursor != 0) {
        uint32_t code;
        uint32_t minimum;
        unsigned int remaining;
        unsigned char first = *cursor++;

        if (first < 0x80) {
            code = first; minimum = 0; remaining = 0;
        } else if (first >= 0xC2 && first <= 0xDF) {
            code = first & 0x1F; minimum = 0x80; remaining = 1;
        } else if (first >= 0xE0 && first <= 0xEF) {
            code = first & 0x0F; minimum = 0x800; remaining = 2;
        } else if (first >= 0xF0 && first <= 0xF4) {
            code = first & 0x07; minimum = 0x10000; remaining = 3;
        } else {
            return 0;
        }
        while (remaining-- > 0) {
            if ((*cursor & 0xC0) != 0x80) return 0;
            code = (code << 6) | (*cursor++ & 0x3F);
        }
        if (code < minimum || code > 0x10FFFF ||
            (code >= 0xD800 && code <= 0xDFFF)) return 0;
        *last = code;
    }
    return 1;
}

char *josa_execute(const char *word, const char *word_class)
{
    static const char *const pairs[][2] = {
        {"은", "는"}, {"이", "가"}, {"을", "를"}, {"과", "와"}, {"으로", "로"}
    };
    uint32_t last;
    const char *suffix = "undefined";
    size_t i, word_length, suffix_length;
    char *result;
    int has_last;

    if (word == NULL || word_class == NULL || !last_code_point(word, &last) ||
        last < 0xAC00 || last > 0xD7A3) {
        errno = EINVAL; /* 원본의 "Word is not Hangul."에 해당하는 입력 오류. */
        return NULL;
    }
    has_last = (last - 0xAC00) % 28 != 0;
    for (i = 0; i < sizeof(pairs) / sizeof(pairs[0]); ++i) {
        if (strcmp(word_class, pairs[i][0]) == 0 || strcmp(word_class, pairs[i][1]) == 0) {
            suffix = pairs[i][has_last ? 0 : 1];
            /* 종성 인덱스 8(ㄹ)에만 적용하며 ㄺ·ㄻ 등의 겹받침은 제외합니다. */
            if (i == 4 && (last - 0xAC00) % 28 == 8) suffix = "로";
            break;
        }
    }

    word_length = strlen(word);
    suffix_length = strlen(suffix);
    if (word_length > SIZE_MAX - suffix_length - 1) {
        errno = ENOMEM;
        return NULL;
    }
    result = (char *)malloc(word_length + suffix_length + 1);
    if (result == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    memcpy(result, word, word_length);
    memcpy(result + word_length, suffix, suffix_length + 1);
    return result;
}
