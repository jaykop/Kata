# Kata 구현 상태

갱신: 2026-09-20

## 현재 기준

UE 5.8 / GAS 필수 / 싱글플레이. 모듈은 KataConditions, KataRuntime, KataEditor다.
새 콘텐츠는 UKataAction 전용 오브젝트 uasset이며, 런타임 실행 단위는 UKataActionInstance다.
KataAI, 네트워크 및 예측은 범위 밖이다.

KataAction 리네임과 레거시 제거 이후 사용자가 Editor 빌드와 Kata Action 에셋 생성을 확인했다.
에이전트는 빌드·UHT·테스트·UI 실행·별도 검사·리뷰를 수행하지 않았고 스트레스 테스트와 새 테스트 코드도 작성하지 않았다.
프리뷰 실행, 타임라인 조작, bSingleFrame 실행 동작의 회귀 여부는 아직 확인하지 않았다.

## 원본 에셋과 실행

- UKataAction은 액션 하나를 담는 UObject 에셋 타입이다. 클래스 상속 계층 없이 단일 클래스다.
- Content Browser에서 UKataAction 인스턴스를 저장한다. Blueprint/GeneratedClass/CDO를 만들지 않는다.
- ParentAction 객체 참조와 OverriddenSettings로 고유 설정을 상속한다. 구조체의 직접 필드는 개별 경로로 기록한다.
- TimelineTasks는 로컬 선언만 보관한다. TaskOverrides는 TaskId에 대한 프로퍼티 수정·비활성화·제거를 기록한다.
- 에셋 상속은 ParentAction 객체 체인이며 순환을 거절한다.
- UKataResolvedAction에 병합 결과와 SourceAction을 보관하고 설정·태스크·조건 사본을 만든다.
- UKataActionInstance와 UKataTaskInstance에만 실행 상태를 저장한다.
- UKataComponent::PlayKataAction, PlayKataActionOnSelf, CanPlayKataAction을 추가했다.
- UAbilityTask_PlayKataAction::PlayKataAction으로 GAS Ability에서 에셋을 실행한다.
- 에셋은 매 실행 시 해석한다. 해석 결과 캐시는 두지 않는다.
- UKataActionInstance::GetKataAction으로 실행 원본을 조회한다.
- 쿨다운 작성 필드는 Enabled, Duration, Start Time으로 단순화했다. 내부 Duration Gameplay Effect가 ASC에 시간을 저장한다.
- Shared Group Tags가 비어 있으면 Source Object로 원본 Kata를 식별하고, 설정하면 같은 태그의 Kata끼리 쿨다운을 공유한다.
- 외부 Cooldown Gameplay Effect, Effect Level, Kata/CallingAbility Owner 선택은 제거했다.

Blueprint/CDO 기반 UKataDefinition 경로는 제거했다. 병합은 ParentAction 체인 한 갈래만 사용한다.
FKataTimelineEntry·FKataTaskOverride의 DeclaringClass, UKataResolvedAction의 SourceClass,
UKataComponent·UAbilityTask_PlayKataAction의 클래스 오버로드, 에디터의 Import Legacy가 함께 사라졌다.
클래스 경로 전용이던 진단 코드 EditedInheritedEntry·UnstampedTimelineEntry도 더는 발생하지 않는다.
이전 계획의 “클래스 상속이 핵심”이라는 결정은 이번 에셋 모델로 대체한다.

## 전용 에디터

- UFactory와 UAssetDefinition을 통해 Kata 생성 메뉴와 더블클릭 동작을 연결했다.
- 전용 FAssetEditorToolkit에 Preview / Timeline / Kata Details / Task Details 탭을 배치했다.
  Preview는 자체 탭 영역에 두고 Timeline은 아래쪽 전체 폭을 사용한다.
- 프리뷰 배치·조명 설정은 Preview Details 전용 탭에서 편집한다. Preview 월드 탭과 Kata Details에서는 숨긴다.
- 탭 배치는 레이아웃 이름 KataAssetEditor_v3으로 EditorLayout에 저장한다. 이 이름은 고정하며 탭을 추가해도 바꾸지 않는다.
  이름을 바꾸면 사용자가 저장한 배치가 사라진다. 모든 탭은 Window 메뉴에 등록해 닫아도 다시 열 수 있다.
- 타임라인 눈금·스냅 간격을 Interval (s)로 조절하고 Snap 체크로 켜고 끈다.
  스냅은 눈금과 다른 태스크의 시작·끝 중 가까운 값을 사용한다.
