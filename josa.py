"""받침 유무에 따라 조사를 선택하는 TypeScript Josa의 Python 3 포팅.

사용 예: Josa("사람", "와").execute() -> "사람과"
미지원 조사는 "undefined"로 처리하며, ㄹ 받침에는 '으로' 대신 '로'를 붙입니다.
공개 속성은 실행 사이에 바꿀 수 있으므로 단어의 종성은 캐시하지 않습니다.
"""


class Josa:
    _PAIRS = {
        "은": "는",
        "이": "가",
        "을": "를",
        "과": "와",
        "으로": "로",
    }

    # 열 개의 입력 조사를 한 번만 정규화해 생성자에서 선형 검색/제너레이터를 없앱니다.
    # 클래스 생성 시 한 번 만들어지고 모든 인스턴스가 O(1) 조회로 재사용합니다.
    _FORMAT = {particle: base for base, alternate in _PAIRS.items()
               for particle in (base, alternate)}

    def __init__(self, word: str, word_class: str) -> None:
        self.word = word
        # class는 Python 예약어이므로 word_class로 표기합니다.
        self.word_class = self._FORMAT.get(word_class, "undefined")

    def execute(self) -> str:
        # 원본은 빈 단어도 받침 없는 입력으로 처리합니다.
        last = ord(self.word[-1]) if self.word else 0xAC00
        if not 0xAC00 <= last <= 0xD7A3:
            raise ValueError("Word is not Hangul.")

        # 완성형 한글은 한 음절마다 28개의 종성 조합을 가집니다.
        final = (last - 0xAC00) % 28
        suffix = self.word_class
        # 종성 인덱스 8(ㄹ)에만 적용하며 겹받침은 기존 규칙을 유지합니다.
        if suffix == "으로" and final == 8:
            suffix = "로"
        elif final == 0:
            suffix = self._PAIRS.get(suffix, suffix)
        return self.word + suffix
