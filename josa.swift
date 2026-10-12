/// 한글 음절로 끝나지 않는 입력에 대한 오류입니다.
public enum JosaError: Error, Equatable, CustomStringConvertible {
    case wordIsNotHangul

    public var description: String { "Word is not Hangul." }
}

/// C 버전의 조사 선택 규칙을 Swift 문자열과 예외 처리로 구현합니다.
///
/// 사용 예: `try Josa("서울", "으로").execute()` → `"서울로"`
/// 빈 단어는 받침 없는 입력으로, 미지원 조사는 `"undefined"`로 처리합니다.
/// 공개 속성을 수정할 수 있으므로 단어/조사의 결과를 인스턴스에 캐시하지 않습니다.
public struct Josa {
    public var word: String
    public var wordClass: String

    private static let pairs: [(base: String, alternate: String)] = [
        ("은", "는"), ("이", "가"), ("을", "를"), ("과", "와"), ("으로", "로")
    ]

    // 최대 6바이트인 조사만 작은 정수 키로 바꿔 조회합니다.
    // String 딕셔너리의 정규화 동등 비교와 반복 배열 검색을 피하면서 C 바이트 비교를 유지합니다.
    // 표는 한 번만 초기화하며 이후 모든 인스턴스가 불변 데이터를 공유합니다.
    private static let format: [UInt64: String] = {
        var result: [UInt64: String] = [:]
        result.reserveCapacity(10)
        for pair in pairs {
            result[particleKey(pair.base)!] = pair.base
            result[particleKey(pair.alternate)!] = pair.base
        }
        return result
    }()

    private static let canonicalPairs: [UInt64: (base: String, alternate: String)] = {
        var result: [UInt64: (base: String, alternate: String)] = [:]
        result.reserveCapacity(5)
        for pair in pairs { result[particleKey(pair.base)!] = pair }
        return result
    }()

    public init(_ word: String, _ wordClass: String) {
        self.word = word
        self.wordClass = Self.particleKey(wordClass, nullTerminated: true).flatMap { Self.format[$0] } ?? "undefined"
    }

    /// 받침에 맞는 조사를 붙입니다. 반환 문자열의 메모리는 Swift가 관리합니다.
    public func execute() throws -> String {
        // Character는 여러 코드 포인트를 묶으므로 C처럼 마지막 스칼라를 검사합니다.
        // C 문자열은 첫 널 문자에서 끝나므로 동일하게 그 앞부분만 사용합니다.
        let input = Self.cString(word)
        let last = input.unicodeScalars.last?.value ?? 0xAC00
        guard (0xAC00...0xD7A3).contains(last) else {
            throw JosaError.wordIsNotHangul
        }

        let final = (last - 0xAC00) % 28
        if let key = Self.particleKey(wordClass), let pair = Self.canonicalPairs[key] {
            // 종성 인덱스 8은 단독 ㄹ 받침입니다. ㄺ·ㄻ 등의 겹받침은 제외합니다.
            let useAlternate = final == 0 || (pair.base == "으로" && final == 8)
            return input + (useAlternate ? pair.alternate : pair.base)
        }
        return input + wordClass
    }

    // 각 바이트를 순서대로 포장합니다. 해시가 아니므로 최대 6바이트 내에서는 충돌이 없습니다.
    // 생성자 입력만 C 문자열처럼 널에서 멈춥니다. 공개 wordClass를 나중에 수정했다면
    // 원래 구현의 전체 바이트 비교를 유지해 임의의 문자열을 지원 조사로 오인하지 않습니다.
    private static func particleKey(_ value: String, nullTerminated: Bool = false) -> UInt64? {
        var result: UInt64 = 0
        var length = 0
        for byte in value.utf8 {
            if byte == 0 {
                if nullTerminated { break }
                return nil
            }
            if length == 6 { return nil }
            result = (result << 8) | UInt64(byte)
            length += 1
        }
        return result
    }

    private static func cString(_ value: String) -> String {
        // 일반 입력에는 널 문자가 없습니다. 이 경로에서 문자열을 재구성하지 않습니다.
        // 실제 널 문자가 있는 드문 경우에만 앞부분을 새 문자열로 만듭니다.
        guard let end = value.utf8.firstIndex(of: 0) else { return value }
        return String(value.unicodeScalars[..<end])
    }
}