- 툴바에 Select Target과 Resize를 추가했다. Resize는 가장 늦게 끝나는 태스크에 View (s)를 맞춘다.
  View (s)는 프로젝트별 에디터 사용자 설정에 즉시 저장하며 다음 에디터 세션에서 복원한다.
- Kata Details, Task Details, Preview Details는 타입을 처음 표시할 때 모든 필드를 펼친다.
  이후에는 서로 다른 영속 식별자로 사용자가 접거나 펼친 상태를 다음 에디터 세션에서 복원한다.
- 태스크 타입 선택, 복수 행 선택, 드래그 이동, 양쪽 끝 길이 변경, 삭제, Details 편집을 구현했다.
  왼쪽 끝은 끝 시각을, 오른쪽 끝은 시작 시각을 고정하며 위치에 따라 커서 모양을 바꾼다.
- Ctrl 또는 Shift 클릭으로 태스크를 복수 선택한다. 선택한 클립에는 주황색 외곽선을 표시한다.
  같은 클래스의 태스크를 함께 선택하면 Task Details가 공통 값을 한 번에 편집한다. 서로 다른 클래스 선택은 개수만 표시한다.
- 태스크 추가·삭제·복사·붙여넣기와 Undo/Redo를 타임라인 우클릭 팝업 메뉴로 제공한다.
  Add Task와 메뉴의 Paste는 우클릭한 시각을 시작 시각으로 사용한다.
- 타임라인에 키보드 포커스가 있을 때 Delete, Ctrl+C, Ctrl+V를 처리하고 Ctrl+Z·Ctrl+Y는 툴킷 전체에 연결했다.
  단축키 붙여넣기는 재생 헤드 시각을 사용한다.
- 복사본은 열려 있는 Kata 에디터가 공유하는 단일 태스크 슬롯이며 붙여넣을 때 새 Task Id를 발급한다.
- 미완성·비활성 태스크도 편집 목록에 포함한다. 실행용 해석은 기존 오류 판정을 유지한다.
- 태스크 설정 오류는 저장 검사에서 경고로만 보고하고 에셋을 Invalid로 만들지 않는다.
  해석 단계에서는 그대로 Error로 남겨 해당 태스크를 실행에서 제외한다. 진단에 bIncompleteAuthoring을 기록해 두 경로를 구분한다.
- 진단 메시지는 TaskId GUID 대신 태스크 표시 이름을 쓰고, 오류 코드에 대응하는 설명과 시작 시각을 함께 적는다.
  UKataTask::DescribeConfigurationError를 파생 태스크가 재정의해 설명을 제공한다.
- 편집용 사본에 부모와 자식의 최종 값을 표시하며, 실제 편집한 프로퍼티만 원본에 기록한다.
- 부모 에셋 변경 알림, Save, Undo/Redo, Create Child, 오버라이드 복원 UI를 연결했다.
- Import Legacy (Replace)는 제거했다. 변환할 레거시 Blueprint 콘텐츠가 없다.

## 프리뷰

- 별도 FPreviewScene의 EditorPreview 월드를 사용한다. GamePreview 월드는 RequiresHitProxies가 꺼져
  메시 히트 프록시가 생성되지 않아 트랜스폼 기즈모를 집을 수 없다. 비게임 월드용 MovementComponent 갱신은 함께 켠다.
- 프리뷰 주체·대상 클래스와 Transform을 에셋의 editor-only 데이터에 저장한다.
- ASC와 KataComponent를 연결하고 게임의 에셋 실행 경로를 사용한다.
- Play / Pause / Stop·Reset을 Timeline 탭 상단의 아이콘 버튼으로 제공한다.
- Preview 탭 상단에서 Perspective, Top, Right, Back 카메라를 전환한다. 활성 뷰는 파란 Toggle Button으로 표시한다.
  Top과 Right는 직교 투영이며 Self와 Target을 모두 담는 영역에 대해 엔진의 FocusViewportOnBox로 중심과 확대 배율을 맞춘다.
  Back View도 직교 투영이며 Self Actor의 Forward를 수평면에서 가장 가까운 월드 축으로 스냅해 그 시선 방향의 직교 뷰를 고른다.
  즉 Self Actor의 등 뒤에서 바라보고, 중심과 확대 배율은 Top·Right와 같은 포커스 계산을 사용한다.
  구도별 카메라 위치·회전·확대 배율을 각각 기억해 전환할 때 복원하며, 처음 여는 구도만 기본 위치로 맞춘다.
  활성 버튼을 다시 누르면 그 구도를 기본 위치로 되돌린다. Back View는 Self Actor의 방향이 바뀌어
  다른 축을 쓰게 되면 기록을 버리고 다시 맞춘다.
