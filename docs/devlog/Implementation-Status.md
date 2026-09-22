# Kata 구현 상태

갱신: 2026-09-23

## 현재 기준

UE 5.8 / GAS 필수 / 싱글플레이. 플러그인 모듈은 KataConditions, KataRuntime, KataGraph, KataEditor,
KataGraphEditor다. 프로젝트 전용 검증 코드는 ProjectKataTesting DeveloperTool 모듈이 맡는다.
새 콘텐츠는 UKataAction 전용 오브젝트 uasset이며, 런타임 실행 단위는 UKataActionInstance다.
KataAI, 네트워크 및 예측은 범위 밖이다.

KataAction 리네임·레거시 제거와 GenericGraph 흡수 이후 사용자가 Editor 빌드를 확인했다.
Kata Action 에셋과 Kata Graph 에셋 생성도 확인했다.
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
- UKataExecutionWorldSubsystem이 월드의 활성 인스턴스를 Execution Priority와 실행 시작 순서로 정렬해
  Actor·Component Tick 전에 진행한다. 인스턴스 내부 순서는 기존 FKataTaskScheduler가 담당한다.
  Subsystem이 생성되지 않는 특수 월드에서는 UKataComponent Tick이 대체 경로로 진행한다.
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
- 전용 FAssetEditorToolkit에 Preview / Timeline / Kata Action Details / Timeline Details 탭을 배치했다.
  Preview는 자체 탭 영역에 두고 Timeline은 아래쪽 전체 폭을 사용한다.
- 프리뷰 배치·조명 설정은 Preview Details 전용 탭에서 편집한다. Preview 월드 탭과 Kata Action Details에서는 숨긴다.
- 탭 배치는 레이아웃 이름 KataAssetEditor_v3으로 EditorLayout에 저장한다. 이 이름은 고정하며 탭을 추가해도 바꾸지 않는다.
  이름을 바꾸면 사용자가 저장한 배치가 사라진다. 모든 탭은 Window 메뉴에 등록해 닫아도 다시 열 수 있다.
- 타임라인 눈금·스냅 간격을 Interval (s)로 조절하고 Snap 체크로 켜고 끈다.
  스냅은 눈금과 다른 태스크의 시작·끝 중 가까운 값을 사용한다.
- 타임라인은 행이 적어도 Timeline 탭 높이를 채운다. 스크롤 영역의 현재 높이를 타임라인의 최소 높이로
  요구하며, 행이 늘어 희망 높이가 그 값을 넘으면 평소대로 스크롤한다.
- 시간 눈금 영역과 태스크 배치 영역 사이에 가로 경계선을 그린다.
- 툴바에 Select Target과 Resize를 추가했다. Resize는 가장 늦게 끝나는 태스크에 Length를 맞춘다.
  Length는 프로젝트별 에디터 사용자 설정에 즉시 저장하며 다음 에디터 세션에서 복원한다.
- Timeline 상단에 Current Time을 표시하고 값을 직접 입력해 재생 헤드를 이동할 수 있다.
  시간 눈금과 태스크 클립이 없는 빈 시간 영역은 클릭·드래그 탐색을 지원한다. 이 영역은 십자 커서를,
  태스크 양쪽 끝은 좌우 크기 조절 커서를 사용한다.
  탐색을 시작하면 재생 헤드는 프리뷰 시작 성공 여부와 무관하게 목표 시각으로 즉시 이동하고, 실행 가능한
  프리뷰 시뮬레이션만 내부에서 해당 시각까지 따라간다.
  일반 재생 중에는 실제 액션 인스턴스 시각을 표시하며, EditorPreview 월드가 전역 실행 콜백을 제공하지 않을 때만
  프리뷰가 인스턴스를 직접 한 번 진행시키는 보완 경로를 사용한다.
- Kata Action Details, Timeline Details, Preview Details는 타입을 처음 표시할 때 모든 필드를 펼친다.
  이후에는 서로 다른 영속 식별자로 사용자가 접거나 펼친 상태를 다음 에디터 세션에서 복원한다.
- 태스크 타입 선택, 복수 행 선택, 드래그 이동, 양쪽 끝 길이 변경, 삭제, Details 편집을 구현했다.
  왼쪽 끝은 끝 시각을, 오른쪽 끝은 시작 시각을 고정하며 위치에 따라 커서 모양을 바꾼다.
