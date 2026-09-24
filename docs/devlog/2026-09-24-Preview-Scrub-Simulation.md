# 프리뷰 시간 탐색의 실행 시뮬레이션 전환

작성: 2026-09-24  
갱신: 2026-09-24  
유형: 구현 기록·결정 기록  
대상: KataEditor 프리뷰 시간 탐색(SKataPreviewViewport, SKataTimeline, FKataActionEditor)  
기준: 커밋 0c58911 위의 미커밋 변경. 이 기록과 함께 커밋한다.

## 배경과 결론

Kata 에디터의 타임라인 바를 옮기면 프리뷰가 그 시각의 상태를 보여 줘야 한다. 이 기능은 같은 날 세 가지 방식을 거쳤다.

1. 월드를 목표 시각까지 여러 프레임에 나눠 진행한 뒤 몽타주 위치를 보정했다(Claude). 화면이 목표를 늦게 따라가고,
   블렌드와 루트 모션이 맞지 않았다.
2. UAnimPreviewInstance로 몽타주 에셋 포즈를 바로 평가하고 루트 모션을 계산으로 적용했다(GPT, [구현 기록](2026-09-24-Montage-Scrub-Implementation.md)).
   포즈는 즉시 나왔지만 위치가 재생 결과와 달랐고, 탐색 뒤 Play가 처음부터 시작했다.
3. 드래그로 재생 헤드가 바뀔 때마다 실제 실행을 한 프레임 안에서 시뮬레이션한다(이 기록).

최종 방식은 3이다. 사용자는 탐색 결과가 재생했을 때와 같아야 한다고 판단했고, 드래그할 때마다 시뮬레이션하는 방식을 직접 선택했다.
구현 과정에서 한 엔진 프레임 안에서 월드를 여러 번 진행할 때 걸리는 엔진 제약 세 가지를 찾았다. 아래 "엔진 제약"은
프리뷰나 도구에서 월드를 동기로 진행하는 다른 작업에도 그대로 적용된다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 탐색 | 에셋 포즈 직접 평가와 루트 모션 역산 | Seek는 요청 시각만 기록하고, 다음 TickSimulation이 마지막 요청을 프레임당 한 번 처리한다. 앞으로 가면 살아 있는 인스턴스를 이어서 진행하고, 뒤로 가면 장면을 다시 만들어 0초부터 진행한다. 1/60초 단계로 한 프레임 안에서 끝내며 한도는 액션 전체 길이다 |
| Play | 탐색 뒤에는 0초부터, 한때는 예약 재생 | 탐색 결과가 일시 정지된 실제 인스턴스이므로 그 시각부터 이어 재생한다 |
| 동기 진행 | 한 프레임의 두 번째 World Tick부터 액터가 진행하지 않음 | 단계마다 GFrameCounter를 올려 각 단계를 새 게임 프레임으로 만든다 |
| 프리뷰 메시 | 캐릭터 설정의 Visibility Based Anim Tick Option을 그대로 사용 | ResetScene이 스폰한 Self·Target 스켈레탈 메시를 AlwaysTickPoseAndRefreshBones로 둔다 |
| 드래그 중 그리기 | 마우스를 누르는 동안 뷰포트 갱신이 멈춤 | 탐색 캡처가 PreventThrottling을 반환하고, Seek와 탐색 처리에서 클라이언트 Invalidate로 다시 그리기를 요청한다 |
| 소리 | 없음(월드 정지) | 동기 진행 중에는 UWorld::bAllowAudioPlayback을 꺼서 사운드 노티파이가 탐색마다 울리지 않게 한다 |
| 끝 처리 | 탐색이 끝에 닿으면 Repeat·자동 초기화 가능 | 탐색 결과는 반복·자동 초기화 대상으로 표시하지 않는다. 끝나서 멈춘 인스턴스에서 끝 이후를 탐색하면 다시 실행하지 않는다 |
| 조작 기록 | 루트 모션으로 옮겨진 위치가 클릭만으로 기록될 수 있음 | 탐색 뒤 뷰포트 클라이언트의 기록 기준(SyncCommitBaseline)을 갱신한다 |
| 제거 | UAnimPreviewInstance 교체, 루트 모션 역산, 예약 재생, KataEditor의 AnimGraph 의존성, bIsAutonomousTickPose 처리 | 호출 경로가 없어져 삭제 |

