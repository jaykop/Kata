# 다음 작업 계획 — 단일 Kata 실행과 GAS 연결

작성: 2026-09-19 · 인계 대상: Claude Code

## 1. 시작점과 작업 규칙

- 유일한 작업 루트: `C:\Users\jeyko\Documents\Unreal Projects\ProjectKata`.
- 작업 전에 루트 `AGENTS.md`, `docs/Implementation-Status.md`, `docs/Conditions.md`를 읽는다. `CLAUDE.md`는 공용 지침을 참조한다.
- UE 5.8, GAS 필수, 싱글플레이. 모듈은 KataConditions / KataRuntime / KataEditor 세 개다. KataAI는 추가하지 않는다.
- **테스트·빌드·검사 및 별도 리뷰는 사용자가 담당한다. 명시적 요청 없이 실행하거나 테스트 코드를 추가하지 않는다.** 검증용 프로젝트 사본도 만들지 않는다.
- 관련 구현 파일과 기존 변경사항만 필요한 만큼 읽는다. 로컬 Config 변경 등 이전 작업자의 변경을 보존하고, 요청 범위 밖 변경을 커밋에 섞지 않는다.
- 이번 문서는 다음 구현의 제안 범위다. 과거 두 설계 문서의 모든 제안을 확정 사양으로 취급하지 않는다. 특히 콤보 소유권·Ability 대응 단위는 이번 단계에서 확정하지 않는다.

## 2. 현재 전달된 구현

`KataConditions`에 다음이 구현되어 있다.

- `UKataCondition`: C++/BP 확장, 공통 Invert, Evaluate/IsSatisfied 진입점.
- `FKataConditionContext`: Self/Target Actor 및 선택적 ASC 전달.
- `FKataConditionResult`: Pass/Fail/Invalid 및 FName 진단 사유. Invalid는 Invert하지 않는다.
- Tag: Subject, Any/All, Exact Match.
- Attribute: 절대값 또는 Current/Max 비율, 비교 연산, 같음 허용 오차.
- Distance: 양쪽 Actor/Socket 기준점, ComponentTag, 2D/3D, 거리 범위.
- Angle: 2D/3D, HalfAngle, YawOffset. 양수 Offset은 오른쪽으로 방향을 회전한다.

사용자 지침 변경 전에 조건 구현의 Editor 컴파일·링크는 성공했다. 조건 자동화 테스트 코드는 존재하지만 실행하지 않았으며, 변경 후 Game 빌드도 수행하지 않았다. 이를 통과했다고 가정하거나 인계 후 자동 실행하지 않는다.

`KataRuntime`과 `KataEditor`는 모듈 골격이다. 아직 Kata 에셋, 액션 실행기, AbilityTask, 몽타주 태스크는 없다.

## 3. 다음 단계의 목표와 범위

**Gameplay Ability에서 Kata 에셋 하나를 요청하고, 조건에 따라 시작해 몽타주를 재생하고 완료·취소 결과를 돌려받는 최소 실행 흐름**을 만든다.

포함:

1. Details 패널로 설정하는 Kata 데이터 에셋.
2. 액션·태스크 정의와 실행 상태의 분리.
3. 캐릭터당 하나의 Kata를 실행하는 컴포넌트.
4. GAS에서 시작하고 종료를 기다리는 AbilityTask.
5. 기본 몽타주 재생 태스크 하나.
6. 사용법과 현재 구현 상태 문서 갱신.

후속 범위: 콤보 에셋, 입력 버퍼, 조건 조합/ConditionSet, BT/StateTree, 다중 액션 채널, 네트워크, 커스텀 타임라인 UI, 프리뷰 시뮬레이션, 히트 판정·이동·이펙트 태스크. 이번 구현에 함께 추가하지 않는다.

## 4. 데이터와 API 제안

