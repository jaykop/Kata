# 시작 프레임의 Kata 시각과 몽타주 동기화

작성: 2026-10-15  
갱신: 2026-10-15  
유형: 진단 / 결정 기록  
대상: KataRuntime `UKataActionInstance::StartInstance`, `GetStartFrameCatchUpTime`, `FinishFrameAdvance`  
기준: [#115](https://github.com/jaykop/Kata/issues/115) 구현 작업 트리. 다른 작업의 미커밋 변경은 확인 대상에서 제외했다

## 배경과 결론

[#108](https://github.com/jaykop/Kata/issues/108) 회귀 확인 중 `t.MaxFPS 20`에서 흑기사 콤보 2~4타의 Hit Trace 판정이 휘두름 앞부분을 놓쳤다. 프리뷰와 고프레임에서는 보이지 않았다.
원인은 Hit Trace가 아니었다. 액션이 시작한 프레임에 몽타주는 진행하는데 Kata 시각은 진행하지 않아, 이후 내내 Kata 시각이 몽타주보다 한 프레임 늦었다.
액션을 시작한 프레임에 Avatar 메시가 아직 포즈를 갱신하지 않았으면, 시작 직후 그 프레임 DeltaTime만큼 Kata 시각을 진행하도록 고쳤다.
현재 규칙은 [런타임 사용법](../manual/Runtime-Usage.md)의 시작 처리 단락에 있다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 |
|---|---|---|
| 증상 | 20fps 판정 시작 Tick 로그에서 1타는 Kata 시각 0.70·몽타주 0.70, 2~4타는 Kata 0.70·몽타주 0.75(4타 1.10·1.15). 판정 창 첫 Tick의 칼끝 이동이 프리뷰 238cm, PIE 4타 23cm | 수정 뒤 플레이어·흑기사 콤보 모두 Kata 시각과 몽타주 위치가 같다 |
| 시작 프레임 처리 | `StartInstance`가 `LastTickFrame = GFrameCounter`로 시작 프레임을 갱신 완료로 표시해 그 프레임에 진행하지 않았다 | 표시는 유지하고, 조건이 맞으면 시작 갱신 안에서 그 프레임 DeltaTime만큼 더 진행한다 |
| 어긋나는 시작 경로 | 실행 Subsystem의 PreActorTick 안 그래프 전이(`HandleActionEnded`·`HandleTransitionWindowOpened`), 메시보다 먼저 Tick하는 플레이어 입력 처리 | 두 경로 모두 따라잡는다. 메시 포즈 갱신 뒤에 시작하는 경로(흑기사 1타)는 원래 어긋나지 않아 그대로다 |
| 종료·회차 처리 | `TickInstance` 안에만 있었다 | `FinishFrameAdvance`로 분리해 따라잡기 뒤에도 같은 처리를 한다 |

## 주요 결정과 이유

- **조건은 메시 포즈 갱신 여부로 판단한다.** 몽타주는 `Montage_Play` 뒤 같은 프레임의 메시 포즈 갱신에서 진행한다. 그래서 시작 시점에 Avatar의 첫 `USkeletalMeshComponent`가 `PoseTickedThisFrame()`이면 따라잡지 않고, 아니면 따라잡는다.
  메시는 몽타주 태스크와 같은 규칙으로 고른다. 엔진 헤더 주석상 이 함수는 CharacterMovement가 루트 모션용으로 포즈를 먼저 갱신한 경우도 반영한다.
- **진행량은 프레임 전체다.** `World->GetDeltaSeconds() × GetCustomTimeDilation()`로 실행 Subsystem과 같은 식이다. 전이에서 이전 액션이 프레임 중간에 끝났어도 몽타주는 그 프레임 전체만큼 진행하므로 Kata 시각도 프레임 전체만큼 진행한다.
- **"Task Tick은 프레임당 최대 한 번"을 유지한다.** 따라잡기는 시작 갱신의 연장이라 Tick 번호와 태스크 Tick 기록을 새로 시작하지 않는다. 시각 0 태스크는 Start → Tick(0)을 그대로 받고, 다음 프레임 Tick의 DeltaTime이 따라잡은 시간을 포함한다.
- **프리뷰와 길이 0 타임라인은 제외한다.** 액션 편집기 프리뷰는 뷰포트가 시뮬레이션 단계를 정한다. 길이 0 타임라인은 기존처럼 재생 요청 안에서 끝난다.

채택하지 않은 대안:

- 실행 Subsystem이 이번 프레임에 새로 시작한 인스턴스를 같은 콜백에서 한 번 더 Tick하는 방식. 전이 경로는 고치지만 Kata Tick 밖에서 메시보다 먼저 시작하는 입력 경로는 고치지 못한다.
- 몽타주 태스크가 첫 Tick에 몽타주 위치를 Kata 시각으로 되감는 방식. 시작 직후 포즈가 한 프레임 멈추고 입력 반응이 한 프레임 늦어진다.
- Hit Trace에서 어긋남을 보정하는 방식. 같은 액션의 연출·캔슬 창 같은 다른 타임라인 태스크는 계속 어긋난다.

## 근거

- [KataActionInstance.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataActionInstance.cpp): `StartInstance`의 따라잡기, `GetStartFrameCatchUpTime`, `FinishFrameAdvance`.
- [KataExecutionWorldSubsystem.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataExecutionWorldSubsystem.cpp): PreActorTick에서 인스턴스를 진행하는 시점과 배율 식.
- [KataTask_PlayMontage.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Tasks/KataTask_PlayMontage.cpp): 몽타주 재생 메시를 고르는 규칙.
- [KataHitSubsystem.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/HitTrace/KataHitSubsystem.cpp): 진단에 쓴 판정 구간 로그(`begin time … montage …`).

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 빌드 | 사용자 Editor 빌드(2026-10-15) | 통과 | Game 타깃 |
| 20fps 콤보 | 사용자 PIE, 에이전트가 MCP로 로그 확인 | 플레이어 콤보와 흑기사 콤보 1~4타 모두 판정 Tick의 Kata 시각과 몽타주 위치가 같다. 히트스톱 직후 Tick도 같다 | — |
| 고프레임·프리뷰 | 사용자 실행, 로그 | 약 144fps와 프리뷰 attack04에서 같다 | — |
| 회귀 | 사용자 실행 보고 | 정상 프레임 전투, 짧은 액션 연속, 히트스톱 직후 전이 이상 없음 | 루프 액션 |

`GetDeltaSeconds()`가 PreActorTick 시점에 그 프레임 값이라는 점은 엔진 cpp가 설치본에 없어 소스로 확인하지 못했고, 로그 일치로 확인했다.

## 남은 제한과 후속 작업

- 메시가 그 프레임에 Tick하지 않는 경우(화면 밖 애니메이션 최적화)는 다루지 않는다. 이때는 따라잡기가 몽타주보다 앞설 수 있다.
- 무기 궤적([#109](https://github.com/jaykop/Kata/issues/109))은 이 수정을 전제로 판정과 같은 구간 경계를 쓴다.

## 연관 문서 반영

| 문서 | 반영 내용 |
|---|---|
| [런타임 사용법](../manual/Runtime-Usage.md) | 시작 프레임 진행 규칙과 확인 상태 |
| [#115](https://github.com/jaykop/Kata/issues/115) | 결과 댓글과 종료 |