- 기본 Self Transform은 바닥 위 Z 100cm이며, Target은 Top View 화면에서 Self보다 위쪽인 -X 200cm, Z 100cm에 놓는다.
  Self의 기본 Yaw는 180도, Target의 기본 Yaw는 0도로 서로 마주 본다.
- Directional Light의 Rotation, Brightness, Color를 에셋의 editor-only 값으로 저장하고 프리뷰 장면에 적용한다.
  기본 Rotation은 Pitch -40, Yaw 157.5, Roll 0이다.
- Select Target을 켜면 Target Actor를 프리뷰 전용 선택 집합에 등록하고 Unreal 네이티브 트랜스폼 위젯을 붙인다.
  커스텀 프리뷰에서 생성되지 않는 ITF 자동 기즈모 대신 엔진의 레거시 FWidget 렌더링·히트 프록시 경로를 사용한다.
  ModeTools에는 PreviewScene만 연결하고 FWidget에는 연결하지 않는다. FWidget::Render는 ModeTools가 연결되면 활성 레거시
  에디터 모드를 요구해 프리뷰에서 기즈모가 그려지지 않는다. 위젯 모드·위치·좌표계는 뷰포트 클라이언트의 재정의만 사용한다.
  Q/W/E로 선택·이동·회전 모드를 바꾸며, 위젯 축 드래그와 방향키·PageUp/PageDown 조작을 지원한다.
  크기 조절은 제공하지 않는다.
  위젯 델타는 프리뷰 Target Actor에 직접 적용해 기본 에디터 처리기가 입력만 소비하는 경우를 피한다.
  이동 델타가 에디터 그리드 단위로 양자화되지 않도록 위젯 스냅을 끄고, 프리뷰 액터의 루트는 Movable로 맞춘다.
  Target 보조 외곽선은 그리지 않는다. 측정 도형은 히트 프록시 패스에서 제외해 이동 축 판정을 덮지 않게 하고,
  드래그 중에는 히트 프록시를 갱신하지 않으며 Target 변경과 드래그 종료 시 갱신한다.
  조작을 끝낼 때 Preview Target Transform에 트랜잭션으로 기록한다. 축은 월드 고정이다.
- 노출은 FPreviewScene의 기본 자동 노출 경로를 사용한다. 에셋의 Light 설정과 별개인 고정 EV100 보정은 적용하지 않는다.
- 배경색과 Environment Size X/Y/Z를 Preview Details에서 조절한다. X/Y는 바닥 크기와 벽 폭,
  Z는 벽 높이에 함께 적용하며 앞쪽·왼쪽 벽이 코너를 이룬다.
- 바닥과 벽에는 M_ProcGrid의 Object Aligned 설정을 가진 `MI_ProcGrid`를 사용해 세로 벽에서도 격자 비율을 유지한다.
- Show Debug Shape로 측정 표시를 켜고, Debug Shape enum으로 Grid 또는 Sphere를 선택한다.
  Grid Cell Size, Debug Color, Debug Thickness를 조절할 수 있으며 최대 범위는 Environment Size를 따른다.
  Sphere의 마지막 구는 설정 간격으로 나누어떨어지지 않아도 환경 최대 범위에 정확히 맞춘다.
  Show Debug Shape의 기본값은 false이고 Debug Thickness의 기본값은 2다.
- 눈금 탐색은 액터 초기화 후 고정 시간 간격 재생으로 구현했다. 임의 역재생이나 결정적 스냅샷 복원은 아니다.
- 프리뷰 종료·편집·Undo 시 실행을 정리한다.
- 프리뷰 클래스가 없으면 위치 표시용 구체를 사용하며 충돌 바닥을 생성한다.

## 유지한 런타임과 조건

- Tag: Any/All, Exact Match, Self/Target.
- Attribute: 절대값 또는 Current/Max Ratio, 비교 연산, 같음 허용 오차.
- Distance: 양쪽 Actor/Socket, ComponentTag, 2D/3D, 비교 연산자 + 기준 거리.
- Angle: 2D/3D, HalfAngle, YawOffset.
- 공통 Context·Pass/Fail/Invalid·Invert 및 C++/Blueprint 확장.
- Kata 태그, Activation/Block, StartCondition, GAS 쿨다운, 루프 정책.
- 인스턴스 소유 스케줄러, Phase·OrderHint, AfterStart/AfterCompletion 의존성.
- UKataTask::bSingleFrame. 켜면 Duration과 무관하게 시작한 프레임에서 Tick을 한 번만 받고 끝난다.
  Duration 0인 순간 태스크는 Tick을 한 번도 받지 않으므로 서로 다른 경로다.
  스케줄러가 종료 경계를 만들지 않고 TickActiveTasks가 Tick 직후 완료 처리한다.
  타임라인에서는 최소 폭 표식으로 그리고 길이 조절 손잡이를 감춘다. Details에서는 Duration을 숨긴다.
