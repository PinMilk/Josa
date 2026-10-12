"""받침 유무에 따라 조사를 선택하는 TypeScript Josa의 Python 3 포팅.

사용 예: Josa("사람", "와").execute() -> "사람과"
미지원 조사는 "undefined"로 처리하며, ㄹ 받침에는 '으로' 대신 '로'를 붙입니다.
"""


class Josa:
    _PAIRS = {
        "은": "는",
        "이": "가",
        "을": "를",
        "과": "와",
        "으로": "로",
    }

    def __init__(self, word: str, word_class: str) -> None:
        self.word = word
        # class는 Python 예약어이므로 word_class로 표기합니다.
        self.word_class = next(
            (base for base, alternate in self._PAIRS.items()
             if word_class in (base, alternate)),
            "undefined",
        )

    def execute(self) -> str:
        # 원본은 빈 단어도 받침 없는 입력으로 처리합니다.
        last = ord(self.word[-1]) if self.word else 0xAC00
        if not 0xAC00 <= last <= 0xD7A3:
            raise ValueError("Word is not Hangul.")

        # 완성형 한글은 한 음절마다 28개의 종성 조합을 가집니다.
        has_last = (last - 0xAC00) % 28 != 0
        suffix = self.word_class
        # 종성 인덱스 8(ㄹ)에만 적용하며 겹받침은 기존 규칙을 유지합니다.
        if suffix == "으로" and (last - 0xAC00) % 28 == 8:
            suffix = "로"
        elif not has_last:
            suffix = self._PAIRS.get(suffix, suffix)
        return self.word + suffix
