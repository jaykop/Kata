# Play Montage 타임라인 포즈 탐색 진단

> 2026-09-25 안내: 이 문서는 당시 소스의 진단이다. 여기에 나온 SyncToKataTime·직접 포즈 평가 제안은 현재 API가 아니다.
> 후속 구현과 재변경을 거친 현행 방식은 [실행 시뮬레이션 기록](2026-09-24-Preview-Scrub-Simulation.md),
> 실제 조작과 남은 제한은 [Editor-Usage](../manual/Editor-Usage.md#프리뷰)를 따른다.

작성: 2026-09-24  
갱신: 2026-09-25
유형: 진단  
대상: KataEditor 프리뷰, KataRuntime Play Montage  
기준: 기존 미커밋 변경을 포함한 작업 트리와 로컬 UE 5.8 엔진 소스. 이번 작업에서는 C++ 소스를 수정하지 않았다.

## 배경과 결론

사용자는 시간 바를 앞으로나 뒤로 옮길 때 섹션 연결에 관계없이 해당 시각의 몽타주 포즈를 보고 싶다고 요청했다.
현재 구현은 월드를 목표 시각까지 재실행한 뒤 실행 중인 몽타주 위치를 보정한다.
이 방식은 에셋의 임의 시각을 직접 평가하는 몽타주 에디터와 계약이 다르다.
반복 횟수를 늘리거나 섹션 연결 계산을 보강하는 것만으로는 요구를 충족하지 못한다.

실행 중인 태스크·몽타주의 생존 여부와 독립적인 에디터 포즈 평가 경로를 두고 엔진의
UAnimPreviewInstance를 재사용하는 방향을 제안한다. 수정 구현은 아직 착수하지 않았다.

## 진단 내용

| 항목 | 소스에서 확인한 내용 | 영향 |
|---|---|---|
| D01 시각의 의미 | CalculateMontagePosition은 CompositeSections의 NextSectionName을 순회한다. 연결이 없으면 섹션 끝을 반환하고 순환이면 나머지 시간을 계산한다. | 에셋상 시각과 다른 포즈를 반환한다. 예를 들어 A=[0,1), B=[1,2), C=[2,3]에서 A→C이면 경과 1.5초가 2.5초로 바뀐다. A에서 연결이 끊기면 B·C로 갈 수 없다. |
| D02 실행 상태 의존 | SyncPoseToKataTime은 액션이 Running일 때만 활성 태스크를 순회한다. Play Montage의 SyncToKataTime도 Running 태스크와 자신이 시작한 활성 몽타주 인스턴스가 있어야 한다. | 자연 종료·블렌드 아웃·태스크 종료·액션 종료 뒤에는 위치 보정이 빠진다. ContinueTimeline도 몽타주 인스턴스를 계속 살려 두는 설정은 아니다. |
| D03 재실행 의존 | Seek는 역방향 이동 시 Start→ResetScene으로 장면을 다시 만들고 TickSimulation이 1/60초씩 프레임당 최대 30회 월드를 진행한다. | 헤드는 즉시 움직여도 포즈는 시뮬레이션 완료까지 지연된다. 멀리 이동한 뒤 다시 뒤로 드래그하면 재실행이 반복된다. |
| D04 블렌드·AnimBP 의존 | SyncToKataTime은 Montage_SetPosition만 호출한다. 마지막 TickAnimation(0)은 시간 진행에 따른 Blend In을 채워 주지 않으며 원래 AnimBP의 Slot 경로를 그대로 쓴다. | 시작 직후 가중치가 낮거나 0이면 몽타주 포즈가 약하게 보이거나 보이지 않을 수 있다. Slot이 출력에 연결되지 않은 경우도 직접 포즈 프리뷰와 다르다. 해당 에셋의 AnimBP 설정은 이번에 확인하지 않았다. |
| D05 종료 뒤 정지 화면 | TickSimulation은 액션이 없거나 종료되면 Seek·Pause 의도보다 먼저 Idle용 World Tick 분기로 들어간다. | 끝으로 탐색한 뒤에도 월드가 계속 진행할 수 있다. Seek의 끝 이후 처리에서 Paused 문자열을 설정하는 것만으로 이를 막지 못한다. |

`bIsAutonomousTickPose` 보정은 한 엔진 프레임 안의 여러 포즈 Tick을 위한 처리다.
D01·D02·D04의 시간 매핑, 인스턴스 수명, 가중치 문제를 해결하지는 않는다.

## 엔진 에디터와의 비교

로컬 엔진 루트: `C:/Program Files/Epic Games/UE_5.8`.

- `Engine/Source/Editor/Persona/Private/AnimTimeline/AnimModel.cpp:99`:
  FAnimModel::SetScrubPosition은 PreviewInstance 재생을 멈추고 프레임 시각을 초로 바꿔 SetPosition에 직접 전달한다.
- `Engine/Source/Editor/Persona/Private/SAnimMontageScrubPanel.cpp:125`:
  별도 몽타주 스크럽 패널의 OnValueChanged는 MontagePreview_JumpToPosition을 호출한다.
- `Engine/Source/Editor/AnimGraph/Private/AnimPreviewInstance.cpp:880`:
  MontagePreview_JumpToPosition은 먼저 SetPosition(NewPosition, false)를 호출한다.
  이후 섹션 처리도 있지만 이는 후속 프리뷰 재생·루프 설정이며 요청 시각을 섹션 경과 시간으로 변환하지 않는다.
- 같은 파일의 `MontagePreview_PreviewNormal`과 `MontagePreview_PreviewAllSections`는
  프리뷰 몽타주를 준비하고 SetWeight(1.0f), 재생 여부 설정, 블렌드 아웃 조정을 수행한다.
- `Engine/Source/Runtime/Engine/Private/Animation/AnimSingleNodeInstance.cpp:359`:
  SetPositionWithPreviousTime은 입력 시각을 에셋 길이로 Clamp하고 프록시와 활성 몽타주의 위치를 설정한다.
  Notify 발생은 인자로 제어한다. 최신 타임라인의 기본 SetPosition 호출과 별도 몽타주 스크럽 패널의
  false 지정은 다르므로, 엔진 스크럽 전체가 항상 Notify를 끈다고 해석하면 안 된다.
- `Engine/Source/Runtime/Engine/Private/Animation/AnimSingleNodeInstanceProxy.cpp`:
  단일 에셋용 평가 경로는 ActiveMontageSlot을 사용하며 additive의 PreviewBasePose도 처리한다.
  임의의 게임 AnimBP에 위치만 설정하는 것과 동일하지 않다.

타입과 공개 함수는 [Epic의 UAnimPreviewInstance API](https://dev.epicgames.com/documentation/unreal-engine/API/Editor/AnimGraph/UAnimPreviewInstance)에서도 확인했다.
세부 동작 판단은 위 로컬 UE 5.8 소스를 근거로 했다.

## 근거

- [SKataPreviewViewport.cpp](../../Plugins/Kata/Source/KataEditor/Private/SKataPreviewViewport.cpp): Seek, TickSimulation, SyncPoseToKataTime.
- [KataTask_PlayMontage.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Tasks/KataTask_PlayMontage.cpp): SyncToKataTime, CalculateMontagePosition, 종료 정책.
- [KataTaskInstance.h](../../Plugins/Kata/Source/KataRuntime/Public/Runtime/KataTaskInstance.h): 프리뷰용 SyncToKataTime과 실제 시작 시각의 계약.
- [KataEditor.Build.cs](../../Plugins/Kata/Source/KataEditor/KataEditor.Build.cs): 현재 AnimGraph 의존성이 없다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 현재 코드와 미커밋 변경 | 사용자 요청에 따른 에이전트 소스 진단 | 위 다섯 가지 경로·조건 확인 | 사용자가 보고한 화면에서 어떤 조건이 실제 발현했는지는 미확인 |
| 엔진 에디터 | 로컬 UE 5.8 소스 대조 | 직접 시각 지정과 프리뷰 전용 평가 경로 확인 | Kata 프리뷰 액터에 통합한 결과 |
| 빌드·테스트·UI | 미실시 | 실행 결과 없음 | 전부 |

## 남은 제한과 후속 작업

수정 계획(완료 후 삭제)에 구현 순서와 미확정 경계 정책을 정리했다.
후속 소스 변경은 [구현 기록](2026-09-24-Montage-Scrub-Implementation.md)에 남겼다.
다중 섹션은 직접 위치 평가로 해결할 수 있지만, 겹치는 여러 태스크·다중 Slot의 합성,
루트 모션으로 이동한 액터의 위치와 게임 실행 상태 복원은 별도의 계약이 필요하다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [현재 구현 상태](Implementation-Status.md) | 현재 스크럽의 한계와 진단 링크, 중복된 단계 수 설명 수정 |
| 수정 계획(완료 후 삭제) | 엔진 프리뷰 재사용 제안과 완료 조건 |
| 다음 작업 계획(2026-09-24 삭제, 이슈로 대체) | 이번 진단과 미착수 수정 계획 연결 |
| [에디터 사용법](../manual/Editor-Usage.md) | 이번에는 기능·조작 변경이 없어 미수정. 구현 때 포즈 탐색 계약과 제한 반영 |
| [문서 목록](../README.md) | 새 진단·계획 등록 |
