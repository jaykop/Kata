# Play Montage 포즈 탐색 구현

작성: 2026-09-24  
갱신: 2026-09-24  
유형: 구현 기록  
대상: KataEditor 몽타주 포즈 탐색, KataRuntime의 이전 프리뷰 보정 API  
기준: 사용자 기존 미커밋 변경 위에 이번 소스 수정을 반영한 작업 트리

## 배경과 결론

[진단](2026-09-24-Montage-Scrub-Diagnosis.md)의 기존 탐색은 월드 재실행과 활성 몽타주에 의존했다.
이번 변경은 타임라인 시각에서 몽타주 에셋 위치를 직접 구하고 UE 5.8 UAnimPreviewInstance로
해당 위치의 포즈를 평가한다. 일반 Play는 장면을 다시 시작한다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 탐색 입력 | 액션 실행 후 1/60초씩 목표 시각까지 월드를 진행 | 편집용 해석 결과에서 현재 시각의 Play Montage를 찾고 포즈를 바로 평가 |
| 섹션 위치 | Next Section 연결을 따라 경과 시간을 변환 | Start Section 시작 위치 + 태스크 경과 시간 × Play Rate × RateScale을 에셋 길이로 제한 |
| 프리뷰 애니메이션 | 실행 중 AnimBP의 몽타주 위치·블렌드에 의존 | 메시를 UAnimPreviewInstance로 전환하고 전체 섹션 프리뷰를 준비해 위치를 직접 지정 |
| 상태 전환 | 끝난 액션에서는 보정이 건너뛰어지고 Idle Tick이 화면을 덮을 수 있음 | 탐색 중 월드 Tick을 멈춤. Stop에서는 메시 설정을 복원하고 Play에서는 장면을 새로 시작 |
| 이전 API | Runtime 태스크에 편집기 탐색용 SyncToKataTime과 수동 섹션 순회 계산 존재 | 호출 경로가 없어져 함께 제거. 게임 실행의 Montage_Play·GAS 처리와 종료 정책은 유지 |

## 주요 결정과 이유

- 몽타주 표시 구간은 Play Montage 태스크의 시작부터 끝까지다. 순간·Single Frame은 시작 시각에만 표시한다.
  몽타주가 태스크보다 짧으면 마지막 에셋 시각으로 제한한다. 시작 전·끝 뒤에는 원래 메시 설정으로 돌아간다.
- 여러 태스크가 겹치면 시작 시각이 가장 늦은 항목을 표시한다. 같은 시작 시각이면 해석된 배열의 나중
  항목이 우선한다. 하나의 단일 에셋 프리뷰에서 서로 다른 몽타주를 합성하지 않기 위한 결정이다.
- 스크럽은 저작 시각을 보여준다. 의존성 대기로 늦어진 실제 태스크 시작 시각, 몽타주 종료 정책,
  게임플레이 부작용과 누적 루트 모션은 재현하지 않는다. 탐색 중 Notify도 발생시키지 않는다.
