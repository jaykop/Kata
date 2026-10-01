# Kata 구현 상태

갱신: 2026-09-30  
기준: 현재 작업 트리의 소스·설정과 기존 사용자 확인 기록. KataCamera 뼈대·Spline 레일(#20 CAM-1·2, 사용자 샘플 테스트 완료)와 캐릭터 데이터·비동기 생성(#26, cce1398)을 포함한다.

## 현재 기준

UE 5.8, GAS 필수, 싱글플레이 전용이다. 재사용 코드는 플러그인, ProjectKata는 샘플,
ProjectKataTesting은 개발 하네스를 담당한다. 네트워크·예측·BT 어댑터는 범위 밖이다.
KataAI는 StateTree 기반으로 계획되어 있으나 아직 플러그인을 구현하지 않았다.

| 플러그인·모듈 | 현재 제공 범위 |
|---|---|
| Kata / KataConditions | Tag·Attribute·Distance·Angle·Group 조건, Context와 순수 판정 함수 |
| Kata / KataRuntime | 액션 에셋·상속 해석, Task·Command·실행기, GAS 연결, 기본 태스크 5종 |
| Kata / KataGraph | Entry·Action·Conduit·Alias, 엣지·전이 창·트리거·대상 유지, 실행 컴포넌트 |
| Kata / KataEditor·KataGraphEditor | 액션·타임라인·프리뷰와 그래프 에디터 |
| KataFramework / KataFramework | ASC·액션·그래프·타게팅·HitBox 컴포넌트와 팀 인터페이스를 갖춘 AKataCharacter, PC용 타게팅·입력 처리 컴포넌트를 갖춘 AKataPlayerCharacter, UKataInputConfig, AKataPlayerController. 공통·PC·NPC 캐릭터 테이블 행, 비동기 로드·생성 서브시스템, Blueprint 생성 노드, 행으로 PC를 생성하는 AKataGameMode. Hit Trace 태스크·프리셋·HitBox·HurtBox 컴포넌트·Subsystem·처리기·프로젝트 설정(Kata Hit Trace) |
| KataFramework / KataFrameworkEditor | 액션 에디터 프리뷰 툴바의 Hit Trace 디버그 토글 |
| KataTargeting / KataTargeting | 팩션 설정·관계표·팀 번호 연결·UKataFL_Faction, 타게팅 기반·PC 컴포넌트(소프트 타겟·락온), Preset 확장 태스크 4종, 대상·방향 결정 Command 2종, 회전 태스크 |
| KataCamera / KataCamera | 단계 파이프라인을 실행하는 AKataPlayerCameraManager, 카메라 데이터 UKataCameraData, Boom Arm·Spline Rail 배치, GameplayTag 지정 UKataCameraRailComponent, Feature 기반 클래스 UKataCameraFeature, GameplayDebugger 글·피벗 점·2D 레일 패널 |
| ProjectKata | 샘플과 게임별 태그 생성. GameplayTags에 의존 |
| ProjectKataTesting | bBuildDeveloperTools 대상의 테스트 액터·콘솔·디버그 태스크 |

코어는 위성·통합 플러그인을 참조하지 않는다. KataTargeting은 GameplayTags·AIModule·DeveloperSettings·TargetingSystem과
GameplayAbilities(Private)를 사용하며, 대상 결정 Command와 회전 태스크 때문에 Kata 코어(KataRuntime)에 의존한다. KataFramework는 캐릭터 조합 때문에 KataGraph·KataTargeting·AIModule에, 플레이어 입력 때문에 EnhancedInput에, Hit Trace 대상 필터 때문에 엔진 TargetingSystem에, 프로젝트 설정 때문에 DeveloperSettings에, 기본 카메라 매니저 지정 때문에 KataCamera(Private)에 의존한다. KataCamera는 엔진 모듈(Core·CoreUObject·Engine·GameplayTags)과 GameplayDebugger 지원에 의존하며 코어 Kata를 아직 참조하지 않는다. 목표 분리 구조는 [#1](https://github.com/jaykop/Kata/issues/1)을 따른다.

## 원본 에셋과 실행

- UKataAction 객체 uasset → UKataResolvedAction 사본 → UKataActionInstance 실행 상태로 분리한다.
- 액션 실행 컴포넌트는 UKataActionComponent다. 2026-09-26 UKataComponent에서 이름을 바꿨고(#16), 클래스·캐릭터 멤버·Blueprint 함수 Redirect를
  DefaultEngine.ini에 둔다. 서브오브젝트 이름과 델리게이트 타입 이름도 바꿨다. 2026-09-26 사용자가 빌드와 기존 액션·그래프 에셋 열기를 확인했다. [기록](2026-09-26-KataActionComponent-Rename.md).
- ParentAction 체인과 명시적 OverriddenSettings·TaskOverrides를 병합한다. TaskId는 상속 태스크의 정체성이다.
  부모와 값이 같아져도 오버라이드를 자동 제거하지 않는다. 순환 부모는 거절하고 매 실행 새로 해석한다.
- 정책의 직접 구조체 필드는 개별 경로를 사용한다. 조건·배열·태그와 Pre/Post Commands는 전체를 덮어쓴다.
  Instanced 객체는 직접 객체·객체 배열·직접 객체 필드를 가진 구조체 배열까지 복제한다. Map/Set·더 깊은 중첩은 미지원이다.
- Blueprint/CDO 기반 UKataDefinition·클래스 실행 API·Import Legacy는 제거했다.
- PlayKataAction·PlayKataActionOnSelf·CanPlayKataAction을 제공한다. ASC가 필수이며 시작 실패 사유를 반환한다.
- 월드 Subsystem은 ExecutionPriority 오름차순과 시작 순서로 Actor·Component Tick 전에 진행한다.
  Subsystem을 사용하지 못하면 컴포넌트 Tick이 진행한다. 다단계 Gather/Commit은 없다.
- 시작 시 GAS 활성 상태·Pre Commands·시각 0 태스크를 즉시 처리한다. 순간 액션은 Play 반환 전에 끝날 수 있다.
- 태스크 Tick은 게임 프레임당 최대 한 번이다. Duration 0은 Tick 없이 끝나고 Single Frame은 한 번 Tick한다.
- 루프는 회차 끝 정리 후 다음 프레임에 재시작하며 초과 시간을 넘기지 않는다. MaxLoopCount는 최초 포함, 0은 무한이다.
  Restart On Loop를 끈 태스크는 이후 Skipped다. MaxIterationsPerTick은 제거했다.
- 종료는 태스크 정리 → Post Commands → GAS 회수다. 첫 종료 요청 사유를 보존한다.
- Pre Commands 동안만 SetTargetActor를 허용한다. 구체 Command는 아직 없고 BP·C++ 확장 기반을 제공한다.

사용 계약은 [런타임 사용법](../manual/Runtime-Usage.md), 제작은 [Task-Authoring](../manual/Task-Authoring.md),
결정 이유는 [에셋 모델](2026-09-25-Action-Asset-Model.md)과 [실행 수명](2026-09-25-Execution-Lifecycle.md)에 있다.

## GAS와 기본 태스크

- 내부 Duration GE로 쿨다운을 관리한다. Enabled·Duration·Start Time·Shared Group Tags를 작성한다.
  공유 태그가 비면 SourceAction별 쿨다운이고 On End는 Branched를 포함한 모든 종료에 적용한다.
- 기본 태스크는 Play Montage, Send Gameplay Event, Apply Gameplay Effect, Apply Loose Tag, Transition Window다.
- 이벤트·GE·Loose Tag의 대상은 Avatar·Owner·ContextTarget이다. Avatar·Owner는 실행 ASC를 공유한다.
- 몽타주는 호출 Ability 유무에 따라 ASC 또는 AnimInstance 경로로 재생한다. 재생 실패가 액션 시작 실패로 반환되지는 않는다.
- GE는 제거 정책에 따라 받은 ASC의 적용 핸들을, Loose Tag는 같은 대상의 참조 수를 정리한다. Instant GE를 되돌리지는 않는다.
- AbilityTask는 일반 종료에서 OnCompleted·OnBranched·OnInterrupted를 구분한다.
  Activate 안에서 이미 끝난 인스턴스는 실제 종료 사유와 무관하게 OnCompleted(Completed)로 알리는 제한이 있다.
- Kata 자체 비용 정책·공용 Task 출력·태스크별 프리뷰 정책·VFX/SFX는 미구현이다.
- Hit Trace(KataFramework, [#6](https://github.com/jaykop/Kata/issues/6))는 구현 중이다. SocketTrace는 소켓 목록의 직전·현재 위치로 만든
  삼각형 띠와 HurtBox 도형의 직접 교차로, ShapeSweep은 엔진 Object Type Sweep으로 판정한다.
  판정 대상은 UKataHurtBoxComponent(Sphere·Capsule·Box, HurtBoxTags)뿐이다(HT-11). 프로젝트 설정 Kata Hit Trace의 HurtBoxCollisionProfile이
  HurtBox 기본 콜리전과 판정 Object Type을 정하고, 프리셋 HurtBoxTagQuery로 거르며, BoneName은 HurtBox 부착 소켓이다.
  Send Gameplay Event 처리기는 HurtBox 태그를 TargetTags로 넘긴다. 샘플은 Object Channel·프로필 KataHurtBox를 정의한다.
  서브스텝 보간·시작·종료 시점 판정·대상 1회·TargetingPreset 필터·Subsystem 순차 처리기·프리뷰 판정·디버그 표시를 포함한다.
  프레임 사이 포즈는 활성 몽타주를 재샘플링하고 양 끝 실제 포즈와의 차이를 보간해 호를 따라간다(HT-10, 프리셋 bSampleAnimation, 몽타주가 없으면 선형).
  다단히트·사용 설명서는 남아 있다. 프리셋의 TraceChannel·bTraceComplex는 없앴다(테스트 에셋만 영향). 설계는 [Hit Trace 계획](../plan/Hit-Trace-Plan.md)을 따른다.

설정과 실패 계약은 [기본 태스크 설명](../manual/Runtime-Usage.md#기본-태스크), 이유는 [GAS 결정 기록](2026-09-25-GAS-and-Tasks.md)에 있다.

## 조건·팩션·게임플레이 태그

- Distance는 SocketName이 비면 Actor 위치, 있으면 ACharacter 기본 Mesh 소켓을 사용한다. Mode·ComponentTag는 제거했다.
- Group은 All/Any 단축 평가다. 평가한 자식의 Invalid를 전파하며 빈 배열·null·직접 자기 참조를 거절한다.
- UKataFL_Condition의 CompareValue·CheckDistance·CheckAngle·CheckTag는 순수 함수다. Context·Invert·결과 변환은 조건이 맡는다.
- KataTargeting의 Kata Factions는 태그 목록과 방향 없는 관계표를 사용한다. 앞의 255개 인덱스가 팀 번호다.
  관계는 구체적인 규칙 우선, 없으면 같은 팩션 우호·나머지 중립이다. NoTeam은 중립이다.
- 모듈 시작에 전역 팀 관계 함수를 등록하고 종료에 기본 함수를 복원한다. 액터·폰 컨트롤러에서 팀을 조회한다.
  AKataCharacter가 팀 인터페이스로 타게팅 컴포넌트의 팩션 팀 번호를 돌려준다(#17). AIController의 팀 인터페이스는 KataAI(#22)에서 만든다.
- 타게팅(#13 TG-3): 기반 `UKataTargetingComponent`가 팩션과 `GetCurrentTarget`·`ResolveActionTarget`(BlueprintNativeEvent),
  Preset 즉시 실행 헬퍼를 제공한다. PC용 `UKataPlayerTargetingComponent`는 소프트 타겟과 락온(획득·좌우 전환·해제)을 관리한다.
  락온 중에만 Tick(기본 0.1초)으로 거리·대상 ASC 태그를 확인하고, 파괴는 OnEndPlay로 즉시 처리한다. 해제 시 동작은 해제 또는 다음 대상이다.
  확장 태스크: Kata Filter Faction, Kata Filter Lock Side, Kata Sort Screen Center, 가중치 정렬 기반 `UKataTargetingSortTask_Weighted`.
  디버그 CVar `Kata.Targeting.Debug`는 `ENABLE_DRAW_DEBUG`로 감싼다. 몬스터 파생 컴포넌트는 아직 없다.
  2026-09-25 사용자가 빌드와 Targeting Preset의 Kata 태스크 표시를 확인했다. 사용법은 [타게팅 사용법](../manual/Targeting.md).
- 대상·방향 결정(#13 TG-4): PreCommand `Resolve Target`은 컴포넌트의 `ResolveActionTarget`으로 대상을 정하고,
  Keep Valid Target(기본 켬)이어도 `CanKeepActionTarget`이 거부하면 다시 구한다. `Resolve Facing`은 `ResolveFacingDirection` 방향으로 즉시 돌리고,
  `Kata Task: Rotate To Facing`은 구간 동안 Rotation Rate(기본 720°/s)로 돌린다(방향 매 Tick 갱신 기본 켬).
  PC의 공격 방향 우선순위는 락온 대상 → 이동 입력 → 소프트 타겟 → 정면 유지다. 소프트 타겟은 방향 기준이므로 락온이나 이동 입력이 있으면 정하지 않는다.
  이동 입력은 폰의 이동 입력 벡터로 읽으므로 이동 입력을 무시하는 동안에는 없는 것으로 본다. 결정 이유는 [TG-4 기록](2026-09-26-Targeting-Resolve-Commands.md).
- 프로젝트 Config/Tags/Native ini에서 PreBuildSteps가 KataTags.h/.cpp를 생성한다. 생성 파일은 커밋하지 않는다.
  게임별 태그는 샘플이 소유하며 플러그인은 FGameplayTag 값을 받는다. 조건 테스트의 정적 테스트 태그는 예외다.

[Conditions](../manual/Conditions.md), [Factions](../manual/Factions.md), [Gameplay-Tags](../manual/Gameplay-Tags.md)를 따른다.
태그 생성 이유는 [기존 구현 기록](2026-09-24-Gameplay-Tag-Generation.md)에 보존한다.

## 그래프 계층

- GenericGraph 기반 자료구조와 병렬 엣지를 사용한다. 출처·변경은 [UPSTREAM.md](../../Plugins/Kata/Source/KataGraph/UPSTREAM.md)에 있다.
- Entry·Action·Conduit·Alias를 제공한다. 머무를 수 있는 노드는 UKataNode::IsExecutableState가 정하며 Action만 참이다.
- Conduit은 머무르지 않고 실행 가능한 Action까지 해석한다. 지나는 노드와 엣지의 조건이 하나라도 막히면 전이가 성립하지 않는다.
- Alias는 포함된 노드에서 나가는 전이를 대신한다. Any State를 켜면 머무를 수 있는 모든 노드를 포함하고, 별칭 전이는 자기 자신을 목표로 삼지 않는다.
- Conduit을 지난 엣지는 떠나는 액션이 없어 Window와 Timing을 보지 않는다. Alias 엣지는 현재 액션을 떠나므로 둘을 적용한다.
- Trigger는 계층 매칭, Window는 액션의 Transition Window 태스크가 연다. 자동 전이는 시작·정상 완료 때 평가한다.
- Priority 내림차순과 저장된 자식·엣지 순서로 고른다. Immediate는 Branched, OnActionEnd는 정상 완료 후 예약 전이다.
- Keep Target은 기본 true이며 진입 엣지는 시작 Context를 사용한다. 액션 시작 후 바뀐 대상을 그래프에 기록한다.
- 엣지 Trigger Event Tag의 선택 목록은 `Trigger.*`, Required Action Window Tag와 Transition Window 태스크의 Window Tag는 `TransitionWindow.*`로 거른다.
  입력 계층은 `Input.*`를 `Trigger.*`로 바꿔 보낸다. 샘플 창 태그는 `TransitionWindow.Combo`·`TransitionWindow.Cancel`이다.
- 입력 버퍼·SubGraph·다중 액션 채널은 없다. 동기 전이 32단계 제한이 있다. Failed 종료 사유는 도입하지 않았다.
- Conduit이 막다른 길일 때 전이가 조용히 성립하지 않는다. 이를 잡는 에디터 검증은 없다.

사용법은 [에디터](../manual/Editor-Usage.md#그래프-에디터)·[런타임](../manual/Runtime-Usage.md#콤보-그래프-실행), 이유는
[그래프 결정 기록](2026-09-25-Graph-Transition.md), 에디터 패널 쪽 결정은
[그래프 에디터 패널 기록](2026-09-26-Graph-Editor-Panels.md)에 있다.

## 전용 에디터

- Preview·Timeline·Kata Action Details·Timeline Details·Preview Details를 제공한다. 레이아웃과 펼침 상태를 저장한다.
- 태스크 추가·복수 선택·시간 이동·양끝 길이 조절·설정·삭제·Undo/Redo, 단일 태스크 복사·붙여넣기를 제공한다.
- 시간 눈금·Snap·Current Time·Length·Resize, 태스크 자동 색·주석과 편집 전용 그룹을 제공한다.
  Snap은 화면 8픽셀 안의 대상에만 붙고, 대상(Tasks·Playhead·Interval)을 Snap To에서 고른다. 기본값은 Tasks·Playhead다.
  재생 헤드는 시간 눈금 영역에서만 움직이며 스냅하지 않는다. 편집으로 장면을 다시 만들어도 재생 헤드 시각을 유지한다([#15](https://github.com/jaykop/Kata/issues/15)).
  그룹은 실행 순서에 영향이 없고 부모에서 상속하지 않는다.
- 미완성 설정은 저장 검사에서 경고로 다루지만 실행 해석 오류는 유지해 실행을 거절한다.
- 그래프는 에셋 이름을 따르는 Action 노드, 분리된 Details, Comment와 병렬 엣지 표시를 제공한다.
  Comment는 선택한 노드를 감싸고 선택이 없을 때만 커서 위치에 만든다.
- Ctrl+F로 노드와 전이를 찾는다. 노드 제목·주석·핀에 더해 액션 에셋 이름과 경로, 트리거 태그, 전이 창 태그로 찾을 수 있다.
- Alias의 출발지는 그래프 안의 노드 목록에서 체크 상자로 고른다. 노드가 에셋이 아니라 기본 오브젝트 피커를 쓸 수 없기 때문이다.
- 태스크 의존성 시각 편집은 없다. 클립보드는 에디터 세션 내 단일 슬롯이며 OS 클립보드와 공유하지 않는다.

## 프리뷰

- 별도 EditorPreview 월드에서 Self·Target 한 쌍과 충돌 바닥을 만든다. 클래스가 없으면 빈 액터를 둔다.
  기본 Target은 -X 500cm, 양쪽 벽은 기본 꺼짐이다. 프리뷰 설정은 에셋의 editor-only 데이터다.
- 엔진 뷰포트 툴바·월드 축 직교 카메라와 Select Self/Target 이동·회전 위젯을 사용한다. 크기 조절은 없다.
- Play/Pause·Stop/Reset·Repeat는 Timeline에서 조작한다. 살아 있는 실행은 Pause 후 이어가며 종료된 실행은 새로 시작한다.
- 탐색은 실제 실행 시뮬레이션이다. 앞으로는 이어가고 뒤로는 0초부터 진행한다. 입력은 다음 Tick에 마지막 요청으로 처리한다.
  1/60초 단계마다 GFrameCounter를 늘려 동기 World Tick을 수행하고 프리뷰 소리를 끈다. 직접 UAnimPreviewInstance 평가는 제거했다.
- 프리뷰 메시를 AlwaysTickPoseAndRefreshBones로 갱신하고 Character의 컨트롤러 없는 이동을 준비한다.
- 호출 Ability·GameInstance·Controller·PlayerState·AttributeSet·수신 Ability 초기화를 자동 제공하지 않는다.
- 긴 역방향 탐색 비용, 종료된 인스턴스의 대기 월드 Tick, 일반 재생 뒤 클릭 시 Preview Transform 기록 가능성이 남아 있다.
  루트 모션 미이동·Pause 재시작 사용자 보고의 해당 환경 원인·해소 여부는 이번에 확인하지 않았다.

현재 절차·문제 해결은 [Editor-Usage](../manual/Editor-Usage.md), 엔진 제약·이력은
[프리뷰 시뮬레이션 기록](2026-09-24-Preview-Scrub-Simulation.md)에 있다.

## 입력 계층

- 폰의 `UKataInputHandlerComponent`가 `UKataInputConfig`를 참조해 Enhanced Input을 바인딩한다. 폰의 컨트롤러 변경 델리게이트로
  로컬 플레이어 컨트롤러에 빙의되는 동안 기본 IMC를 추가하고, 빙의가 풀리면 제거한다. `AKataPlayerCharacter`는 이 컴포넌트를 기본으로 가진다.
- 입력 설정은 기본 IMC 목록, 이동·시점 InputAction, InputAction 이벤트를 `Input.*` 태그로 바꾸는 Input Bindings, `Input.*`를 `Trigger.*`로 바꾸는 Trigger Mappings를 가진다.
  이동은 컨트롤 회전의 Yaw 기준, 시점은 Yaw·Pitch 입력이다.
- 입력 처리 컴포넌트는 Trigger 태그를 지정된 Graph로 보낸다. 그래프는 콤보가 끝나면 종료되므로 실행 중이 아니면 대상 없이 새로 시작하고,
  새로 시작한 그래프가 진입하지 못하면 Cancelled로 멈춘다. 그래프를 상주시키지 않는 것은 사용자 결정이다. 대상은 액션의 Resolve Target Command가 정한다.
- 그래프 엣지 Trigger Event Tag와 `SendTrigger` 핀은 `Categories = "Trigger"`로, 입력 설정의 Input 태그는 `Input`으로 에디터 선택 목록을 거른다.
- 바인딩을 폰 쪽에 둔 이유는 캐릭터마다 입력 설정이 다를 수 있고, PC 캐릭터 행([#26](https://github.com/jaykop/Kata/issues/26))이 설정을 채우기 때문이다.
  이후 늘어날 입력 로직이 캐릭터 클래스에 쌓이지 않도록 컴포넌트로 분리했다. `UEnhancedInputComponent`는 상속하지 않는다.
- IMC 추가·제거는 엔진 `UEnhancedInputLocalPlayerSubsystem`의 Blueprint 노드를 쓰며 Kata API는 두지 않는다.
- `AKataPlayerController`는 어떤 폰을 조종하든 유지할 IMC 같은 플레이어 단위 기능을 둘 자리다. 현재는 생성자에서 `PlayerCameraManagerClass`를 `AKataPlayerCameraManager`로 지정하는 일만 한다.
- 락온 입력과 입력 버퍼는 아직 없다. 카메라는 아래 카메라 절을 따른다. 순서는 [입력 계층 계획](../plan/Input-Plan.md)과 [#19](https://github.com/jaykop/Kata/issues/19)를 따른다.

사용법은 [입력 사용법](../manual/Input.md)에 있다.

## 캐릭터 데이터와 비동기 생성

- `FKataPlayerCharacterRow`와 `FKataNPCCharacterRow`는 소프트 참조로 Character Class·메시·Anim Blueprint를 지정한다. PC 행은 입력 설정과 그래프도 지정한다. 빈 선택 항목은 캐릭터 Blueprint 기본값을 유지한다.
- `UKataCharacterSpawnSubsystem`은 `FDataTableRowHandle`의 행을 요청 시점에 복사하고 에셋을 비동기로 로드한다. 지정 에셋의 로드 실패는 생성 실패로 알린다. `AKataCharacter`는 Blueprint Construction Script 이후 `OnConstruction`에서 행을 적용한다. 요청 중 테이블을 강한 참조로 유지하며, 취소·월드 정리 시 로드와 Blueprint 비동기 노드의 등록을 해제한다.
- `AKataGameMode`는 PC 행이 설정되면 기본 폰 대신 비동기로 생성해 빙의시킨다. 행이 비면 엔진 기본 폰 경로를 따른다. 로드 중에는 폰이 없다. 샘플 에셋은 로컬 `Content/KataTest`에 있으며 저장소에서는 추적하지 않는다.
- 사용법은 [캐릭터 데이터 사용법](../manual/Character-Data.md), 적용 순서의 진단과 변경 이유는 [캐릭터 행 적용 기록](2026-09-30-Character-Row-Spawn.md)을 따른다. GAS 데이터 에셋·로딩 화면·KataAI 항목은 아직 범위 밖이다.

## 카메라

- KataCamera 플러그인이 플레이어 카메라를 자체 구현한다. 엔진 GameplayCameras는 쓰지 않는다. 설계와 결정은 [카메라 시스템 계획](../plan/Camera-Plan.md)을 따른다.
- `AKataPlayerCameraManager`가 `UpdateViewTargetInternal`에서 파이프라인을 실행한다. 순서는 컨트롤 회전 → Rotation Feature → 피벗 계산 → 카메라 데이터의 Placement → Framing·Constraint·Reaction Feature다.
  카메라 액터·디버그 카메라 스타일·`UCameraModifier`·렌즈 효과는 엔진 `UpdateViewTarget`이 그대로 처리한다.
- 뷰 타깃이 폰이 아니거나 적용할 카메라 데이터 또는 Placement가 없으면 엔진 기본 계산을 쓴다. Placement 누락 경고는 데이터마다 한 번 남긴다.
- `UKataCameraData`는 피벗 오프셋(X·Y는 카메라 Yaw 기준, Z는 월드), FOV, 피치 제한, Placement 인스턴스를 가진다. 피치 제한은 적용 데이터가 바뀔 때 매니저의 `ViewPitchMin/Max`에 반영한다.
- 배치 방식은 카메라 데이터의 `UKataCameraPlacement` 인스턴스로 고른다. `Boom Arm`과 `Spline Rail`을 제공하며 배치 객체는 공유 에셋의 설정을 const로 평가한다. 배치 자체는 충돌을 처리하지 않으며 Constraint 단계의 `Shrink` Feature가 맡는다.
- `UKataCameraRailComponent`는 캐릭터 Blueprint에서 편집하는 Spline과 GameplayTag를 제공하며 생성자에서 Closed Loop의 기본값을 false로 지정한다. 기존 BP의 true 오버라이드는 별도로 해제한다. Spline 배치는 현재 뷰 타깃 폰에서 정확히 일치하는 하나의 레일을 사용한다. 컴포넌트 원점이 피벗이며 상대 회전·스케일에 카메라 Yaw를 적용한다. 입력 Pitch는 레일 길이에 대응하고 조준점·FOV 커브는 CameraData의 배치 서브오브젝트에서 설정한다.
- 잘못된 레일은 경고와 설정 거리의 Boom Arm으로 대체하며 Data.PivotOffset을 사용한다. 폰·태그·컴포넌트 변화는 다음 갱신에 반영한다. 레일 참조·진단은 매니저에, 정규화 입력과 궤도 오프셋은 프레임 결과에 두며 공유 배치에 실행 상태를 저장하지 않는다.
- `UKataCameraFeature`는 단계·우선순위·켜기 여부를 갖는 기반 클래스이며 매니저가 인스턴스를 소유한다. 구체 Feature로 장애물 Shrink(`UKataCameraFeature_Shrink`)가 있다. 피벗에서 카메라까지 구 스윕한 뒤 같은 광선 위로 당기며, 당김 비율을 즉시 당김·느린 복귀로 보간한다. Feature는 `GetDebugString()`으로 GameplayDebugger에 상태를 보인다.
- 적용 데이터는 매니저의 `DefaultCameraData` 하나다. 카메라 StateTree(CAM-4), 락온(CAM-5), 디더링(CAM-6)은 아직 없다. 순서는 [#20](https://github.com/jaykop/Kata/issues/20)을 따른다.
- GameplayDebugger 카테고리 `KataCamera`는 글·피벗 점과 Canvas의 작은 XZ 레일 패널을 제공한다. 카메라 연결 선은 제외했고 Unpossess 후 월드 DebugDraw 미표시는 기존 제한이다. Canvas 패널은 월드 도형 표시 여부와 독립적이지만 관전 모드의 새 패널 동작은 미확인이다. `WITH_GAMEPLAY_DEBUGGER`가 꺼진 대상에서는 빠진다.

사용법은 [카메라 사용법](../manual/Camera.md), CAM-2의 결정 이유는 [Spline 레일 기록](2026-09-30-Camera-Spline-Rail.md)에 있다.

## 프로젝트 테스트 하네스

ProjectKataTesting은 DeveloperTool이며 bBuildDeveloperTools가 꺼진 대상에 포함하지 않는다.
코드 액션 예제·AKataTestActor·Kata.Resolve/Play/List/Stop 콘솔을 제공한다.
ProjectKata에서 옮긴 테스트 타입과 KataFramework로 옮긴 캐릭터의 Redirect는 샘플 설정에 있다.
Content/KataTest는 NeverCook이며 cooked Game용 하네스 정책은 없다.
[개발 하네스 사용법](../manual/Runtime-Usage.md#개발-하네스)과 [에셋 이전 안내](../manual/Asset-Migration.md)를 따른다.

## 확인 범위

| 대상 | 기존 사용자 확인 | 남은 확인 |
|---|---|---|
| 액션 리네임·GenericGraph·테스트 모듈 분리 | Editor 빌드, 액션·그래프 에셋 생성. 정확한 일자 미상 | 기존 테스트 에셋 Redirect·하네스 회귀 |
| 조건 Function Library | 빌드 성공, 정확한 일자 미상 | 모든 BP 노드·각 조건 실행 |
| Distance 위치 단순화 | 2026-09-24 빌드, Distance 테스트 두 건, 에디터 Socket 판정 | 다른 조건 전체로 확대하지 않음 |
| 프리뷰 시뮬레이션 | 2026-09-24 빌드·탐색 동작 확인 보고 | 항목별 결과는 따로 보고되지 않음. 이번 대화의 루트 모션·Pause 문제 해소는 미확인 |
| AKataCharacter 이동 | 2026-09-24 빌드. 사용자가 캐릭터 BP를 새로 제작 | 이전 BP Redirect 성공 여부 |
| 대상·방향 결정(#13 TG-4) | 2026-09-26 Editor 빌드, PreCommands 목록의 Resolve Target·Resolve Facing과 태스크 목록의 Rotate To Facing 표시, 프리뷰에서 Rotate To Facing 회전 동작 | 락온·이동 입력 우선순위와 콤보 대상 유지의 런타임(입력 계층 #19 이후) |
| 캐릭터 조합(#17) | 2026-09-26 Editor 빌드, 기존 에셋 열기, AKataPlayerCharacter 파생 BP 생성과 타게팅 컴포넌트의 PC 항목 표시. BP_SampleCharacter의 BP HitBox 컴포넌트는 사용자가 제거 | 실제 액터 팩션 판정·락온 런타임 |
| 입력 계층(#19 IN-1~IN-3) | 2026-09-27 Editor 빌드, `LV_TestMap` PIE에서 WASD 이동과 마우스 시점 확인, 컴포넌트 분리 후 재확인. 마우스 왼쪽으로 그래프 시작과 공격 액션 실행, 반복 입력, 태그 선택 창의 Input·Trigger 거르기 확인. `TransitionWindow.Combo` 창으로 공격 1 → 2 Immediate 콤보 전이 확인 | 엔진 노드로 IMC 제거·추가, 폰 교체 시 IMC 교체, Alias 캔슬, 게임패드, Game 타깃 |
| 캐릭터 데이터·비동기 생성(#26) | 2026-09-30 사용자 에디터 실행에서 테이블 행의 교체 Mesh가 적용되지 않는 현상을 확인해 적용 위치를 수정했다. Editor 빌드의 C2039/C2065를 `FWorldDelegates`로 수정한 뒤, 사용자가 재빌드와 PIE 테스트 완료를 보고했다 | PC·NPC 행의 Mesh·Anim, 잘못된 행, 취소·월드 정리 등 개별 시나리오의 결과는 별도 보고되지 않음 |
| Command·Keep Target | 2026-09-24 빌드·Details 표시 | 런타임 실행 |
| 팩션 | 2026-09-24 빌드·설정 화면·BP 함수 노출 | 실제 액터 관계 판정 |
| 프로젝트 태그 생성 | 2026-09-24 Rider 빌드·Tag Manager·에디터 태그 추가 | Game 타깃·패키징·오류 입력 출력 |
| 개별 태스크·Loop·Single Frame | 해당 기능의 별도 결과 기록 없음 | 게임 실행·자원 회수·경계 동작 |
| Hit Trace(#6) | 2026-09-25 Editor 빌드, 프리뷰에서 SocketTrace 면 판정·히트 표시 확인. 2026-09-26 프리뷰 정상 프레임과 t.MaxFPS 20에서 재샘플링 판정 확인, 20fps 진단 로그로 판정 창 전체 판정과 직전 포즈 기록 확인. 2026-09-26 HurtBox(HT-11) Editor 빌드 후 프리뷰에서 KataHurtBox_Body 히트와 BoneName(pelvis)을 로그로 확인 | 게임 실행·ShapeSweep·필터·처리기 수신·주황 교차 지속 표시·프리뷰 디버그 저장 |
| 그래프 Comment·노드 검색(#3) | 2026-09-26 Editor 빌드. Comment가 선택한 노드를 감싸는 동작과 노드 검색 확인 | 없음 |
| 그래프 노드 타입 Conduit·Alias(#25) | 2026-09-26 Editor 빌드. Alias 디테일 패널의 노드 목록·토글 표시 확인 | 그래프 런타임 전이. 실행 수단이 없어 확인하지 못했다 |
| 카메라 뼈대(#20 CAM-1) | 2026-09-29 사용자 Editor 빌드 중 C4458(`PivotOffset` 이름 가림) 보고 후 수정. 이후 에디터에서 KataCamera 클래스로 `Content/KataTest/Camera`의 카메라 데이터·매니저 BP·컨트롤러 BP를 MCP로 생성·저장하고 `BP_KataTestGameMode`에 연결. 2026-09-30 GameplayDebugger 카테고리 표시 확인. 카메라를 향한 월드 선이 시야를 가려 제거했고, 사용자 요청에 따라 피벗 점만 다시 추가. Unpossess 후 월드 DebugDraw 미표시는 기존 현상으로 확인됨 | PIE 카메라 동작, 피치 제한, 피벗 점 복원 후 빌드·표시, Game 타깃 |
| Spline 카메라 레일(#20 CAM-2) | 2026-09-30 사용자 설계를 반영한 소스·문서 구현 후 로컬 샘플 설정·저장. 사용자가 샘플 테스트 완료를 보고했다. 에이전트 C++ 빌드·테스트·별도 검사는 미실시 | C++ 빌드·Game 타깃, 수동 BP 편집, Pitch·Yaw·패널의 세부 관찰 결과, 조준점·FOV 커브, 오류 대체·폰 교체·관전 모드의 개별 결과는 별도 보고되지 않음 |
| 장애물 Shrink(#20 CAM-3) | 2026-10-02 사용자 Editor 빌드, 매니저 BP Features에 Shrink 추가, PIE에서 벽·천장 근처 당김과 부드러운 복귀, GameplayDebugger의 ratio·blocked 표시 확인 | 피벗이 막힌 경우, Game 타깃 |
| CAM-2 로컬 샘플 설정 | 2026-09-30 에디터 재실행 후 `SetSplinePoints`로 BP_SamplePC에 열린 5점 레일·원점 Z 60cm·`Camera.Rail.Backview` 태그 설정. DA_KataTestCamera_Default를 Spline Rail·FOV 85°·Pitch -70°~60°·대체 거리 400cm로 설정하고 매니저 연결 유지. Blueprint 생성 클래스 갱신 후 세 에셋 저장 완료 | 샘플 uasset은 로컬 전용이며 저장소에서 추적하지 않음 |
| 타임라인 스냅 대상·재생 헤드 유지(#15) | 2026-09-25 재생 헤드 탐색 영역 제한까지 사용자 에디터 확인 | 범위 내 자석 스냅·Snap To 저장·편집 뒤 재생 헤드 유지의 빌드·실행 |

2026-09-25에는 문서와 관련 소스만 대조했다. 빌드·UHT·테스트·UI 실행·별도 코드 검사를 수행하지 않았다.
과거 문서의 C4996 경고 6건은 당시 빌드의 기록이며 현재 경고 수를 새로 확인하지 않았다.

## 기록과 후속 작업

설명서·결정 기록 목록은 [docs README](../README.md), 진행 상태는 [GitHub 이슈](https://github.com/jaykop/Kata/issues)에서 관리한다.
문서 부채 정비 결과는 [2026-09-25 기록](2026-09-25-Documentation-Maintenance.md)에 있다.
이전 쿨다운·Distance·Loop 필드와 에셋 모델의 자동·수동 처리 범위는 [이전 안내](../manual/Asset-Migration.md)에 모았다.