## 엔진 제약

### 1. 한 엔진 프레임 안의 추가 World Tick은 액터를 진행하지 않는다

- FTickFunction::QueueTickFunction은 Tick 함수마다 방문한 GFrameCounter를 기록하고 같은 프레임에는 다시 큐에 넣지 않는다.
  같은 프레임에 World->Tick을 다시 불러도 액터·컴포넌트 Tick(CharacterMovement, 스켈레탈 메시 등)은 돌지 않는다.
- USkeletalMeshComponent::ShouldTickPose의 PoseTickedThisFrame, FTimerManager도 GFrameCounter로 프레임당 한 번만 진행한다.
- Kata 인스턴스의 프레임당 1회 제한에는 EditorPreview 월드 예외가 있고, StepWorld가 인스턴스를 직접 진행하는 보완 경로가 있다.
  그래서 Kata 시각만 앞서 나가고 캐릭터는 첫 단계만 반영되는 현상이 생겼다. 클릭 점프와 뒤로 드래그는 거의 움직이지 않았고,
  프레임당 1~2단계만 진행하는 앞으로 드래그만 정상이었다.
- 해결: 단계마다 GFrameCounter를 올린다. 엔진 자동화 헬퍼 AutomationCommon.cpp의 TickWorld도 월드를 동기로 여러 번 진행할 때
  World->Tick 뒤에 GFrameCounter를 올린다. 프레임 번호는 늘어나기만 하므로 엔진 루프의 증가와 충돌하지 않는다.
- 중간에 시도한 bIsAutonomousTickPose(한 프레임 여러 번 포즈 진행을 허용하는 예외)는 포즈 제한만 풀 뿐, Tick 관리자 제한을
  풀지 못해 효과가 없었다. CharacterMovement의 TickCharacterPose가 이 플래그를 끝에서 무조건 false로 되돌리는 점도 확인했다.

### 2. 월드 시간이 앱 경과 시간을 앞지르면 메시가 멈춘다

- 스켈레탈 메시는 bRecentlyRendered일 때만 본을 다시 계산한다(USkinnedMeshComponent::ShouldUpdateTransform).
  예외는 AlwaysTickPoseAndRefreshBones다. 캐릭터 설정에 따라 포즈 진행도 같은 판정에 묶인다.
- bRecentlyRendered는 LastRenderTime > World->TimeSeconds - 1.0으로 판정한다. 그런데 실시간 에디터 뷰포트는
  UseAppTime에 따라 렌더링 시각을 앱 경과 시간으로 기록한다(FEditorViewportClient::Draw).
- 동기 진행은 한 프레임에 월드 시간을 최대 액션 길이만큼 늘린다. 계속 드래그하면 프리뷰 월드 시간이 앱 경과 시간을 앞지르고,
  그때부터 메시가 멈춘다. ResetScene은 월드를 다시 만들지 않으므로 Reset을 눌러도 Idle이 재생되지 않았다.
- 해결: 프리뷰가 스폰한 Self·Target 스켈레탈 메시만 AlwaysTickPoseAndRefreshBones로 둔다. 캐릭터 에셋 설정은 바꾸지 않는다.
  월드 시간을 되돌리는 방법은 GE 지속 시간 등 월드 시간에 기대는 시스템을 깨뜨릴 수 있어 채택하지 않았다.

### 3. 마우스를 누르는 동안 Slate가 실시간 뷰포트 갱신을 멈춘다

- 마우스 버튼 누름을 처리한 위젯이 PreventThrottling을 반환하지 않으면 Slate가 반응성 모드에 들어간다(FSlateApplication).
  이 모드에서 UEditorEngine은 bNeedsRedraw가 켜진 뷰포트만 그린다.
- SEditorViewport::Invalidate는 위젯 쪽 함수라 bNeedsRedraw를 켜지 않는다. 클라이언트의 Invalidate(Viewport->InvalidateDisplay)가 켠다.
- 해결: 타임라인 탐색 캡처에 PreventThrottling을 붙이고, Seek와 탐색 처리에서 클라이언트 Invalidate를 호출한다.
  Current Time 스핀박스 드래그도 클라이언트 Invalidate로 다시 그린다.

## 주요 결정과 이유