- 실행 종료 시 태스크 정리 및 GAS 활성 태그·Ability 차단 회수.
- 기본 Play Montage 태스크.
- 조건 테스트(KataConditions/Private/Tests)는 수정·확장하지 않았다.

## 프로젝트 테스트 하네스

Source/ProjectKata/Testing은 프로젝트 전용이며 플러그인에 포함하지 않는다.

- KataTestActions가 코드로 UKataAction 트리를 만든다. EKataTestAction은 Basic, Override, Dependency, Loop, Invalid다.
- 클래스 상속 대신 ParentAction 객체 체인으로 부모·자식을 구성하며 Task Id는 고정 GUID를 유지한다.
- 자식의 고유 설정은 OverriddenSettings에 등록해야 병합 결과에 남는다. Loop 액션이 LoopPolicy를 등록한다.
- AKataTestActor는 에셋용 ActionToPlay와 코드 하네스용 BuiltInAction을 함께 가진다.
  BuiltInAction이 None이 아니면 매 실행마다 액션 트리를 새로 만들어 우선 사용한다.
- 콘솔은 Kata.Resolve <이름|에셋경로>, Kata.Play [이름|에셋경로], Kata.List, Kata.Stop이다.
  Kata.List는 내장 액션 목록과 로드된 UKataAction 에셋을 나눠 출력한다.

## 제한과 다음 범위

- 이전 CooldownPolicy의 Owner, CooldownEffect, CooldownTags, EffectLevel 값은 새 필드로 자동 변환하지 않는다. 기존 에셋은 Duration과 필요한 Shared Group Tags를 다시 지정해야 한다.
- 프리뷰는 PIE/게임 세션 초기화를 대체하지 않는다. GameInstance·Controller·PlayerState 의존 처리는 자동 제공하지 않는다.
- 프리뷰에 호출 Gameplay Ability는 없다. 프로젝트 확장 태스크는 이 환경을 고려해야 한다.
- 조건 객체·배열·태그 컨테이너는 해당 프로퍼티 전체를 오버라이드한다.
- Map/Set 및 구조체 전체 내부의 복잡한 Instanced 객체 복제는 미지원이다.
- AfterMeshPose는 실제 엔진 갱신 시점 연결 전까지 오류로 처리한다.
- 타임라인 트랙 그룹·의존성 시각 편집은 미구현이다. 복사·붙여넣기는 한 번에 한 태스크만 지원한다.
- 태스크 클립보드는 에디터 세션 동안만 유지하며 OS 클립보드나 다른 프로세스와 공유하지 않는다.
- 콤보 그래프(KataGraph)·입력 버퍼·다중 액션 채널·전역 실행 Subsystem은 미구현이다. GenericGraph 의존성도 아직 없다.
- bSingleFrame 태스크가 타임라인 끝이나 루프 경계에서 시작하면 Tick을 받기 전에 Kata가 끝나 Interrupted로 종료될 수 있다.

사용법은 [Editor-Usage.md](../manual/Editor-Usage.md), [Runtime-Usage.md](../manual/Runtime-Usage.md)를 따른다.
후속 범위는 [Next-Work-Plan.md](../plan/Next-Work-Plan.md)에 정리했다.

## 사용자 빌드 오류 대응

- KataAssetEditor.cpp의 존재하지 않는 CoreUObjectDelegates.h include를 UE 5.8에서 FCoreUObjectDelegates를 선언하는 UObjectGlobals.h로 수정했다. 수정 후 빌드·검사는 실행하지 않았다.
- SKataPreviewViewport의 `TUniquePtr<FKataPreviewScene>`이 전방 선언 상태에서 삭제 코드를 인스턴스화하던 C4150 오류를 수정했다.
  소유 포인터는 완전한 기반 타입 `FPreviewScene`을 사용하고 실제 객체만 배경색 지원 파생 타입으로 생성한다. 수정 후 빌드·검사는 실행하지 않았다.