- Ctrl 또는 Shift 클릭으로 태스크를 복수 선택한다. 선택한 클립에는 주황색 외곽선을 표시한다.
  같은 클래스의 태스크를 함께 선택하면 Timeline Details가 공통 값을 한 번에 편집한다. 서로 다른 클래스 선택은 개수만 표시한다.
- 태스크 추가·삭제·복사·붙여넣기와 Undo/Redo를 타임라인 우클릭 팝업 메뉴로 제공한다.
  Add Task와 메뉴의 Paste는 우클릭한 시각을 시작 시각으로 사용한다.
- 타임라인에 키보드 포커스가 있을 때 Delete, Ctrl+C, Ctrl+V를 처리하고 Ctrl+Z·Ctrl+Y는 툴킷 전체에 연결했다.
  단축키 붙여넣기는 재생 헤드 시각을 사용한다.
- 복사본은 열려 있는 Kata 에디터가 공유하는 단일 태스크 슬롯이며 붙여넣을 때 새 Task Id를 발급한다.
- UKataTask의 editor-only 자동 색상, TimelineDisplayColor와 EditorComment를 타임라인 색·호버 툴팁·선택적
  클립 텍스트에 사용한다. 자동 색상은 Task Id 기반 색조라 기존·신규 태스크에 안정적으로 다른 색을 부여한다.
- UKataAction의 editor-only TimelineGroups로 태스크를 묶고 제목·접기/펼치기·그룹 해제를 제공한다.
  그룹 헤더 선택 시 Timeline Details에서 제목·색상·주석·태스크 수를 표시한다. 그룹 우클릭 메뉴에서 새 태스크 생성과
  현재 선택 태스크 이동을 지원하며, 태스크 우클릭 메뉴에서도 기존 그룹을 골라 이동할 수 있다.
  그룹 태스크 행에는 그룹 색상 레일·가지선·배경 틴트와 들여쓰기를 표시한다.
  그룹은 TaskId만 참조하며 실행에는 영향을 주지 않고
  부모 액션에서 상속하지 않는다. 접힘 상태는 사용자 설정이다.
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
- Play/Pause, Stop·Reset, Repeat를 Timeline 탭 상단의 아이콘 버튼으로 제공한다.
  재생과 일시 정지는 한 버튼이 맡으며 재생 중에는 일시 정지 아이콘으로 바뀐다.
  Repeat는 프리뷰가 타임라인 끝까지 진행해 끝났을 때만 처음부터 다시 실행하는 편집기 설정이다.
  에셋의 FKataLoopPolicy와 무관하며 프로젝트별 에디터 사용자 설정에 저장한다.
  실행 시각이 진행되지 않고 끝난 인스턴스는 재시작 대상이 아니므로 길이가 0인 액션이 매 프레임 되살아나지 않는다.
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
- 눈금 탐색은 고정 시간 간격 재생으로 구현했다. 앞으로 이동하면 현재 프리뷰 상태를 이어 쓰고,
  뒤로 이동하면 액터를 초기화한 뒤 처음부터 다시 실행한다. 임의 역재생이나 결정적 스냅샷 복원은 아니다.
  탐색은 화면 한 프레임에서 최대 8개의 시뮬레이션 프레임을 진행한다. Task의 Tick 제한은 각 시뮬레이션 프레임에 적용한다.
  월드 갱신 뒤 직접 갱신이 필요한지는 UKataActionInstance의 TickSerial로 확인한다. Loop로 액션 시각이 되돌아가도
  같은 시뮬레이션 프레임에서 중복 진행하지 않는다.
- 프리뷰 종료·편집·Undo 시 실행을 정리한다.
- 프리뷰 클래스가 없으면 위치 표시용 구체를 사용하며 충돌 바닥을 생성한다.
- KataRuntime이 `AKataCharacter`를 제공한다. ACharacter에 ASC와 KataComponent를 붙이고
  PostInitializeComponents에서 ASC의 Actor Info를 초기화하는 런타임 기반 클래스이며,
  Preview Actor Class에 지정할 기본 캐릭터로 쓴다. 스켈레탈 메시와 Anim Instance는
  파생 블루프린트에서 지정한다. 싱글플레이 전용이라 복제를 설정하지 않는다.

## 유지한 런타임과 조건

- Tag: Any/All, Exact Match, Self/Target.
- Attribute: 절대값 또는 Current/Max Ratio, 비교 연산, 같음 허용 오차.
- Distance: 양쪽 Actor/Socket, ComponentTag, 2D/3D, 비교 연산자 + 기준 거리.
- Angle: 2D/3D, HalfAngle, YawOffset.
- `UKataFL_Condition` Blueprint Function Library에 `CheckAngle`, `CheckDistance`, `CheckTag`, `CompareValue`를
  부작용 없는 공용 판정 함수로 제공한다. 공개 함수는 Kata 전용 커스텀 구조체를 매개변수로 받지 않는다.