- **탐색 결과를 실제 실행과 같게 한다.** 직접 포즈 평가는 즉시 반응하지만, 착지·걷기 중 바닥 추종·충돌·태스크 종료 뒤
  위치 유지를 계산으로 재현할 수 없다. "Montages Only" 모드의 몽타주 루트 모션은 블렌드 가중치 없이 적용되므로
  추출 값은 같았고, 차이는 적용 방식에서 생겼다.
- **드래그마다 시뮬레이션한다.** 드래그를 멈추거나 마우스를 놓을 때만 확정하는 안도 검토했다. 사용자는 드래그하는 동안에도
  정확한 결과를 보는 쪽을 택했다.
- **처리는 프레임당 한 번, 입력 콜백 밖에서 한다.** 마우스 이동은 한 프레임에 여러 번 올 수 있고, 입력 처리 중에 월드를 진행하지 않기 위해서다.
- **블렌드는 게임과 같다.** 태스크 시작 직후에는 Blend In 동안 이전 포즈와 섞여 보여 몽타주 에디터의 단독 포즈와 다르다.
  섹션 반복·건너뛰기도 실제 재생 순서를 따른다.

## 근거

- [SKataPreviewViewport.cpp](../../Plugins/Kata/Source/KataEditor/Private/SKataPreviewViewport.cpp): Seek, ApplyPendingSeek, SimulateTo, StepWorld, ResetScene.
- [SKataTimeline.cpp](../../Plugins/Kata/Source/KataEditor/Private/SKataTimeline.cpp): 탐색 캡처의 PreventThrottling.
- [KataActionEditor.cpp](../../Plugins/Kata/Source/KataEditor/Private/KataActionEditor.cpp): Seek 호출, Current Time 입력 처리.
- UE 5.8 엔진 소스: TickTaskManager.cpp(QueueTickFunction), SkeletalMeshComponent.cpp(ShouldTickPose),
  SkinnedMeshComponent.cpp(ShouldUpdateTransform, bRecentlyRendered), EditorViewportClient.cpp(Draw, UseAppTime),
  SlateApplication.cpp(반응성 모드), EditorEngine.cpp(뷰포트 그리기 조건), Tests/AutomationCommon.cpp(TickWorld).

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 엔진 제약 세 가지 | 에이전트의 UE 5.8 소스 읽기 | 위 절의 호출 경로와 판정식 확인 | 없음 |
| 최종 구현 | 사용자 빌드·에디터 확인 보고 (2026-09-24) | 사용자가 GFrameCounter 수정 뒤 동작이 훌륭하다고 보고 | 항목별 결과(클릭·양방향 드래그·Play 이어가기·Reset 뒤 Idle·소리)는 개별로 보고받지 않음 |
| 뒤로 탐색 비용 | 미측정 | 없음 | 긴 액션에서 드래그가 끊기는 정도 |

에이전트는 빌드·테스트를 실행하지 않았다.

## 남은 제한과 후속 작업

- 뒤로 탐색하는 비용은 목표 시각에 비례한다(1초당 60단계). 긴 액션을 뒤로 드래그하면 끊겨 보일 수 있다.
- GFrameCounter를 올리므로, 프레임 번호로 "이번 프레임에 이미 했는지"를 기억하는 다른 에디터 캐시가 탐색 프레임에 한 번 더 계산할 수 있다.
- 탐색이 진행한 만큼 프리뷰 월드 시간은 실제 시간보다 앞선다. 월드 시간을 앱 시간과 비교하는 다른 기능이 생기면 같은 문제를 확인해야 한다.
- 일반 재생에서 루트 모션으로 이동한 뒤 조작 대상이 선택된 상태로 뷰포트를 클릭하면, 옮겨진 위치가 Preview Transform에
  기록될 수 있다. 탐색 경로만 SyncCommitBaseline으로 막았다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [현재 구현 상태](Implementation-Status.md) | 탐색 방식, 엔진 제약 대응, 확인 결과 |
| [에디터 사용법](../manual/Editor-Usage.md) | 탐색 결과·비용·소리·Play 이어가기 |
| 몽타주 포즈 탐색 계획(완료 후 삭제) | "탐색 방식 변경" 결정과 동기 진행 항목 |
| [몽타주 포즈 탐색 구현 기록](2026-09-24-Montage-Scrub-Implementation.md) | 후속 변경 3과 두 후속 수정. 당시 기록을 보존하고 이 문서로 정리 |
| [문서 목록](../README.md) | 이 기록 등록 |
