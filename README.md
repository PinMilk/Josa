# Josa

한국어 단어의 받침에 맞는 조사를 붙여 반환하는 작은 라이브러리입니다.
**TypeScript · C · C++ · Python · Swift** 구현을 제공합니다.

```text
사람 + 와   → 사람과
사과 + 은   → 사과는
서울 + 으로 → 서울로
닭   + 로   → 닭으로
```

## 목차

- [지원 언어](#지원-언어)
- [조사 선택 규칙](#조사-선택-규칙)
- [TypeScript](#typescript)
- [C](#c)
- [C++](#cpp)
- [Python](#python)
- [Swift](#swift)
- [입력과 오류 처리](#입력과-오류-처리)
- [테스트와 성능](#테스트와-성능)

## 지원 언어

| 언어 | 구현 파일 | API | 실행 환경 |
| --- | --- | --- | --- |
| TypeScript | [`josa.ts`](./josa.ts) | `new Josa(word, wordClass).execute()` | TypeScript 컴파일러, Node.js |
| C | [`josa.c`](./josa.c) | `josa_execute()` / `josa_execute_into()` | C11 컴파일러 |
| C++ | [`josa.cpp`](./josa.cpp) | `Josa(word, wordClass).execute()` | C++11 이상 |
| Python | [`josa.py`](./josa.py) | `Josa(word, word_class).execute()` | Python 3 |
| Swift | [`josa.swift`](./josa.swift) | `try Josa(word, wordClass).execute()` | Swift 컴파일러 |

추가 외부 라이브러리는 필요하지 않습니다. C/C++ 입력과 소스 파일은 **UTF-8**을 사용하세요.
아래 명령은 저장소 루트에서 실행하며, C/C++은 macOS·Linux의 Clang 기준입니다.

## 조사 선택 규칙

입력 조사는 각 쌍의 어느 쪽을 지정해도 됩니다. `와`를 전달해도 받침이 있으면 `과`가 붙습니다.

| 입력 조사 | 받침 있음 | 받침 없음 | 예시 |
| --- | --- | --- | --- |
| `은` / `는` | 은 | 는 | 사람은 / 사과는 |
| `이` / `가` | 이 | 가 | 사람이 / 사과가 |
| `을` / `를` | 을 | 를 | 사람을 / 사과를 |
| `과` / `와` | 과 | 와 | 사람과 / 사과와 |
| `으로` / `로` | 으로 | 로 | 사람으로 / 사과로 |

**ㄹ 받침 예외:** `으로` / `로`에는 단독 ㄹ 받침도 `로`를 붙입니다.
예를 들어 `길로`, `서울로`가 됩니다. ㄺ·ㄻ 등의 겹받침은 이 예외에 포함하지 않으므로
`닭으로`, `삶으로`가 됩니다. 다른 조사에는 ㄹ 받침도 일반 받침 규칙을 적용합니다(`길은`).

## TypeScript

`example.ts`를 다음 내용으로 만드세요.

```typescript
console.log(new Josa('사람', '와').execute());    // 사람과
console.log(new Josa('사과', '은').execute());    // 사과는
console.log(new Josa('서울', '으로').execute());  // 서울로

try {
    new Josa('hello', '은').execute();
} catch (error) {
    console.error(error); // Error: Word is not Hangul.
}
```

빌드 및 실행:

```bash
mkdir -p .build
cat josa.ts example.ts > .build/example.ts
tsc --strict --target es2015 --outDir .build .build/example.ts
node .build/example.js
```

현재 `josa.ts`는 `export` 없는 스크립트입니다. 위 명령은 구현과 예제를 합친 뒤 하나의
JavaScript 파일로 컴파일합니다. npm 패키지나 ES 모듈로 import할 수 있도록 구성된 버전은 아닙니다.
`cat`은 macOS·Linux 셸 명령이며, 최신 TypeScript에서 지원하지 않는 `--module none` 설정에 의존하지 않습니다.

## C

### 문자열을 할당해 반환하는 API

`example.c`:

```c
#include <stdio.h>
#include "josa.c"  /* 작은 예제를 위해 구현을 직접 포함합니다. */

int main(void)
{
    char *result = josa_execute("서울", "으로");
    if (result == NULL) {
        perror("josa_execute");
        return 1;
    }

    puts(result);   /* 서울로 */
    free(result);   /* 반환 문자열은 호출자가 반드시 해제합니다. */
    return 0;
}
```

```bash
mkdir -p .build
clang -O3 -std=c11 -Wall -Wextra example.c -o .build/example-c
./.build/example-c
```

`josa_execute()`는 성공하면 새 문자열, 실패하면 `NULL`을 반환합니다.
입력 오류는 `errno == EINVAL`, 메모리 할당·크기 오류는 `errno == ENOMEM`입니다.

### 반복 호출 시 출력 버퍼 재사용

`josa_execute_into()`는 메모리를 할당하지 않습니다. 충분한 출력 버퍼를 미리 준비해 재사용할 수 있습니다.

```c
char output[256];
if (josa_execute_into("길", "으로", output, sizeof(output))) {
    puts(output); /* 길로. output은 free()하지 않습니다. */
} else {
    perror("josa_execute_into");
}
```

- 성공하면 `1`, 실패하면 `0`을 반환합니다.
- 용량은 **널 종료 문자를 포함한 UTF-8 바이트 수**입니다.
- 버퍼가 부족하면 `errno == ERANGE`이며, 실패 시 출력 버퍼는 변경하지 않습니다.
- 출력 버퍼는 입력 단어·조사 문자열과 겹치면 안 됩니다.

큰 프로젝트에서는 함수 선언을 별도 헤더에 두고 `josa.c`를 따로 컴파일해 링크하세요.
위 예제처럼 구현을 포함할 때는 빌드 명령에 `josa.c`를 다시 추가하지 않습니다.

<a id="cpp"></a>
## C++

`example.cpp`:

```cpp
#include <iostream>
#include "josa.cpp"

int main()
{
    try {
        std::cout << Josa("사람", "와").execute() << '\n';    // 사람과
        std::cout << Josa("서울", "으로").execute() << '\n';  // 서울로
    } catch (const std::invalid_argument &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
```

```bash
mkdir -p .build
clang++ -O3 -std=c++11 -Wall -Wextra example.cpp -o .build/example-cpp
./.build/example-cpp
```

반환값은 `std::string`이므로 수동으로 메모리를 해제하지 않습니다.
잘못된 입력에는 `std::invalid_argument("Word is not Hangul.")` 예외를 발생시킵니다.

## Python

`example.py`:

```python
from josa import Josa

print(Josa("사람", "와").execute())    # 사람과
print(Josa("사과", "은").execute())    # 사과는
print(Josa("서울", "으로").execute())  # 서울로

try:
    Josa("hello", "은").execute()
except ValueError as error:
    print(error)  # Word is not Hangul.
```

```bash
python3 example.py
```

`example.py`와 `josa.py`는 같은 디렉터리에 두세요. 별도 설치나 컴파일은 필요하지 않습니다.

## Swift

`main.swift`:

```swift
do {
    print(try Josa("사람", "와").execute())    // 사람과
    print(try Josa("사과", "은").execute())    // 사과는
    print(try Josa("서울", "으로").execute())  // 서울로
} catch {
    print(error) // Word is not Hangul.
}
```

```bash
mkdir -p .build
swiftc -O josa.swift main.swift -o .build/example-swift
./.build/example-swift
```

파일을 함께 컴파일할 때 최상위 실행 코드는 **`main.swift`**에 두세요.
`Josa`는 구조체이며, `execute()`는 문자열을 반환하거나 `JosaError.wordIsNotHangul`을 던집니다.

별도 라이브러리로 빌드하려면 다음 명령을 사용할 수 있습니다(macOS 기준).

```bash
mkdir -p .build
swiftc -O -emit-library -emit-module -module-name Josa \
  -emit-module-path .build/Josa.swiftmodule josa.swift -o .build/libJosa.dylib
```

모듈을 사용하는 프로그램에서는 `import Josa`와 함께 모듈 검색·링크 경로를 설정해야 합니다.

## 입력과 오류 처리

- **마지막 글자**가 완성형 한글(`가`~`힣`)이어야 합니다. 앞부분에 다른 문자가 있어도 됩니다.
- 숫자·영문자·문장부호로 끝나는 단어와 분해형 자모는 지원하지 않습니다.
  예: `hello`, `서울!`, `ㄹ`, `가`는 오류입니다.
- 입력을 자동 정규화하거나 뒤쪽 공백·문장부호를 제거하지 않습니다.
- 원본과의 호환성을 위해 **빈 단어는 받침 없는 입력**으로 처리합니다. `Josa("", "은")`의 결과는 `는`입니다.
- **미지원 조사**는 오류 대신 단어 뒤에 `undefined`를 붙입니다. 예: `사람 + unknown → 사람undefined`.
- C 문자열은 첫 널 문자에서 끝나며 Swift 포팅도 이 동작을 따릅니다. 일반적으로 널 문자가 없는 입력을 사용하세요.

| 언어 | 잘못된 단어 처리 |
| --- | --- |
| TypeScript | `Error("Word is not Hangul.")` |
| C | `NULL` 또는 `0` 반환, `errno == EINVAL` |
| C++ | `std::invalid_argument` |
| Python | `ValueError("Word is not Hangul.")` |
| Swift | `JosaError.wordIsNotHangul` |

## 테스트와 성능

로컬에 테스트 스크립트가 있는 경우:

```bash
python3 tests/run_tests.py
```

필수 도구는 Python 3, Clang, Clang++, TypeScript 컴파일러, Node.js, Swift 컴파일러입니다.
다섯 언어의 공통 입력, ㄹ·겹받침, 오류 입력, C 버퍼 경계와 메모리 안전성을 검증하며,
Swift는 완성형 한글 전체를 디버그·최적화 빌드에서 추가 검사합니다.

> 기존 `.gitignore`의 `/tests*` 규칙에 따라 테스트 스크립트는 Git에서 제외됩니다.
> 저장소를 새로 복제한 경우 스크립트가 없을 수 있습니다. `.build/`도 Git에서 제외됩니다.

최적화 방식, 측정 조건과 빌드 옵션은 [PERFORMANCE.md](./PERFORMANCE.md)를 참고하세요.
측정 결과는 환경·입력에 따라 달라지며 보편적인 최고 속도를 보장하지 않습니다.

## 조사란?

조사는 주로 체언에 붙어 다른 단어와의 문법적 관계를 표시하거나 뜻을 더하는 품사입니다.
이 라이브러리는 문장 전체의 의미를 분석하지 않고, 마지막 한글 음절의 받침만으로 조사를 선택합니다.

출처: [네이버 지식백과](https://terms.naver.com/entry.nhn?docId=548582&cid=46674&categoryId=46674)