- Angle·Distance·Tag·Attribute 조건 UObject는 설정과 Context 변환, Invalid 진단, Invert 책임을 유지하고
  실제 반복 판정은 `UKataFL_Condition`에 위임한다. 수치 비교 설정 검사도 Distance와 Attribute가 공유한다.
- Group: 인라인 자식 조건 배열을 All/Any로 평가한다. 단축 평가하며 자식은 Evaluate 진입점으로 호출해
  자식의 Invert와 설정 검사를 반영한다. 자식 Invalid는 그대로 전파한다.
  빈 배열·null 항목·자기 참조는 설정 오류이며 IsDataValid가 자식 오류까지 함께 보고한다.
- 공통 Context·Pass/Fail/Invalid·Invert 및 C++/Blueprint 확장.
- Kata 태그, Activation/Block, StartCondition, GAS 쿨다운, 루프 정책.
- 인스턴스 소유 스케줄러, Phase·OrderHint, AfterStart/AfterCompletion 의존성.
- UKataTask::bSingleFrame. 켜면 Duration과 무관하게 시작한 프레임에서 Tick을 한 번만 받고 끝난다.
  Duration 0인 순간 태스크는 Tick을 한 번도 받지 않으므로 서로 다른 경로다.
  시작 즉시 Tick 한 번과 완료를 처리해 타임라인 끝이나 루프 경계에서도 실행을 보장한다.
  타임라인에서는 최소 폭 표식으로 그리고 길이 조절 손잡이를 감춘다. Details에서는 Duration을 숨긴다.
- 태스크 Tick은 게임 프레임당 최대 한 번이다. 시작·종료 경계를 순서대로 처리하되, 종료하는 태스크는 End 직전에,
  계속 실행되는 태스크는 이번 프레임의 진행 끝에서 한 번 Tick한다. 다른 태스크의 경계마다 다시 Tick하지 않는다.
  DeltaTime에는 마지막 Tick 또는 실제 시작 이후의 실행 구간을 전달한다. 한 프레임 안에 시작과 종료를 모두 지난
  지속 태스크는 Start → Tick → End로 처리하고, 프레임 끝에 시작하면 첫 Tick의 DeltaTime은 0이다.
- 재생 요청은 GAS 활성 상태를 적용하고 시각 0의 태스크를 즉시 실행한다. 일반 지속 태스크는 Start → Tick(0),
  Single Frame 태스크는 Start → Tick(0) → End, 순간 태스크는 Start → End로 처리한다. 완료 의존성을 기다리는
  태스크나 시작 콜백에서 스스로 끝난 태스크에는 실행을 강제하지 않는다.
  최초 시작을 해당 게임 프레임의 갱신으로 기록해 같은 프레임의 TickInstance는 다시 진행하지 않는다.
  시간은 다음 프레임부터 진행한다. 길이 0 액션의 완료 알림은 재생 요청 함수 안에서 발생할 수 있다.
  첫 TickInstance까지 Start를 미루던 변경을 되돌려, 월드 실행 콜백 이후의 재생 요청도 즉시 반응하도록 했다.
- 완료 의존성은 타임라인 처리 중 충족되면 같은 갱신에서 이어서 실행한다. 몽타주 종료 등 갱신 밖의 콜백으로
  완료된 경우에는 후속 태스크의 Start와 Tick을 다음 실행 갱신에서 함께 처리한다.
- Loop는 액션 실행기가 타임라인 전체를 반복하는 기능이다. MaxLoopCount는 최초 실행을 포함한 총 실행 횟수이며
  0은 무한 반복이다. 회차 끝에서 태스크를 정리하고 다음 프레임에 재시작한다. 끝을 넘긴 DeltaTime은 넘기지 않으므로
  프레임 지연에 따라 실제 반복 완료 시간이 늘어날 수 있다. 시작 조건과 GAS 활성화·쿨다운을 회차마다 다시 처리하지 않는다.
  Task는 Loop 여부를 판단하지 않는다. 실행기가 ResetForExecution으로 재시작을 준비한다. Restart on Loop 설정은 추가하지 않았다.