- 스크럽 뒤 Play는 0초에서 새 액션을 시작한다. 포즈만 이동한 상태를 액션 실행 상태로 간주해 이어서
  재생하면 타이밍과 부작용이 어긋나기 때문이다. 이 결정은 [후속 보완 2](#후속-보완-2--재생-헤드에서-재생과-캡슐-이동)에서
  실제 실행을 재생 헤드까지 동기 진행하는 방식으로 대체했다.
- UAnimPreviewInstance는 KataEditor에서만 사용한다. 런타임 모듈에 AnimGraph 의존성을 넣지 않았다.

## 근거

- [SKataPreviewViewport.cpp](../../Plugins/Kata/Source/KataEditor/Private/SKataPreviewViewport.cpp): Seek, ShowMontagePose, RestorePosePreview, TickSimulation.
- [KataActionEditor.cpp](../../Plugins/Kata/Source/KataEditor/Private/KataActionEditor.cpp): 편집용 해석 결과를 Seek에 전달.
- [KataEditor.Build.cs](../../Plugins/Kata/Source/KataEditor/KataEditor.Build.cs): AnimGraph Private 의존성.
- [KataTask_PlayMontage.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Tasks/KataTask_PlayMontage.cpp): 게임 실행 경로와 종료 정책 유지.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 소스 구현 | 에이전트의 수정 | 직접 포즈 평가 경로와 이전 보정 제거 | 컴파일·UHT·실제 메시 평가 |
| 빌드·테스트·UI | 미실시 | 실행 결과 없음 | 단일/다중 섹션, 역방향, 종료 후 탐색, 일반 Play 복귀 |

## 남은 제한과 후속 작업

아래의 최초 구현 제한 중 루트 모션 이동 누락은 다음 후속 보완으로 대체했다.

첫 Slot을 쓰는 엔진 단일 에셋 프리뷰이므로 게임 AnimBP의 레이어·IK·다중 Slot 합성과 화면 결과가
다를 수 있다. 탐색은 본 포즈를 목표로 하며 루트 모션 누적 이동·충돌·다른 태스크의 월드 효과는
재현하지 않는다. 사용자 빌드와 에디터 화면 확인 후 실제 결과를 문서에 반영해야 한다.

## 후속 보완 — 루트 모션 이동과 일시 정지 재개

2026-09-24 사용자가 루트 모션 몽타주의 위치 이동 누락과, 일반 재생을 일시 정지했다 재생하면 간혹
처음으로 돌아가는 현상을 보고했다. 첫 구현은 본 포즈만 평가했으므로 루트 모션 이동 적용이 빠져 있었다.

- UE 5.8의 UDebugSkelMeshComponent::ConsumeRootMotion도 정지 상태의 스크럽에서는 루트 모션을
  별도로 추출해 적용한다. 같은 UE::Anim::ExtractRootMotionFromAnimationAsset을 사용해
  Start Section 시작 위치부터 현재 몽타주 위치까지의 이동·회전을 구하도록 보완했다.
- 추출 결과는 저장해 둔 메시의 원래 상대 Transform에 적용한다. 이전 커서 위치의 결과에 더하지 않아
  왕복 탐색·반복 입력·다른 태스크 전환에서 이동이 누적되지 않는다. 구간 밖과 Stop에서 원래 Transform을 복원한다.
- 일반 Pause는 실행 인스턴스를 유지하며 Play는 이를 이어 쓴다. 반면 Current Time의 두 콜백은
  값이 그대로인 확정에도 Seek를 호출해 ResetScene으로 인스턴스를 잃을 수 있었다.
  SeekFromTimeInput에서 동일 값과 비유한 값을 걸러내고 표시 정밀도를 6자리로 늘렸다.
  입력 확정 시 포커스도 해제한다. 실제 시간 탐색 뒤 Play의 원점 재생 정책은 별개다.

루트 모션은 메시 이동·회전이며 캡슐 충돌·CharacterMovement 시뮬레이션을 수행하지 않는다.
재시작 문제는 소스에서 확인한 불필요한 Seek 경로를 차단했으며 사용자 증상과의 동일성은 실행 미확인이다.
이번 보완의 빌드·테스트·UI 재현은 에이전트가 수행하지 않았다. 최초 수정의 완료 보고는 이 보완의 검증 근거가 아니다.

## 후속 보완 2 — 재생 헤드에서 재생과 캡슐 이동

2026-09-24 사용자가 두 가지를 보고했다. 시간 바를 옮긴 뒤 Play를 누르면 그 시점이 아니라 처음부터 재생된다.
루트 모션 몽타주를 탐색하면 캐릭터 캡슐은 제자리에 있고 메시만 움직인다. 두 현상 모두 버그가 아니라
위 구현의 정책·제한이었다. 사용자가 다음 방식으로 바꾸기로 결정했다. 동기 진행 한도는 액션 전체 길이로 정했다.

| 항목 | 이전 | 변경 |
|---|---|---|
| 스크럽 뒤 Play | 0초부터 새로 재생 | 0초부터 시작한 뒤 다음 프레임에 재생 헤드 시각까지 1/60초 단계로 한 프레임 안에서 동기 진행하고 이어 재생 |
| 탐색 루트 모션 | 메시 상대 Transform에 적용 | 메시가 루트 모션만큼 움직였을 때의 액터 Transform을 역산해 캡슐째 이동. 메시 상대 Transform은 유지 |
| 조작 기록 기준 | 탐색으로 액터가 옮겨져도 기준 유지 | 탐색이 액터를 옮길 때마다 SyncCommitBaseline으로 기준 갱신 |

- 동기 진행은 한 엔진 프레임 안에서 월드를 여러 번 진행한다. USkeletalMeshComponent::ShouldTickPose는
  PoseTickedThisFrame(GFrameCounter 비교)으로 두 번째 진행부터 포즈를 건너뛰므로, 이 구간에만 Self·Target 메시의
  bIsAutonomousTickPose를 켠다. 엔진이 한 프레임 여러 번 포즈 진행을 허용하는 예외이며, CharacterMovement의
  TickCharacterPose도 이 플래그를 켰다 끄므로 루트 모션 경로에서 포즈가 두 번 진행되지 않는다.
  위 구현이 제거한 Seek 시뮬레이션과 달리 드래그마다 실행하지 않고 Play를 누를 때 한 번만 실행한다.
- 동기 진행은 버튼 입력 처리 중이 아니라 다음 TickSimulation에서 수행한다. 입력 콜백 안에서 월드를 진행하지 않기 위해서다.
- 역산 식은 새 액터 = 메시 상대⁻¹ × 루트 모션 × 메시 상대 × 원래 액터다. 월드 Transform = 상대 × 액터 관계에서 나온다.
  메시 상대 Transform에 비균등 스케일이 있으면 역행렬이 근사가 된다.
- SyncCommitBaseline 전에는 조작 대상이 선택된 상태에서 뷰포트를 클릭만 해도 탐색으로 옮겨진 위치가
  Preview Transform에 기록될 수 있었다. 일반 재생에서 루트 모션으로 이동한 뒤 클릭하는 경우에도 같은 경로가 있으며,
  이번 변경 범위에 넣지 않았다.

이번 보완의 빌드·테스트·UI 재현은 에이전트가 수행하지 않았다.

## 후속 변경 3 — 탐색을 실행 시뮬레이션으로 대체

2026-09-24 사용자가 두 가지를 보고했다.

1. 시간 바를 계속 드래그하면 포즈가 갱신되지 않는다.
2. 탐색한 루트 모션 위치가 재생했을 때와 다르다.

### 원인

- 1은 계산이 아니라 그리기 문제였다. 타임라인은 마우스를 누를 때 CaptureMouse만 반환했다. 그러면 Slate가
  버튼을 누르는 동안 반응성 모드(SlateApplication.cpp의 EnterResponsiveMode)에 들어간다. 이 모드에서
  UEditorEngine은 bNeedsRedraw가 켜진 뷰포트만 그린다(EditorEngine.cpp). Seek가 부르던 Invalidate는
  SEditorViewport의 위젯 함수라 bNeedsRedraw를 켜지 않았다. Current Time 스핀박스 드래그도 같은 경로다.
- 2는 직접 포즈 평가의 구조적 한계다. "Montages Only" 모드에서는 몽타주 루트 모션을 블렌드 가중치 없이
  적용하므로(AnimInstance.cpp) 추출 값 자체는 같다. 차이는 적용 방식에서 생긴다.
  - 첫 탐색이 장면을 다시 만든 뒤 월드를 멈춰 스폰 높이에 뜬 상태를 기준으로 삼았다.
  - 걷는 중 CharacterMovement는 루트 모션의 Z 대신 바닥을 따른다(ConstrainAnimRootMotionVelocity).
  - 충돌을 계산하지 않았다.
  - 태스크 구간 밖에서 원래 배치로 되돌렸다.

### 변경

| 항목 | 이전 | 변경 |
|---|---|---|
| 탐색 | UAnimPreviewInstance로 에셋 포즈를 평가하고 루트 모션을 역산 | 드래그로 바뀔 때마다 실제 실행을 시뮬레이션. 프레임당 마지막 요청 한 번, 앞으로는 이어서, 뒤로는 0초부터 |
| Play | 예약 재생(PendingResumeTime) | 살아 있는 인스턴스를 그대로 이어 재생 |
| 드래그 중 그리기 | 반응성 모드에서 멈춤 | 탐색 캡처에 PreventThrottling, Seek와 탐색 처리에서 클라이언트 Invalidate |
| 소리 | 탐색 중 월드를 진행하지 않아 없음 | 진행 중 UWorld::bAllowAudioPlayback을 끔 |
| 의존성 | KataEditor에 AnimGraph | 제거 |

- 앞의 후속 보완 1·2에서 도입한 직접 포즈 평가, 액터 역산, 예약 재생은 이 변경으로 대체했다. 당시 기록은 보존한다.
- SyncCommitBaseline은 유지한다. 시뮬레이션한 루트 모션이 액터를 옮겨도 같은 기록 문제가 생기기 때문이다.
- 탐색이 액션 끝에 닿아도 Repeat·자동 초기화 대상으로 표시하지 않는다. 끝 포즈를 보고 있어야 하기 때문이다.
- 끝까지 진행해 끝난 인스턴스에서 끝 이후를 탐색하면 다시 실행하지 않는다. 끝 너머를 드래그하는 동안
  마우스 이동마다 처음부터 재실행하지 않게 하기 위해서다.

### 후속 수정 — 메시가 멈추는 문제

사용자가 변경 3을 확인하며 세 가지를 보고했다. Reset 뒤에도 Idle 없이 멈춘다. 드래그하면 캡슐은 이동하지만
메시는 멈춘다. 클릭 탐색은 거의 이동하지 않고 드래그는 이동한다.

- 원인은 월드 시간과 렌더링 시각의 기준 차이다. 스켈레탈 메시는 bRecentlyRendered일 때만 본을 다시 계산한다
  (USkinnedMeshComponent::ShouldUpdateTransform). 예외는 AlwaysTickPoseAndRefreshBones다. bRecentlyRendered는
  LastRenderTime > World->TimeSeconds - 1.0으로 판정한다. 그런데 실시간 에디터 뷰포트는 UseAppTime에 따라
  렌더링 시각을 앱 경과 시간으로 기록한다(FEditorViewportClient::Draw). 탐색은 한 프레임에 월드 시간을 최대
  액션 길이만큼 진행하므로, 계속 드래그하면 프리뷰 월드 시간이 앱 경과 시간을 앞지르고 판정이 계속 거짓이 된다.
  포즈 진행과 루트 모션 추출은 계속되어 캡슐은 움직이지만 본은 갱신되지 않는다. Reset은 월드를 다시 만들지 않아
  상태가 남는다. 클릭 탐색이 거의 움직이지 않는 현상은 렌더링 여부로 포즈 진행을 제한하는 캐릭터 설정일 때
  같은 판정 때문인 것으로 추정한다. 프리뷰 캐릭터의 실제 설정은 확인하지 않았다.
- ResetScene이 스폰한 Self·Target 스켈레탈 메시를 AlwaysTickPoseAndRefreshBones로 바꿨다. 월드 시간을 되돌리는
  방법은 GE 지속 시간 등 월드 시간에 기대는 시스템을 깨뜨릴 수 있어 쓰지 않았다.
- CharacterMovement의 TickCharacterPose는 끝에서 bIsAutonomousTickPose를 이전 값이 아니라 false로 되돌린다.
  SimulateTo가 이 플래그를 루프 전에 한 번만 켜면, 루트 모션이 한 번 돈 뒤에는 그 프레임의 루트 모션 밖 포즈 진행이 멈춘다.
  단계마다 다시 켜도록 고쳤다. 이 처리는 아래 두 번째 후속 수정에서 제거했다.

### 두 번째 후속 수정 — 한 프레임 안의 추가 World Tick이 액터를 진행하지 않음

위 수정 뒤에도 클릭 탐색은 거의 이동하지 않았다. 오른쪽에서 왼쪽으로 드래그하면 아무 변화가 없었고,
왼쪽에서 오른쪽으로 드래그할 때만 정상이었다.

- 앞으로 드래그하면 프레임마다 1~2단계만 진행한다. 클릭 점프와 뒤로 드래그(재시작)는 한 프레임에 많은 단계를 몰아서 진행한다.
  이 차이가 원인을 가리켰다. FTickFunction::QueueTickFunction은 Tick 함수마다 방문한 GFrameCounter를 기록하고,
  같은 프레임에는 다시 큐에 넣지 않는다(TickTaskManager.cpp). 그래서 한 엔진 프레임의 두 번째 World Tick부터는
  액터·컴포넌트 Tick이 전혀 돌지 않았다. Kata 시각만 StepWorld의 직접 TickInstance 보완 경로로 앞서 나갔다.
  앞서 원인으로 본 ShouldTickPose의 PoseTickedThisFrame은 이 제한의 일부였다. FTimerManager도 같은 방식으로 프레임당 한 번만 진행한다.
- 엔진 자동화 헬퍼(AutomationCommon.cpp의 TickWorld)처럼 단계마다 GFrameCounter를 올려, 각 단계를 새 게임 프레임으로 만들었다.
  프레임 번호는 늘어나기만 하므로 엔진 루프의 증가와 충돌하지 않는다. 같은 엔진 프레임에서 이미 진행된 다른 월드가
  다시 진행되지는 않는다. 다만 프레임 번호로 "이번 프레임에 이미 했는지"를 기억하는 다른 에디터 캐시가 한 번 더 계산할 수는 있다.
- 이 방식으로 bIsAutonomousTickPose 처리가 필요 없어져 제거했다. AlwaysTickPoseAndRefreshBones 설정은 월드 시간이
  앱 경과 시간을 앞지르는 문제 때문에 계속 필요해 유지한다.

### 확인 범위와 남은 제한

- 뒤로 탐색하는 비용은 목표 시각에 비례한다(1초당 60단계). 실제로 끊기는 정도는 측정하지 않았다.
- Blend In 구간에서는 몽타주 에디터의 단독 포즈와 다르게 보인다. 게임 실행과 같은 결과다.
- 빌드·테스트·UI 재현은 에이전트가 수행하지 않았다.

## 연관 문서 반영

최종 방식과 엔진 제약은 [프리뷰 시간 탐색의 실행 시뮬레이션 전환](2026-09-24-Preview-Scrub-Simulation.md)에 정리했다.

| 문서 | 반영 내용 |
|---|---|
| [현재 구현 상태](Implementation-Status.md) | 직접 포즈 평가 경로와 확인 대기 상태. 후속 보완 2의 동기 진행·캡슐 이동. 후속 변경 3의 실행 시뮬레이션 탐색 |
| [에디터 사용법](../manual/Editor-Usage.md) | 시각 매핑, 표시 구간, 일반 재생과의 차이. 탐색 뒤 Play가 재생 헤드에서 이어지는 동작. 후속 변경 3의 탐색 결과·비용 |
| [수정 계획](../plan/Montage-Scrub-Plan.md) | 구현 완료와 실행 확인 대기를 구분. 스크럽 뒤 Play 정책 변경 결정. 탐색 방식 변경 결정 |
| [다음 작업 계획](../plan/Next-Work-Plan.md) | 구현 기록 연결 |