| 타입 | 역할 |
|---|---|
| `UKataAsset : UDataAsset` | 길이, 선택적 시작 조건, 태스크 배치 목록 |
| `FKataTaskEntry` | 태스크 정의 참조, 시작 시간, 지속 시간, 안정적인 ID |
| `UKataTask : UObject` | EditInlineNew/DefaultToInstanced인 불변 태스크 정의 |
| `UKataTaskInstance : UObject` | 실행별 상태와 Enter/Tick/Exit 처리. 공유 정의에 상태를 저장하지 않음 |
| `UKataComponent : UActorComponent` | 활성 액션, 시간 진행, 태스크 인스턴스 및 종료 이벤트 소유 |
| `FKataHandle` | 실행 한 번을 식별. 이전 실행의 콜백이 다음 실행을 종료하지 않게 구분 |
| `FKataPlayParams` | Condition Context와 실행을 요청한 Ability 등 필요한 참조 |
| `EKataExitReason` | Completed / Cancelled / Interrupted / OwnerDestroyed / Failed |
| `UAbilityTask_PlayKata` | Ability에서 시작·대기·취소 정리를 수행하는 연결 계층 |
| `UKataTask_PlayMontage` | 몽타주·재생 설정을 보관하고 GAS 경로로 재생 |

타입명은 구현 제안이며 불필요한 중간 추상화는 추가하지 않는다. 실행별 UObject는 실행 컴포넌트의 UPROPERTY 경로에서 참조되어야 한다. Outer 지정만으로 GC 수명이 보장된다고 가정하지 않는다. Ability와 Actor 참조의 수명도 명확히 한다.

`UKataComponent`의 최소 공개 API는 `Play`, `Stop`, `IsPlaying` 및 완료 알림이다. 실행 중 다른 Play 요청은 기본적으로 거절하여 암묵적인 교체·취소 정책을 만들지 않는다. 시작 실패는 핸들 유효 여부와 이유를 호출자에게 전달한다.

## 5. 진행 순서

### 5.1 Kata 에셋과 태스크 모델

- `UKataAsset`은 우선 UDataAsset으로 제공한다. 전용 Factory나 에셋 툴킷 없이 Data Asset 생성과 기본 Details 편집으로 시작한다.
- 시작 조건은 `UPROPERTY(Instanced)`로 보관한다. 미설정이면 시작을 허용하고, 설정되어 있으면 공용 `Evaluate`의 최종 Pass만 허용한다.
- 각 배치가 태스크 정의를 Instanced 소유하게 하되 실행 상태는 별도 인스턴스에 둔다.
- 액션 길이, 시작 시각, 지속 시간이 유한한 값이며 유효한 범위인지 실행 진입점에서 처리한다. 잘못된 데이터는 실패 이유와 함께 거절한다.
- 즉시 태스크는 지속 시간 0으로 표현한다. 구간 태스크는 Enter/Tick/Exit 수명주기를 갖는다. 몽타주 태스크는 재생 구간이 있는 태스크로 시작한다.

### 5.2 단일 액션 실행기

- Kata 시간은 실행기가 소유한다. v1에서는 정방향 재생과 정상 완료·중단에 집중한다. 스크럽·역재생·몽타주 섹션 점프는 구현하지 않는다.
- 이전 시각과 새 시각 사이의 경계를 시간순으로 처리하여 큰 DeltaTime에도 즉시 태스크나 짧은 구간의 진입·종료를 놓치지 않는다.
- 시각 0 태스크는 시작 때 한 번 처리한다. 같은 시각에서는 기존 구간 종료를 먼저 처리한 뒤 목록 순서로 시작·즉시 태스크를 처리하는 등 안정적인 규칙을 문서화한다.
- 정상 완료, 외부 Stop, Ability 취소, 컴포넌트 EndPlay에서 활성 태스크를 정리한다. 정리와 완료 알림은 실행당 한 번만 발생하게 한다.
- 콜백 중 중단 요청이 들어와도 활성 목록 순회나 다음 액션 상태를 훼손하지 않도록 종료 상태·핸들을 확인한다.
- 종료된 액션의 Context·태스크 인스턴스·델리게이트 참조를 해제한다.