- MaxIterationsPerTick과 반복 횟수 초과에 따른 ContractError 종료를 제거했다. 기존 에셋의 해당 값은 사용하지 않으며,
  PostLoad에서 LoopPolicy.MaxIterationsPerTick 오버라이드 경로만 제거한다. 기존 Loop 횟수와 나머지 오버라이드는 유지한다.
  길이 0인 Loop 타임라인은 기존처럼 해석 오류로 처리한다.
- TickInstance는 게임의 같은 프레임 내 중복 호출과 콜백 재진입을 무시한다. EditorPreview의 명시적 시뮬레이션 진행은
  별도 프레임으로 취급한다. Loop·Tick 변경과 최초 실행의 즉시 반응 복원 후 빌드·UHT·테스트·UI 실행·별도 검사는 수행하지 않았다.
  기존 테스트 하네스에서는 삭제된 MaxIterationsPerTick 대입만 제거했으며 테스트를 추가하거나 확장하지 않았다.
- 실행 종료 시 태스크 정리 및 GAS 활성 태그·Ability 차단 회수.
- 기본 Play Montage 태스크.
- 기본 Send Gameplay Event 태스크. 대상 ASC로 이벤트를 한 번 보내고 곧바로 완료한다.
  지속 시간을 줘도 발송 시점은 시작 시각 한 번이다. 페이로드는 Event Tag, Instigator, Target,
  Event Magnitude, Optional Object를 채운다. 대상이 IAbilitySystemInterface를 구현하지 않아도
  동작하도록 UAbilitySystemBlueprintLibrary 대신 ASC의 HandleGameplayEvent로 직접 보낸다.
- 기본 Apply Gameplay Effect 태스크. 적용 주체는 항상 Kata 실행자이고 대상만 설정으로 고른다.
  Remove Policy로 GE 자체 지속 시간을 따를지 태스크 구간 종료에 제거할지 정한다.
  제거는 자신이 적용한 핸들에 대해서만, 핸들이 살아 있는 대상 ASC에서 수행한다.
  Instant GE는 남는 활성 효과가 없어 RemoveOnTaskEnd를 골라도 제거할 대상이 없으며 시작 시 경고를 남긴다.
- 기본 Apply Loose Tag 태스크. 구간 시작에 Loose Gameplay Tag를 붙이고 종료에 회수한다.
  Loose Tag는 참조 카운트로 관리되므로 실제로 붙인 태그와 붙인 ASC를 인스턴스가 기억했다가
  같은 대상에서만 되돌린다. 태그 하나를 위해 GE 에셋을 만들지 않도록 GE 적용과 분리했다.
  구간이 없으면 관측되지 않으므로 bSingleFrame과 Duration 0은 설정 오류로 본다. 기본 Duration은 0.2초다.
- 태스크 공용 대상 지정으로 EKataTaskTargetSource(Avatar/Owner/ContextTarget)와
  FKataContext::ResolveActor·ResolveAbilitySystemFor를 두었다. 선행 태스크의 출력까지 받는
  공용 FKataTargetSpec은 아직 없다. Avatar와 Owner는 현재 같은 ASC를 돌려준다.
- 조건 테스트(KataConditions/Private/Tests)는 수정·확장하지 않았다.
- 조건 Function Library 변경 후 사용자가 빌드 성공을 확인했다. 에이전트는 빌드·UHT·자동화 테스트를 실행하지 않았고,
  Blueprint 노드 노출은 아직 확인하지 않았다.
- Send Gameplay Event·Apply Gameplay Effect·Apply Loose Tag 태스크는 아직 빌드·실행으로 확인하지 않았다.
  프리뷰 월드는 ASC를 준비하지만 AttributeSet과 이벤트를 받을 Ability는 없으므로,
  두 태스크의 프리뷰 동작은 별도로 확인해야 한다. 태스크별 프리뷰 정책 선언은 아직 없다.

## 그래프 계층

GenericGraph(MIT)를 Kata 플러그인 안으로 흡수했다. 출처와 변경 내역은
`Plugins/Kata/Source/KataGraph/UPSTREAM.md`에 있다.

- KataGraph 모듈에 UKataGraphBase·UKataGraphNodeBase·UKataGraphEdgeBase를 둔다. 런타임 3개 클래스는 직접 다시 작성했다.
- UKataGraphNodeBase::Edges는 TMap<Node*, FKataGraphEdgeList>다. UHT가 TMultiMap과 중첩 컨테이너를
  리플렉션 대상으로 지원하지 않아 구조체로 감쌌다. 같은 두 노드 사이에 엣지를 여러 개 둘 수 있다.
