/**
 * @param {string} word - Word to set Josa
 * @param {string} class - Class of Josa
 * 
 * @returns {string} Word and correct Josa
 * 
 * @example
 * console.log(new Josa('사람', '와').execute());
 */

class Josa {
    public word: string;
    public class: string;

    constructor(word: string, wordClass: string) {
        const format: any = {
            은: '은',
            이: '이',
            을: '을',
            과: '과',
            으로: '으로',
            는: '은',
            가: '이',
            를: '을',
            와: '과',
            로: '으로'
        };

        this.word = word;
        this.class = format[wordClass];
    }

    private isHangul() {
        const charCode: number = this.word.charCodeAt(this.word.length - 1) - 44032;
        if (charCode < 0 || charCode > 11171) return false;
        return true;
    }

    private hasLast() {
        return ((this.word.charCodeAt(this.word.length - 1) - 44032) % 28 > 0) ? true : false;
    }

    public execute() {
        const hasLast = this.hasLast();

        if (!this.isHangul()) throw new Error('Word is not Hangul.');

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
                const hasRieul = (this.word.charCodeAt(this.word.length - 1) - 44032) % 28 === 8;
                return this.word + (hasLast && !hasRieul ? '으로' : '로');
            default:
                return this.word + this.class;
        }
    }
}
