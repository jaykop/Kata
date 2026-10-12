# 시간 배율 요청 관리

작성: 2026-10-12  
갱신: 2026-10-12  
유형: 구현 기록, 결정 기록  
대상: KataFramework `UKataTimeDilationSubsystem`·`UKataTask_TimeDilation`, KataRuntime 실행기, KataEditor 액션 프리뷰, Hit Trace  
기준: 2026-10-12 작업 트리

## 배경과 결론

히트스톱([#72](https://github.com/jaykop/Kata/issues/72)), 저스트 회피 슬로모션([#87](https://github.com/jaykop/Kata/issues/87)), 연출이 같은 시간 배율을 바꾸게 되므로 요청 목록으로 관리하기로 했다([#65](https://github.com/jaykop/Kata/issues/65)).
엔진은 월드 배율 하나와 액터별 `CustomTimeDilation` 하나만 갖고 있어 마지막에 쓴 쪽이 이긴다.

조사 중에 게임 월드의 Kata 실행기가 액터 배율을 따르지 않는다는 점을 확인했다. 실행 Subsystem은 `OnWorldPreActorTick`의 DeltaSeconds(월드 배율만 적용)로 진행하고,
몽타주와 프리뷰의 컴포넌트 Tick은 액터 배율까지 적용된 시간으로 진행한다. Hit Trace는 실행기가 액터 배율을 따른다고 가정하고 있었다.
아무도 액터 배율을 바꾸지 않아 드러나지 않았을 뿐이므로 이번에 함께 고쳤다.

KataFramework에 요청 관리 Subsystem과 구간 동안 배율을 요청하는 Kata 태스크를 추가하고, 코어 실행기와 액션 프리뷰가 액터 배율을 따르게 했다. 사용자가 같은 날 Editor 빌드와 항목별 실행 확인을 마쳤다고 보고했다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 게임 월드 실행기 | 월드 배율만 반영 | `UKataActionInstance::GetCustomTimeDilation()`을 곱해 진행 |
| 실행 기준 액터 | 정의 없음 | `GetTimeDilationActor()`: 인스턴스 Outer인 실행 컴포넌트의 소유 액터, 없으면 Context Owner |
| 프리뷰 보조 진행 | 배율 없이 진행 | 월드·액터 배율을 곱해 진행 |
| 프리뷰 탐색 | 월드 1/60초 단계로 진행 | Kata 시각 1/60초 단계를 배율로 나눈 월드 시간으로 진행, 엔진 최대 프레임 시간으로 자름, 단계 상한 8배 |
| Hit Trace 직전 시각 | Instigator의 배율로 되짚음 | 액션 인스턴스의 기준 액터 배율로 되짚음. 애니메이션 진행 추정은 메시 쪽 Instigator 배율 유지 |
| 요청 관리 | 없음 | `UKataTimeDilationSubsystem`(Game·PIE·EditorPreview) |
| 태스크 | 없음 | `Kata Task: Time Dilation`(Self/World, 하한 0.01) |

## 주요 결정과 이유

모두 2026-10-12 사용자 결정이다.

| 항목 | 결정 | 이유와 버린 대안 |
|---|---|---|
| 배치 | 요청 관리는 KataFramework, 코어는 엔진 값만 읽음 | 코어가 요청 관리를 몰라도 실행기는 엔진 값만 따르면 된다. 코어에 두면 코어 태스크가 바로 요청할 수 있지만 코어 범위가 늘어난다 |
| 합성 | 같은 층은 Priority 최대 하나, 같으면 최근. 층 사이는 엔진 곱셈 | 층 안에서 곱하면 연타 히트스톱이 0.05×0.05로 겹쳐 사실상 멈춘다. 최솟값 규칙은 가속(1 초과) 요청을 표현할 수 없다 |
| 만료 시계 | 배율 무관, 일시정지 중 정지 | 액터 시간으로 재면 배율 0.05인 0.1초 요청이 2초가 된다. 실제 시간으로 재면 메뉴를 연 동안 히트스톱이 끝난다 |
| 일시정지 | 요청에 넣지 않고 엔진 일시정지 사용 | 엔진은 월드 배율 0을 허용하지 않고(Min Global Time Dilation), 일시정지는 입력·UI도 함께 멈춘다. 실행기와 Tickable Subsystem은 엔진 일시정지에서 이미 멈춘다 |
| 우선순위 | 정수 | 사용자 결정 |
| Blueprint API | 제공하지 않음 | 현재 수요가 없다 |
| 복원 값 | 층의 첫 요청 때 기억한 값 | 요청 관리를 거치지 않고 설정해 둔 값을 보존한다 |
| 태스크 대상 | Self·World | 슬로모션처럼 월드 전체를 느리게 하는 연출에 World가 필요하다 |
| 실행 기준 액터 | 실행 컴포넌트의 소유 액터 | 컴포넌트 Tick 경로가 이미 이 액터의 배율을 쓰므로 실행 경로가 일치한다 |
| 태스크 하한 | 0.01 | 구간 끝을 Kata 시각으로 재므로 배율이 0에 가까우면 구간이 끝나지 않는다 |

## 근거

- UE 5.8 `LevelTick.cpp`: 월드 Tick이 DeltaSeconds에 `GetEffectiveTimeDilation()`을 곱하고 `FixupDeltaSeconds`로 자른 뒤, 일시정지가 아닐 때만 `OnWorldPreActorTick`을 보낸다.
- UE 5.8 `Actor.h` `FActorComponentTickFunction::ExecuteTickHelper`: 컴포넌트 Tick에 소유 액터의 `CustomTimeDilation`을 곱한다.
- UE 5.8 `WorldSettings.cpp` `FixupDeltaSeconds`, `BaseGame.ini` `MaxUndilatedFrameTime=0.4`.
- [KataTimeDilationSubsystem.h](../../Plugins/KataFramework/Source/KataFramework/Public/Time/KataTimeDilationSubsystem.h), [KataExecutionWorldSubsystem.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataExecutionWorldSubsystem.cpp), [SKataPreviewViewport.cpp](../../Plugins/Kata/Source/KataEditor/Private/SKataPreviewViewport.cpp).

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 엔진 배율·일시정지 동작 | UE 5.8 소스 읽기(에이전트) | 위 근거와 같음 | 해당 없음 |
| 이번 구현 | 사용자 Editor 빌드·테스트 보고(2026-10-12) | 프리뷰 재생·탐색(Self·World), PIE 몽타주·타임라인 일치, 일시정지 복귀, 기존 액션 회귀 확인으로 보고됨. 첫 빌드의 C4458(지역 변수가 부모 멤버를 가림)은 이름을 바꿔 해결 | Game 타깃 빌드 |

## 남은 제한과 후속 작업

- 요청이 있는 동안 다른 코드가 엔진 배율을 직접 바꾸면 다음 갱신 때 덮어쓴다.
- 프리뷰 탐색은 월드·액터 배율의 곱이 약 0.005보다 작은 구간을 지날 때 단계 상한에 걸릴 수 있다.
- 히트스톱은 [#72](https://github.com/jaykop/Kata/issues/72)에서 이 요청 API로 구현한다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#65](https://github.com/jaykop/Kata/issues/65) | 구현 결과와 확인 대기 상태는 사용자 확인 후 게시 |
| [시간 배율 요청 사용법](../manual/Time-Dilation.md) | 새로 작성 |
| [런타임 사용법](../manual/Runtime-Usage.md) | Kata 시각이 월드·액터 배율을 따르는 규칙 추가 |
| [피격 반응 계획](../plan/Hit-Reaction-Plan.md) | 실행기 수정을 HR9에서 #65로 옮김 |