### 5.3 GAS AbilityTask 연결

- `UAbilityTask_PlayKata`에서 현재 Ability의 Avatar와 ASC를 가져오고, 사용자가 넘긴 Target으로 Condition Context를 구성한다. ASC가 PlayerState에 있을 수 있으므로 명시적 ASC 전달 경로를 사용한다.
- Avatar의 `UKataComponent`를 사용한다. 컴포넌트가 없으면 명확히 실패를 알리고 자동 추가하지 않는다.
- 완료·중단·실패를 Blueprint 출력 델리게이트로 제공한다. 실패를 포함해 종료 경로에서 AbilityTask가 남지 않도록 한다.
- Ability 종료 시 자신이 시작한 핸들의 액션만 중단하고 연결한 델리게이트를 해제한다. 액션이 먼저 끝난 경우 재귀 종료와 중복 알림을 방지한다.
- Ability의 CommitAbility, 비용·쿨다운, 최종 EndAbility 정책은 호출 Ability가 담당한다. Kata 조건에서 자원을 소비하거나 AbilityTask가 비용을 자동으로 중복 청구하지 않는다.
- 게임 프로젝트가 사용할 수 있도록 BP에서 호출할 AbilityTask 생성 함수와 최소 사용 예를 문서에 제공한다. 별도의 범용 Ability 프레임워크는 만들지 않는다.

### 5.4 몽타주 태스크

- 실행을 소유한 Ability/ASC를 사용해 몽타주를 재생한다. 임의의 AnimInstance 재생만으로 GAS의 몽타주 상태를 우회하지 않는다.
- 에셋 참조는 v1에서 단순한 Hard 참조로 시작한다. 비동기 로딩 시스템을 별도로 추가하지 않는다.
- 필요한 Ability, ASC, Avatar, AnimInstance 또는 몽타주가 없거나 재생에 실패하면 진단 가능한 실패 결과를 전달한다.
- 실제 재생 상태와 이벤트 핸들은 태스크 인스턴스가 소유한다. 구간 종료·취소 시 자신이 시작한 재생만 정리하고 다른 Ability의 몽타주를 중단하지 않는다.
- v1은 몽타주 하나의 연속 재생으로 제한한다. 겹치는 몽타주 구간은 시작 전에 거절한다. 몽타주 재생 실패·외부 중단은 액션에 Failed/Interrupted로 전달한다.
- 몽타주가 먼저 자연 종료되면 해당 태스크는 더 이상 재생 중인 것으로 취급하지 않는다. 액션 자체의 정상 완료 시점은 명시한 Kata 길이를 따른다. 태스크 구간이 먼저 끝나면 그 정책대로 해당 몽타주를 정리한다.

### 5.5 인계 문서 갱신

- `docs/Implementation-Status.md`에 실제 구현 타입과 남은 기능을 기록한다.
- `docs/Runtime-Usage.md`에 Data Asset 구성, Actor의 KataComponent, AbilityTask 호출·종료 처리 예를 작성한다.
- 예제 설명과 실제 구현 완료를 구분하고, 테스트·빌드를 실행하지 않았다면 그대로 적는다.
- 완료 후 변경 요약과 사용자가 실행할 때 알아야 할 사항만 간결하게 보고한다. 별도 검사 루프를 시작하지 않는다.

## 6. Claude Code에 전달할 시작 문구

> 루트 AGENTS.md를 먼저 읽고 docs/Next-Work-Plan.md의 단일 Kata 실행과 GAS 연결 범위를 구현해줘. 기존 기본 조건을 재사용하고, 콤보·타임라인 UI·프리뷰·KataAI는 추가하지 마. 작업 경로는 ProjectKata 하나로 유지하고 기존 로컬 변경을 보존해줘. 테스트 코드 작성, 빌드, 테스트, 별도 검사는 내가 할 테니 실행하지 말고 구현과 문서 갱신까지만 진행해줘.