- 단계 순회(GetLevelNum, GetNodesByLevel, Print)는 방문 기록을 사용한다. 원본은 순환 그래프에서 끝나지 않았다.
- 노드는 UKataNode(추상, EntryCondition) 아래 UKataActionNode(액션 실행)와 UKataEntryNode(진입점)를 둔다.
  그래프 스키마가 NodeType의 하위 클래스만 우클릭 메뉴에 올리므로 공통 추상 부모가 필요하다.
- UKataEdge는 TriggerTag(계층 매칭), RequiredWindowTag, Condition, Timing, Priority를 가진다.
  TriggerTag가 비면 조건만 보는 자동 전이다. 연결선에 트리거 이름을 표시하며 Trigger Event Tag와
  Required Action Window Tag의 Details 툴팁은 역할과 빈 값의 의미를 한국어로 설명한다.
- 수용 구간은 액션 타임라인의 UKataTask_TransitionWindow가 연다. 액션은 그래프 위상을 모르고
  그래프는 구간의 시각을 모른다. 구간은 UKataTask의 Start Time과 Duration을 그대로 쓴다.
- UKataActionInstance가 열린 창을 태그별로 추적한다. 창이 열린 시각은 월드 시각으로 기록해
  컴포넌트가 들고 있는 트리거 도착 시각과 같은 시계를 쓴다. PreAcceptSeconds가 선행 입력 폭이다.
- UKataGraphInstance가 현재 노드·액션 인스턴스·OnActionEnd 예약을 소유한다. UKataGraphComponent가 같은 액터의
  UKataComponent를 찾아 그래프 시작과 SendTrigger API를 제공한다. Immediate는 현재 액션을 중단하고,
  OnActionEnd는 정상 완료 뒤 전이한다. 자동 전이는 진입 또는 정상 완료 시 평가한다.
- 전이 후보는 Priority 내림차순, 저장된 ChildrenNodes와 자식별 엣지 배열 순서로 결정한다. TMap 순회 순서에는
  의존하지 않는다. 엣지 조건 뒤 대상 노드 EntryCondition을 평가한다. 입력 버퍼는 아직 없다.
- KataGraphEditor 모듈이 그래프 에디터를 제공한다. 에셋 등록은 UAssetDefinition 경로를 쓴다.
  액션 노드 제목은 할당한 에셋 이름을 따르고, Kata Graph Details와 Selection Details를 분리했다.
  Comment 명령과 동일 노드 쌍의 병렬 엣지를 지원한다. 병렬 엣지는 같은 위치에 겹치지 않도록 저장 순서별로
  제목·아이콘을 분리 배치한다. 연결 수 제한 항목은 고급 설정으로 분류했다.
- Slate double→float 전환 관련 C4996 폐기 경고 6건이 남아 있다. 다음 엔진 릴리스에서는 오류가 된다.

## 프로젝트 테스트 하네스

Source/ProjectKataTesting은 프로젝트 전용 DeveloperTool 모듈이며 플러그인과 Shipping Runtime에 포함하지 않는다.

- KataTestActions가 코드로 UKataAction 트리를 만든다. EKataTestAction은 Basic, Override, Dependency, Loop, Invalid다.
- 클래스 상속 대신 ParentAction 객체 체인으로 부모·자식을 구성하며 Task Id는 고정 GUID를 유지한다.
- 자식의 고유 설정은 OverriddenSettings에 등록해야 병합 결과에 남는다. Loop 액션이 LoopPolicy를 등록한다.
- AKataTestActor는 에셋용 ActionToPlay와 코드 하네스용 BuiltInAction을 함께 가진다.
  BuiltInAction이 None이 아니면 매 실행마다 액션 트리를 새로 만들어 우선 사용한다.
- 콘솔은 Kata.Resolve <이름|에셋경로>, Kata.Play [이름|에셋경로], Kata.List, Kata.Stop이다.
  Kata.List는 내장 액션 목록과 로드된 UKataAction 에셋을 나눠 출력한다.
- 테스트 하네스는 `bBuildDeveloperTools`가 켜진 Target에서만 빌드·로드한다. ProjectKata Runtime 모듈은
  KataConditions·KataRuntime·GameplayTags·GameplayAbilities에 대한 테스트 전용 의존성을 갖지 않는다.
- 모듈 이동 전 `/Script/ProjectKata`에 있던 테스트 클래스와 enum에는 `/Script/ProjectKataTesting`으로
  이어지는 Core Redirect를 추가했다.
