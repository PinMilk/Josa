/**
 * 완성형 한글의 종성으로 조사를 선택합니다.
 * 사용: new Josa('사람', '와').execute() → '사람과'
 * ㄹ + 으로/로 → 로. 빈 단어와 미지원 조사의 기존 처리도 유지합니다.
 * word/class는 수정 가능한 공개 속성이므로 계산 결과를 인스턴스에 캐시하지 않습니다.
 */

class Josa {
    public word: string;
    public class: string | undefined;

    // 생성할 때마다 같은 객체를 만들지 않고 모든 인스턴스가 조회 표를 공유합니다.
    // 프로토타입 없는 표이므로 'constructor' 같은 이름도 미지원 조사로 처리됩니다.
    private static readonly format: Readonly<Record<string, string>> = Object.freeze(
        Object.assign(Object.create(null), {
            은: '은', 는: '은', 이: '이', 가: '이', 을: '을', 를: '을',
            과: '과', 와: '과', 으로: '으로', 로: '으로'
        })
    );

    constructor(word: string, wordClass: string) {
        this.word = word;
        this.class = Josa.format[wordClass];
    }

    public execute(): string {
        // 한글 음절은 UTF-16 한 코드 단위이므로 마지막 단위를 한 번만 읽으면 됩니다.
        // 범위 검사와 나머지 계산을 공유해 중복 charCodeAt/종성 계산을 제거합니다.
        const offset = this.word.charCodeAt(this.word.length - 1) - 0xAC00;
        if (offset < 0 || offset > 11171) throw new Error('Word is not Hangul.');
        const final = offset % 28;
        const hasLast = final > 0;
        // 빈 단어의 offset은 NaN: 원본처럼 받침 없는 입력으로 처리됩니다.

        switch (this.class) {
            case '은':
                return this.word + (hasLast ? '은' : '는');
            case '이':
                return this.word + (hasLast ? '이' : '가');
            case '을':
                return this.word + (hasLast ? '을' : '를');
            case '과':
                return this.word + (hasLast ? '과' : '와');
            case '으로':
                // 종성 인덱스 8은 ㄹ: 받침이 있어도 '로'를 붙입니다.
                return this.word + (hasLast && final !== 8 ? '으로' : '로');
            default:
                return this.word + this.class;
        }
    }
}
