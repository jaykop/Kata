# 액션 해석 비용 측정과 시작 판정 경량화

작성: 2026-10-11  
갱신: 2026-10-11  
유형: 진단, 결정 기록, 구현 기록  
대상: KataRuntime(`UKataActionAssetBase`, `UKataResolvedAction`, `UKataActionInstance`, `UKataActionComponent`, `KataPropertyOverride`)  
기준: 2026-10-11 작업 트리. #47 측정용 TRACE 스코프와 이 구현을 함께 커밋한다

## 배경과 결론

`UKataActionAssetBase::Resolve`는 재생·시작 판정마다 부모 Template과 액션을 새로 병합하며 UObject를 만들고 Instanced 객체를 복제한다.
CAPCOM REDox의 원칙(공유 데이터는 읽기 전용으로 한 번 만들고, 필요할 때만 해석하고, 바뀐 부분만 다시 만든다)을 참고해
에셋별 해석 템플릿 캐시를 검토했다([#47](https://github.com/jaykop/Kata/issues/47)).

측정해 보니 해석 비용은 게임 스레드의 약 0.3%였고, 그중 2/3는 결과를 버리는 시작 판정이었다.
2026-10-11 사용자가 전체 캐시는 보류하고 두 개선만 진행하기로 정했다. 시작 판정은 태스크·명령을 복제하지 않는 전용 경로로 처리하고,
실행용 해석에서는 조건·명령의 이중 복제를 없앴다. 사용자가 빌드와 변경 전 동작이 유지되는지 확인했다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 |
|---|---|---|
| 시작 판정 | `CanPlayKataAction`이 `Resolve`로 태스크까지 전부 복제한 뒤 판정 결과만 쓰고 버렸다 | `UKataActionAssetBase::ResolveForStartCheck`로 판정한다. 태스크는 원본을 읽고 Modify 오버라이드 대상만 복제하며, 명령은 다루지 않는다 |
| 조건·명령 이중 복제 | `MakeEffectiveSettings`가 Instanced 설정을 복제하고 `ResolveChain`이 그 사본을 다시 복제했다 | 실행용 해석의 Effective 사본은 원본을 참조만 하고, 결과 소유 사본은 한 번만 만든다(R0) |
| 판정 결과 오용 | 해당 없음 | 판정 전용 결과는 `bStartCheckOnly`가 켜지고 `UKataActionInstance::InitializeInstance`가 `InvalidDefinition`으로 거절한다 |
| 측정 스코프 | 없었다 | `Resolve`·`ResolveForStartCheck`·`MakeEffectiveSettings`·`ResolveChain`·`CanPlayKataAction`·`StartResolved`에 `TRACE_CPUPROFILER_EVENT_SCOPE`를 둔다 |

## 측정

2026-10-10 에이전트가 사용자 요청으로 측정했다. 에디터 DebugGame PIE, `LV_TestMap`에서 런타임 스포너로 StarvedHound 30체를 기본 StateTree AI로 생성하고 약 46초·916프레임을 기록했다.
10체 기록은 분석 단계의 버퍼 경고로 프레임이 하나만 잡혀 쓰지 않았다. 결과는 [#47 댓글](https://github.com/jaykop/Kata/issues/47#issuecomment-6098814880)에 게시했다.

| 값 | 결과 |
|---|---|
| `Resolve` 호출 | 1,584회(시작 판정 1,056 + 재생 528), 평균 0.090ms, 최대 0.333ms |
| 초당 비용 | 약 34회, 3.1ms/s(게임 스레드의 약 0.3%) |
| 해석 1회당 객체 | 6개(Effective 사본, 해석 결과, 태스크 3개, Hit Handler 1개). 초당 약 200개 단명 객체 |
| GC | 측정 구간 안에 엔진이 스스로 실행한 GC가 없어 빈도 영향은 미측정 |
| 누수 | PIE 종료 후 GC에서 해석 결과 0개 |

DebugGame 수치라 Development에서는 더 작을 수 있다. 변경 후 재측정은 사용자 결정으로 생략했다.

## 주요 결정과 이유

### 계획 단계의 질문과 결과

| 질문 | 결과 |
|---|---|
| 해석 비용이 캐시가 필요할 만큼 큰가 | 계획의 판단 기준(게임 스레드 1%)보다 작았다. 전체 캐시는 보류했다 |
| 해석 결과의 설정 객체를 재생 간에 공유할 수 있는가 | 2026-10-10 소스 대조로 네이티브 Task·Condition·Command·Hit Handler는 설정 객체에 실행 상태를 쓰지 않음을 확인했다. Command는 `RunningInstance`를 `Run` 동안만 `TGuardValue`로 기록한다. 복제가 필요한 곳은 편집기용 해석과, 규칙을 어기고 설정 변수를 바꾸는 BP 파생 객체뿐이다 |
| 캐시를 둔다면 어떻게 무효화하는가 | KataGraph는 저장 시점에 실행 사본을 굽고 `CompiledGeneration`·`CompiledDependencies`로 의존 세대를 비교한다. 액션은 매번 해석해 편집이 즉시 반영되므로, 캐시를 둔다면 저장 시 굽기보다 Transient 세대 값을 부모 Template과 비교하는 메모리 지연 캐시가 맞다고 정리했다. 보류했으므로 구현하지 않았다 |
| 측정 스코프를 남길지 | 이 커밋에 남긴다. Insights의 cpu 채널이 켜졌을 때만 기록된다 |

### 시작 판정 경로

- 판정 결과는 `Resolve`와 같아야 한다. 태스크 설정·의존성·쿨다운·반복 길이 오류처럼 시작을 막는 Error 진단은 그대로 계산한다.
  판정 경로에서는 빈 Pre/Post Command 경고만 나오지 않는다. 경고는 판정에 쓰이지 않는다.
- 태스크는 copy-on-write로 다룬다. 원본을 읽기만 하고, Modify 오버라이드가 프로퍼티를 덮어쓰는 태스크만 그 시점에 복제한다. 결과의 `Tasks`에는 담지 않는다.
- `StartCondition`은 원본 조건 객체를 가리킨다. 조건 평가에 부작용이 없다는 규칙에 기댄다.
- 해석 모드(Runtime·Editing·StartCheck)를 헤더에 노출하지 않으려고 `ResolveChain`을 private static 멤버에서 cpp 내부 함수로 옮겼다. 공개 API는 `ResolveForStartCheck` 하나만 늘었다.
- 공개 `MakeEffectiveSettings`는 편집 화면이 사본의 Instanced 객체를 직접 고치므로 계속 복제한다. 참조만 복사하는 경로는 `KataPropertyOverride::CopyOverriddenProperty`의 `bDuplicateInstanced` 인자로 둔다.

### 채택하지 않은 대안

- 전체 템플릿 캐시: 비용이 작아 지금은 이득보다 무효화(에디터 편집, Undo, BP 재컴파일)와 공유 규칙 변경의 부담이 크다.
- 판정에서 타임라인 오류 검사 생략: 더 가볍지만 오류가 있는 액션이 판정을 통과해 재생에서 실패하게 된다. 판정 결과를 바꾸지 않는 쪽을 택했다.

## 근거

- [KataActionAssetBase.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Action/KataActionAssetBase.cpp): `ResolveForStartCheck`, `MakeEffectiveSettingsInternal`, `ResolveChain`의 모드별 처리와 `EnsureOwnedTask`.
- [KataPropertyOverride.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Action/KataPropertyOverride.cpp): `bDuplicateInstanced`가 false일 때의 참조 복사.
- [KataActionInstance.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataActionInstance.cpp): `bStartCheckOnly` 거절.
- [KataActionComponent.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataActionComponent.cpp): `CanPlayKataAction`의 판정 전용 해석.
- UE 5.8 `UObjectGlobals.cpp`: `StaticDuplicateObjectEx`의 `QUICK_SCOPE_CYCLE_COUNTER(STAT_StaticDuplicateObject)`. Insights에서 보려면 `-statnamedevents`가 필요하다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 변경 전 비용 | Insights 트레이스, 에이전트(2026-10-10, 사용자 요청) | 위 측정 표 | GC 빈도 영향, Development 수치 |
| 전체 변경 | 사용자 빌드·실행(2026-10-11) | 변경 전과 같이 동작한다고 보고. 확인을 요청한 항목은 AI Group·PC 콤보·피격 반응·프리뷰였고, 항목별 결과는 따로 받지 않았다 | Game 타깃 빌드, 변경 후 비용 수치 |
| `StartCondition`이 있는 액션 | 사용자 보고(2026-10-11) | 샘플에 해당 액션이 없다 | 조건이 걸린 액션의 판정·재생 |

에이전트는 이 구현의 빌드·테스트를 실행하지 않았다.

## 남은 제한과 후속 작업

- AI Send Trigger 태스크는 그래프가 트리거를 받지 않으면 구간 동안 매 Tick 다시 보낸다. 즉시 전이 대상이 쿨다운·태그·차단으로 거절되는 동안 AI마다 프레임당 해석 한 번과 `LogKata` 로그, 에디터 디버그 기록이 생긴다. 재시도 정책 변경은 별도 결정이 필요하다. 후속: [#57](https://github.com/jaykop/Kata/issues/57)에서 게임플레이 거절이면 그 구간에서 더 보내지 않도록 바꿨다([기록](2026-10-11-AI-Send-Trigger-Retry.md)).
- 재생 경로는 여전히 재생마다 태스크를 복제한다. 비용이 문제가 되면 위 메모리 지연 캐시를 새 이슈로 다시 검토한다.
- BP 파생 Task·Command가 설정 변수를 실행 중에 바꾸는 경우는 지금 재생마다 사본이라 드러나지 않는다. 공유 캐시를 도입할 때 규칙을 정해야 한다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [연결 이슈 #47](https://github.com/jaykop/Kata/issues/47) | 측정 결과 댓글 게시 완료. 구현·확인 결과와 종료 댓글은 사용자 확인 후 게시 |
| [런타임 사용법](../manual/Runtime-Usage.md) | `CanPlayKataAction`의 판정 전용 해석과 `ResolveForStartCheck` 결과의 실행 거절 |
| [태스크 제작](../manual/Task-Authoring.md) | 영향 없음. 재생 경로의 명령은 여전히 해석마다 복제한다 |
| 해석 결과 캐시 계획(`docs/plan/Resolved-Action-Cache-Plan.md`) | #47을 닫으며 결정과 질문의 결과를 이 기록으로 옮기고 삭제했다 |
