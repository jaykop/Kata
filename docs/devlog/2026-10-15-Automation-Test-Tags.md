# 자동화 테스트의 GameplayTag 출처

작성: 2026-10-15  
갱신: 2026-10-15  
유형: 결정 기록  
대상: KataRuntime 자동화 테스트(`Kata.Runtime.Resolve`, `Kata.Runtime.Cleanup`), [자동화 테스트 작성·실행](../manual/Automation-Tests.md)  
기준: 768baa0 이후 작업 트리. `KataCleanupTests.cpp`, manual 갱신과 이 문서는 같은 커밋에 넣는다

## 배경과 결론

[C++ 자동화 테스트 정책](2026-10-14-Automation-Test-Policy.md)은 테스트용 GameplayTag를 어디에 등록할지 첫 구현에서 정하기로 남겨 두었다. Kata 플러그인은 게임용 태그를 정의하지 않는다([게임플레이 태그 사용법](../manual/Gameplay-Tags.md)). 그런데 정리 불변식 테스트(#102)가 쓰는 태스크는 모두 태그가 있어야 설정 오류 없이 실행된다. Loose Tag, 전이·캔슬 창 태그, 이벤트 태그가 그 예다.

2026-10-15 사용자가 결정했다. 테스트는 ProjectKata의 `Config/Tags`에 등록된 태그를 빌려 쓰고, 테스트 전용 태그는 만들지 않는다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 테스트 태그 출처 | 정하지 않았다. `Kata.Conditions`는 이미 `Status.LockOn.*`를 쓰고 있었다 | ProjectKata 태그를 빌려 쓴다. manual 작성 규칙 7에 적었다 |
| `Kata.Runtime.Resolve`(#101) | 태그 출처를 정하기 전에 작성했다 | 태그가 필요 없는 Apply Gameplay Effect 태스크로 구성한 그대로 둔다 |
| `Kata.Runtime.Cleanup`(#102) | 없음 | `Status.SuperArmor`·`Status.Invincible`·`Status.IgnoreDamage`, `Window.Transition.Combo`, `Window.Cancel.Move`·`Jump`, `Event.Hit`, KataTags용 `Trigger.Attack.Light`·`Trigger.Dodge`를 쓴다 |

## 주요 결정과 이유

- **프로젝트 태그를 빌려 쓴다.** 테스트를 실행하는 곳은 샘플 프로젝트 ProjectKata뿐이다. 필요한 태그도 모두 이미 등록돼 있다. 이 방식이면 "플러그인은 태그를 정의하지 않는다"는 규칙에 예외를 만들지 않아도 된다. 대가로 이 테스트들은 ProjectKata 밖에서는 통과하지 않는다. manual 제한 표에 이 사실을 적었다.
- **채택하지 않은 대안: 테스트 파일 안의 네이티브 태그.** `WITH_DEV_AUTOMATION_TESTS` 안에서 테스트 전용 태그를 선언하면 테스트가 자기 완결이 된다. 하지만 이 태그는 개발 빌드 에디터의 태그 선택 목록에도 나타난다. 플러그인이 태그를 정의하지 않는다는 규칙에도 예외가 생긴다.
- **자식이 없는 태그만 고른다.** ASC의 `GetTagCount`는 부모 태그의 수에 자식 태그의 수를 합친다. 수를 단언하는 태그에 자식이 있으면 다른 용도의 태그와 수가 섞인다.
- **KataTags에는 `Trigger.*`를 쓴다.** KataTags는 ASC에 붙지 않는 분류 태그다. 그래서 수를 세는 `Status.*`와 겹치지 않는다. 문서로 정한 루트가 없어 이미 등록된 태그 가운데 의미가 가까운 것을 골랐다.

## 근거

- [Gameplay-Tags.md](../manual/Gameplay-Tags.md) "목적과 준비": 플러그인은 게임용 태그를 정의하지 않는다.
- [KataCleanupTests.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Tests/KataCleanupTests.cpp): `KataCleanupTests::FTags`의 태그 목록과 주석.
- [KataConditionTests.cpp](../../Plugins/Kata/Source/KataConditions/Private/Tests/KataConditionTests.cpp): 기존 조건 테스트가 쓰는 `Status.LockOn.*`.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 사용 태그의 등록 여부와 자식 유무 | 에이전트가 `Config/Tags/**/*.ini` 대조 | 아홉 태그 모두 등록돼 있고 자식이 없다 | - |
| `KataCleanupTests.cpp` 컴파일 | 2026-10-15 에이전트가 사용자 요청으로 Editor 빌드 | 통과, 경고 없음 | - |
| headless 테스트 실행 | 2026-10-15 에이전트가 시도(`Kata.Runtime.Resolve`) | 엔진 시작 단계에서 Mutable 플러그인 로드 실패로 중단 | headless 경로로는 다시 실행하지 않았다 |
| 테스트 실행 | 2026-10-15 사용자가 Editor에서 실행 | `Kata.Runtime.Resolve` 8건, `Kata.Runtime.Cleanup` 5건, `Kata.Graph.Transition` 5건 모두 통과 | - |

## 남은 제한과 후속 작업

- 테스트 실행은 2026-10-15 사용자가 Editor에서 확인했다([#101](https://github.com/jaykop/Kata/issues/101), [#102](https://github.com/jaykop/Kata/issues/102)).
- 그래프 전이 테스트([#103](https://github.com/jaykop/Kata/issues/103))도 같은 규칙으로 ProjectKata 태그를 빌려 쓴다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [자동화 테스트 작성·실행](../manual/Automation-Tests.md) | 작성 규칙 7, 그룹 표, 제한 표 갱신 |
| [C++ 자동화 테스트 정책](2026-10-14-Automation-Test-Policy.md) | 당시 판단은 그대로 두고 남은 제한 항목에 이 문서 링크 추가 |
| [#102](https://github.com/jaykop/Kata/issues/102) | 구현·빌드 결과 댓글과 확인 대기 라벨. 게시는 사용자 확인 후 |
