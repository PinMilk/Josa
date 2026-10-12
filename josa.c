/*
 * UTF-8 문자열에 받침 유무에 맞는 조사를 붙입니다.
 * 사용: char *result = josa_execute("사람", "와");
 * 성공 시 반환된 문자열은 호출자가 free()해야 합니다.
 * 실패 시 NULL을 반환하고 errno를 EINVAL 또는 ENOMEM으로 설정합니다.
 * 반복 호출 시 josa_execute_into()에 재사용 버퍼를 전달하면 malloc/free가 필요 없습니다.
 * UTF-8 입력 길이를 n이라 하면 시간 O(n), 기존 API의 반환 버퍼 공간 O(n)입니다.
 * 전역 가변 상태가 없으며 다른 스레드에서도 독립된 출력 버퍼로 호출할 수 있습니다.
 * 미지원 조사는 "undefined"로 처리하며, ㄹ 받침에는 '으로' 대신 '로'를 붙입니다.
 */
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
 * UTF-8 검증, 마지막 코드 포인트, 바이트 길이를 한 번의 순회로 구합니다.
 * 끝 음절만 검사하면 앞부분의 잘못된 UTF-8을 놓치므로 전체 검증은 유지합니다.
 * 호출 이후 strlen(word)로 다시 순회할 필요가 없도록 길이도 함께 반환합니다.
 */
static int last_code_point(const char *text, uint32_t *last, size_t *length)
{
    const unsigned char *cursor = (const unsigned char *)text;
    /* 원본은 빈 단어도 받침 없는 입력으로 처리합니다. */
    if (*cursor == 0) {
        *last = 0xAC00;
        *length = 0;
        return 1;
    }

    while (*cursor != 0) {
        uint32_t code;
        uint32_t minimum;
        unsigned int remaining;
        unsigned char first = *cursor++;

        /* 선두 바이트로 1~4바이트 길이를 정합니다. C0/C1과 F5 이상은 잘못된 UTF-8입니다. */
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
        /* 연속 바이트는 10xxxxxx 형식이어야 합니다. 널 문자도 여기서 거부됩니다. */
        while (remaining-- > 0) {
            if ((*cursor & 0xC0) != 0x80) return 0;
            code = (code << 6) | (*cursor++ & 0x3F);
        }
        /* 과도하게 긴 인코딩, 유니코드 범위 초과, UTF-16 대리 코드 포인트를 거부합니다. */
        if (code < minimum || code > 0x10FFFF ||
            (code >= 0xD800 && code <= 0xDFFF)) return 0;
        *last = code;
    }
    *length = (size_t)(cursor - (const unsigned char *)text);
    return 1;
}

/* 조사 선택 결과만 보관합니다. 전역 가변 상태나 입력 문자열 캐시는 사용하지 않습니다. */
struct josa_parts {
    const char *suffix;
    size_t word_length;
    size_t suffix_length;
};

static int select_parts(const char *word, const char *word_class, struct josa_parts *parts)
{
    /* 조사 5쌍은 매우 작아 해시 테이블을 만들지 않고 불변 리터럴을 직접 비교합니다. */
    static const char *const pairs[][2] = {
        {"은", "는"}, {"이", "가"}, {"을", "를"}, {"과", "와"}, {"으로", "로"}
    };
    uint32_t last;
    size_t i;
    unsigned int final;

    if (word == NULL || word_class == NULL || !last_code_point(word, &last, &parts->word_length) ||
        last < 0xAC00 || last > 0xD7A3) {
        errno = EINVAL; /* 원본의 "Word is not Hangul."에 해당하는 입력 오류. */
        return 0;
    }
    /* 완성형 한글의 종성은 28개: 0은 없음, 8은 ㄹ. 음절 범위 검증 뒤 한 번만 계산합니다. */
    final = (last - 0xAC00) % 28;
    parts->suffix = "undefined";
    parts->suffix_length = 9;
    for (i = 0; i < sizeof(pairs) / sizeof(pairs[0]); ++i) {
        if (strcmp(word_class, pairs[i][0]) == 0 || strcmp(word_class, pairs[i][1]) == 0) {
            const int use_alternate = final == 0 || (i == 4 && final == 8);
            parts->suffix = pairs[i][use_alternate ? 1 : 0];
            /* 지원 조사의 UTF-8 길이는 한 음절 3바이트, '으로'만 6바이트입니다. */
            parts->suffix_length = i == 4 && !use_alternate ? 6 : 3;
            break;
        }
    }

    /* 덧셈 전에 검사해 길이 합계의 size_t 오버플로와 지나치게 작은 할당을 방지합니다. */
    if (parts->word_length > SIZE_MAX - parts->suffix_length - 1) {
        errno = ENOMEM;
        return 0;
    }
    return 1;
}

/*
 * 무할당 API: 성공하면 1, 실패하면 0을 반환합니다.
 * capacity는 널 문자를 포함한 바이트 수이며, 부족하면 errno=ERANGE입니다.
 * dest는 word/word_class와 겹치지 않는 호출자 소유 버퍼여야 합니다.
 * 입력/용량 검사 완료 전에는 dest를 변경하지 않습니다.
 */
int josa_execute_into(const char *word, const char *word_class, char *dest, size_t capacity)
{
    struct josa_parts parts;
    if (dest == NULL) { errno = EINVAL; return 0; }
    if (!select_parts(word, word_class, &parts)) return 0;
    if (capacity <= parts.word_length + parts.suffix_length) {
        errno = ERANGE;
        return 0;
    }
    memcpy(dest, word, parts.word_length);
    memcpy(dest + parts.word_length, parts.suffix, parts.suffix_length + 1);
    return 1;
}

/* 기존 API는 그대로 유지합니다. 최종 크기만큼 한 번 할당하고 두 번 복사합니다. */
char *josa_execute(const char *word, const char *word_class)
{
    struct josa_parts parts;
    char *result;
    if (!select_parts(word, word_class, &parts)) return NULL;
    result = (char *)malloc(parts.word_length + parts.suffix_length + 1);
    if (result == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    memcpy(result, word, parts.word_length);
    memcpy(result + parts.word_length, parts.suffix, parts.suffix_length + 1);
    return result;
}