- `Content/KataTest`는 `DirectoriesToNeverCook`에 등록해 cooked 빌드에서 제외한다.
- 모듈 분리 후 사용자가 정상 빌드를 확인했다. 에디터에서 기존 테스트 에셋을 다시 열어 Redirect와
  테스트 하네스 동작을 확인하는 작업은 아직 수행하지 않았다.

## 제한과 다음 범위

- 이전 CooldownPolicy의 Owner, CooldownEffect, CooldownTags, EffectLevel 값은 새 필드로 자동 변환하지 않는다. 기존 에셋은 Duration과 필요한 Shared Group Tags를 다시 지정해야 한다.
- 프리뷰는 PIE/게임 세션 초기화를 대체하지 않는다. GameInstance·Controller·PlayerState 의존 처리는 자동 제공하지 않는다.
- 프리뷰에 호출 Gameplay Ability는 없다. 프로젝트 확장 태스크는 이 환경을 고려해야 한다.
- 조건 객체·배열·태그 컨테이너는 해당 프로퍼티 전체를 오버라이드한다.
- Map/Set 및 구조체 전체 내부의 복잡한 Instanced 객체 복제는 미지원이다.
- AfterMeshPose는 실제 엔진 갱신 시점 연결 전까지 오류로 처리한다.
- 타임라인 의존성 시각 편집은 미구현이다. 복사·붙여넣기는 한 번에 한 태스크만 지원한다.
- 태스크 클립보드는 에디터 세션 동안만 유지하며 OS 클립보드나 다른 프로세스와 공유하지 않는다.
- 그래프 입력 버퍼·다중 액션 채널은 미구현이다. 트리거는 SendTrigger 호출 시점에만 평가한다.
- SubGraph는 재사용 단위와 진입·종료 Context 계약이 정해지지 않아 보류한다. Conduit은 현재 Edge Condition과
  Node EntryCondition으로 같은 판정을 표현할 수 있어 추가하지 않는다. Alias는 여러 액션 노드가 같은 UKataAction을
  참조할 수 있어 현재 필요성이 없으며, 실행 추적상의 별도 노드 정체성이 필요해질 때 다시 검토한다.
- 월드 실행 Subsystem은 인스턴스를 순차 진행하는 1단계 구조다. 모든 인스턴스의 상태를 먼저 수집한 뒤
  효과를 일괄 반영하는 다단계 Gather/Commit 모델은 아직 구현하지 않았다.
- ProjectKataTesting과 Content/KataTest는 에디터·개발 검증 전용이다. cooked Game에서 테스트 하네스를
  사용하려면 별도의 개발 패키징 정책을 먼저 정해야 한다.

사용법은 [Editor-Usage.md](../manual/Editor-Usage.md), [Runtime-Usage.md](../manual/Runtime-Usage.md)를 따른다.
후속 범위는 [Next-Work-Plan.md](../plan/Next-Work-Plan.md)에 정리했다.

## 사용자 빌드 오류 대응

- KataAssetEditor.cpp의 존재하지 않는 CoreUObjectDelegates.h include를 UE 5.8에서 FCoreUObjectDelegates를 선언하는 UObjectGlobals.h로 수정했다. 수정 후 빌드·검사는 실행하지 않았다.
- SKataPreviewViewport의 `TUniquePtr<FKataPreviewScene>`이 전방 선언 상태에서 삭제 코드를 인스턴스화하던 C4150 오류를 수정했다.
  소유 포인터는 완전한 기반 타입 `FPreviewScene`을 사용하고 실제 객체만 배경색 지원 파생 타입으로 생성한다. 수정 후 빌드·검사는 실행하지 않았다.

## 한국어 주석 검수

- 플러그인과 프로젝트 테스트 코드의 한국어 주석을 검수해 폐기된 타입명, 구현과 맞지 않는 설명,
  번역투와 불완전한 명사형 문장을 정리했다.
- `UKataInstance` 표기를 현재 타입인 `UKataActionInstance`로 고치고, 프리뷰 위젯이 지원하지 않는
  크기 조절 설명과 제거된 Blueprint/CDO 기반 경로의 표현을 삭제했다.
- 벤더 코드의 원문 주석은 유지했으며 실행 로직, 문자열 리터럴과 리플렉션 메타데이터는 변경하지 않았다.
- 공개 API와 구현 주석의 작성 기준을 `AGENTS.md`에 추가했다.
- 주석만 변경했으며 빌드·UHT·테스트·UI 실행은 수행하지 않았다.
