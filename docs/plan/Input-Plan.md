# 입력 계층 계획

작성: 2026-09-26  
갱신: 2026-10-04  
연결 이슈: [#19 입력 계층 (KataFramework)](https://github.com/jaykop/Kata/issues/19) · 로드맵 [#24](https://github.com/jaykop/Kata/issues/24) 1단계  
현재 상태 근거: [작업 상태](https://github.com/jaykop/Kata/issues/19) · [액션 게임 기반 시스템 계획](Action-Game-Systems-Plan.md#2-인풋) · [플러그인 분리 모듈화 계획](Plugin-Modularization-Plan.md) · [타게팅 시스템 설계](Targeting-Plan.md) · [그래프 노드 타입 계획](Graph-Node-Types-Plan.md)  
대체 관계: 없음

## 목적과 현재 상태

플레이어 입력으로 KataGraph 콤보 전이, 이동·시점, 락온을 조작하는 입력 계층을 `KataFramework`에 만든다.
Input Mapping Context(IMC)를 명시적으로 추가·제거해 플레이어 캐릭터가 할 수 있는 행동을 제어한다.

현재 구현된 부분은 다음과 같다.

- 그래프는 `UKataGraphComponent::SendTrigger(FGameplayTag)`로만 전이 요청을 받는다. 트리거는 도착한 순간에 한 번만 평가하며 버퍼가 없다.
- 그래프는 마지막 액션이 정상 완료되고 자동 전이가 없으면 `Completed`로 끝난다. 실행 중이 아니면 `SendTrigger`는 false를 반환한다.
- 진입 대기 상태에서만 Entry 엣지를 평가한다. 액션 실행 중에는 현재 노드의 엣지를 먼저 평가하고, 이어서 현재 노드를 포함하는
  `UKataAliasNode`의 엣지를 평가한다([#25](https://github.com/jaykop/Kata/issues/25), 983e3af).
  - Alias의 `bAnyState`를 켜면 실행 중일 수 있는 모든 노드가 출발지가 된다. 캔슬 전이를 노드마다 긋지 않아도 된다.
  - Alias 엣지는 Required Window Tag와 Timing을 그대로 적용한다. Window 태그가 비어 있으면 액션 중 언제든 전이한다.
  - 우선순위가 같으면 노드에 직접 그은 엣지가 이긴다. Alias 엣지가 현재 노드 자신을 가리키면 건너뛴다.
  - Alias는 진입 대기 상태에서는 평가하지 않는다. 그래프가 비어 있을 때의 시작은 여전히 Entry 엣지가 담당한다.
  - Alias는 사용자 빌드·UI 확인까지 마쳤고 런타임 확인은 아직이다.
- `UKataAction`의 `ActiveGrantedTags`는 액션이 실행되는 동안 ASC에 loose 태그로 부여된다.
- `UKataPlayerTargetingComponent`는 소프트 타겟·락온 획득·좌우 전환·해제를, 기반 컴포넌트는 `ResolveActionTarget`을 제공한다.
- `AKataCharacter`는 ASC, `UKataActionComponent`, `UKataGraphComponent`, 타게팅 컴포넌트, HitBox 컴포넌트를 소유한다.
  `AKataPlayerCharacter`는 타게팅 컴포넌트를 `UKataPlayerTargetingComponent`로 만든다([#17](https://github.com/jaykop/Kata/issues/17), ab33690).
  캐릭터에 기본 그래프 지정 프로퍼티는 없다.
- `KataFramework`는 KataGraph·KataTargeting에 이미 의존한다. EnhancedInput 의존만 없다.

## 범위

- 포함: `AKataPlayerCharacter`의 입력 바인딩, `AKataPlayerController`, 입력 설정 데이터 에셋, IMC 추가·제거 API, 그래프 트리거 전달,
  이동·시점 입력, 락온 입력 연결, 샘플 IMC·InputAction 에셋.
- 제외: 입력 버퍼([#8](https://github.com/jaykop/Kata/issues/8)), AI 입력, 방향 조합·저스트 입력, 차지 시간 전달,
  입력 키 리바인딩 UI, 카메라 모드([#20](https://github.com/jaykop/Kata/issues/20)).

## 구성

### `UKataInputHandlerComponent` (KataFramework)

- 폰에 붙는 ActorComponent다. 입력 설정(`UKataInputConfig`)과 입력으로 구동할 콤보 그래프(`Graph`) 참조를 가진다.
  당분간 Blueprint 기본값으로 지정하고, 이후 캐릭터 정의([#26](https://github.com/jaykop/Kata/issues/26))가 생성 시 채운다.
- 폰이 `SetupPlayerInputComponent`에서 `SetupPlayerInput`을 호출하면 입력 설정의 InputAction을 `UEnhancedInputComponent`에 바인딩한다.
  `UEnhancedInputComponent`는 엔진이 빙의마다 만드는 바인딩 보관용 컴포넌트이며 상속하지 않는다.
- 폰의 `ReceiveControllerChangedDelegate`를 구독해, 로컬 플레이어 컨트롤러에 빙의되면 기본 IMC를 추가하고 빙의가 풀리면 제거한다.
  컨트롤러가 조종 폰을 바꾸면 이전 폰의 해제가 먼저 일어나므로 이전 폰의 IMC 제거 뒤 새 폰의 IMC가 추가된다.
- 이동·시점 입력을 처리하고, 이후 그래프 트리거는 폰의 `UKataGraphComponent`로, 락온은 폰의 `UKataPlayerTargetingComponent`로 전달한다.
- 캐릭터마다 입력 설정이 다를 수 있어 폰 쪽에 두되, IN-3 이후 늘어나는 입력 로직이 캐릭터 클래스에 쌓이지 않도록 컴포넌트로 분리한다.

### `AKataPlayerCharacter` (KataFramework)

- `UKataInputHandlerComponent`를 기본 서브오브젝트로 가지고 `SetupPlayerInputComponent`를 그 컴포넌트에 넘긴다.

### `AKataPlayerController` (KataFramework)

- 빙의한 폰과 무관한 플레이어 단위 기능을 맡는다. 어떤 폰을 조종하든 유지해야 하는 IMC(메뉴 등)가 생기면 여기에 둔다.
- 샘플 GameMode의 기본 컨트롤러로 지정한다.

### `UKataInputConfig` (UPrimaryDataAsset)

- **기본 IMC 목록**: IMC와 우선순위. 폰이 빙의될 때 추가한다.
- **입력 바인딩 목록**: InputAction, `ETriggerEvent`, Input 태그. 같은 InputAction에 이벤트별로 다른 Input 태그를 줄 수 있다.
- **Input → Trigger 매핑 목록**: Input 태그, 그래프에 보낼 Trigger 태그.
- **고정 기능 InputAction**: 이동, 시점, 락온 획득·해제, 좌·우 전환. 이 입력은 그래프를 거치지 않고 해당 기능을 직접 호출한다.

### Input 태그와 Trigger 태그

- Input 태그(`Input.*`)는 플레이어가 무엇을 눌렀는지를 나타낸다. 예: `Input.Attack.Light`, `Input.Attack.Heavy.Release`.
- Trigger 태그(`Trigger.*`)는 그래프가 받는 전이 이벤트다. 예: `Trigger.Attack.Light`. 그래프 엣지는 Trigger 태그만 사용한다.
- 입력 처리 컴포넌트는 Input 태그를 매핑 목록으로 Trigger 태그로 바꿔 `SendTrigger`에 넘긴다. 매핑이 없는 Input 태그는 그래프로 보내지 않는다.
- 그래프가 입력 장치와 키 구성을 알 필요가 없다. KataAI의 StateTree Task 같은 다른 발신자도 같은 Trigger 태그로 같은 그래프를 구동할 수 있다.
- 두 태그 모두 프로젝트가 소유하며 C++에서 참조하지 않으므로 `Config/Tags/Input.ini`, `Config/Tags/Trigger.ini`에 둔다.
  그래프 엣지의 Trigger Event Tag와 `SendTrigger` 매개변수는 `Categories = "Trigger"`로 에디터 선택 목록을 `Trigger.*`로 거른다.
  입력 설정의 Input 태그는 `Input`, 매핑의 Trigger 태그는 `Trigger`로 거른다. 실행 중 비교에는 영향이 없다(2026-09-27 사용자 결정으로 기존 "프로젝트 규약으로만 지킨다"에서 변경).
- 매핑은 Input·Trigger 선택 목록을 따로 거르기 위해 `TMap` 대신 구조체 배열로 둔다.

누름·홀드·뗌·연타 같은 입력 형태는 Enhanced Input의 Trigger(Pressed, Hold, Released, Tap 등)로 표현한다. 자체 입력 형태 시스템은 만들지 않는다.

### IMC 추가·제거로 행동 제어

- 행동 제어는 IMC를 추가하거나 제거하는 것으로 한다. 조건을 감시해 IMC를 자동으로 넣고 빼는 규칙 시스템은 만들지 않는다.
- 추가·제거는 엔진이 Blueprint에 공개한 `UEnhancedInputLocalPlayerSubsystem`의 Add Mapping Context·Remove Mapping Context를 쓴다.
  같은 기능을 감싸는 Kata API는 만들지 않는다(2026-09-27 결정).
- 폰의 기본 IMC는 입력 처리 컴포넌트가 관리한다.
- 같은 키를 여러 IMC가 쓰면 우선순위가 높은 IMC가 입력을 소비한다(`UInputAction::bConsumeInput`). 행동을 막을 때 IMC 제거와 우선순위 높은 IMC 덮어쓰기를 모두 쓸 수 있다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| 입력 계층 위치 | 확정 | `KataFramework`. 입력 바인딩과 기본 IMC 추가는 폰의 `UKataInputHandlerComponent`, 플레이어 단위 기능은 `AKataPlayerController`가 맡는다. 2026-09-26 사용자 결정으로 기존 "PlayerController와 입력 매핑"에서 캐릭터로 바꾸고, 2026-09-27 캐릭터에서 컴포넌트로 분리. [플러그인 분리 모듈화 계획](Plugin-Modularization-Plan.md) |
| `AKataPlayerController` 추가 | 확정 | 2026-09-26 사용자 결정 |
| 기본 IMC·InputAction 에셋 | 확정 | 2026-09-26 사용자 결정. 샘플 에셋은 프로젝트가 소유한다(플러그인 `CanContainContent` false) |
| IMC 추가·제거로 행동 제어 | 확정 | 2026-09-26 사용자 결정. 호출하는 쪽이 명시적으로 추가·제거한다. 조건에 따른 자동 전환은 만들지 않는다(같은 날 사용자 정정) |
| 락온 입력 | 확정 | 2026-10-04 사용자 결정. 획득·해제 토글 하나, 카메라 기준 왼쪽·오른쪽 전환 각각 하나로 총 세 InputAction을 쓴다. 입력 시작 시 한 번 처리하고, 스틱 방향은 타게팅에 쓰지 않는다. [#19](https://github.com/jaykop/Kata/issues/19) |
| 입력 버퍼 | 확정 | 이번 범위에서 제외. 실제 조작 후 [#8](https://github.com/jaykop/Kata/issues/8)에서 결정 |
| 같은 IMC를 여러 곳에서 추가할 때 | 확정 | 2026-09-26 사용자 결정. 호출 측의 실수로 보고 참조 횟수를 세지 않는다. 추가·제거를 그대로 반영한다 |
| 액션 중 IMC 추가 태스크 | 확정(만들지 않음) | 2026-09-26 사용자 결정. 액션 구간 동안 IMC를 넣는 Kata 태스크는 필요 없다 |
| 입력 설정 형태 | 제안 | 위 `UKataInputConfig` 구조 |
| 고정 기능 InputAction 위치 | 확정 | 2026-09-26 사용자 결정. 이동·시점·락온 InputAction도 `UKataInputConfig`에 둔다 |
| 트리거 태그 | 확정 | 2026-09-26 사용자 결정. Input 태그와 Trigger 태그를 나눠 매핑하고, 그래프는 Trigger 태그만 사용한다. 구조는 위 "Input 태그와 Trigger 태그" 항목 |
| 샘플 에셋 위치 | 확정 | 2026-09-26 사용자 결정. `Content/KataTest/Input`. 기존 샘플 캐릭터·그래프와 같은 곳이며 쿠킹에서 빠진다. 정식 샘플 폴더는 [#26](https://github.com/jaykop/Kata/issues/26)에서 정한다. 2026-10-10 `Content/KataSample/Input`으로 옮겼다 |
| KataGraph 발동 방식 | 확정 | 2026-09-27 IN-3에서 확정([34a5549](https://github.com/jaykop/Kata/commit/34a5549)). 아래 "KataGraph 발동 방식" 항목 참고 |

## KataGraph 발동 방식

| 쟁점 | 선택지 | 제안 |
|---|---|---|
| 그래프 지정 | 확정(IN-3 구현) | 캐릭터 기본 그래프 하나. 입력별 시작 액션은 Entry 엣지의 트리거 태그로 구분한다 |
| 기본 그래프 보관 위치 | 확정(2026-09-27 사용자 결정) | 기본 그래프는 입력이 없던 상태에서 첫 트리거가 왔을 때 시작할 플레이어 캐릭터의 콤보 그래프다. 2026-09-26에는 `KataFramework` 캐릭터가 런타임 슬롯을 갖기로 했으나, IN-3에서 `UKataInputHandlerComponent`의 `Graph`에 두었고 2026-09-27 사용자가 이를 유지하기로 했다. 몬스터는 이 슬롯을 쓰지 않고 이후 KataAI([#22](https://github.com/jaykop/Kata/issues/22))의 StateTree Task가 그래프를 지정한다. 슬롯은 당분간 BP 기본값으로 채우고, 이후 캐릭터 정의 데이터([#26](https://github.com/jaykop/Kata/issues/26))가 생성 시 채운다 |
| 그래프 수명 | 확정(2026-09-27 결정) | 입력 시 시작하고 상주시키지 않는다. 콤보가 끝나면 그래프가 종료되는 현재 동작을 유지한다. 실행 중이 아니면 `Graph`를 시작한 뒤 같은 트리거를 보내고, 새로 시작한 그래프가 진입하지 못하면 Cancelled로 멈춰 대기 상태를 남기지 않는다 |
| 시작 대상 | 확정(IN-3 구현) | 그래프는 대상 없이 시작한다. 대상은 각 액션의 Resolve Target Command가 정한다. 처음 제안한 `ResolveActionTarget` 결과를 시작 Context에 넣는 방식은 쓰지 않았다 |
| 액션 중 캔슬(회피 등) | 확정(2026-09-26 사용자 확인) | 캔슬 시점은 각 액션의 Transition Window 태스크가 열고, 엣지의 Required Window Tag와 Immediate Timing으로 전이한다. 여러 액션에서 같은 곳으로 캔슬하는 엣지는 Alias(Any State 포함)로 한 번만 긋는다. 입력 계층은 트리거만 보내며 캔슬을 따로 처리하지 않는다 |
| 캔슬 대상 액션의 진입 | Entry 엣지와 Alias 엣지를 함께 긋기 | 그래프가 비어 있을 때는 Entry, 액션 중에는 Alias가 평가되므로 회피처럼 양쪽에서 시작하는 액션은 두 엣지를 모두 긋는다. 회피 중 연속 회피는 Alias가 자기 자신을 건너뛰므로 회피 노드에 직접 엣지를 긋는다 |
| 그래프 밖 직접 실행 | 허용 / 모든 액션을 그래프로 | 모든 액션을 그래프로. 단일 액션도 Entry→Action 그래프로 만든다 |

## 작업 순서와 완료 조건

단계마다 사용자 빌드·실행 확인을 거친 뒤 다음 단계로 넘어간다. 각 단계 끝에 연결 이슈와 설명서를 해당 범위만 갱신한다.
결정은 그 결정이 필요한 단계를 시작할 때 확정한다. 그래프 발동 방식은 IN-1·IN-2의 선행 조건이 아니다.

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| IN-1 | 높음 | 기본 입력 설정: `KataFramework`에 EnhancedInput 의존 추가, `UKataInputConfig`(기본 IMC 목록, 이동·시점 InputAction), `AKataPlayerCharacter`의 입력 설정 참조·바인딩·빙의 시 기본 IMC 추가와 해제 시 제거·이동·시점 처리, `AKataPlayerController`, 샘플 IA·IMC·InputConfig 에셋과 GameMode 지정 | 없음 | 샘플 맵에서 플레이어 캐릭터가 키 입력으로 이동하고 시점을 돌린다 |
| IN-2 | 높음 | 입력 처리 컴포넌트 분리: IN-1 로직을 `UKataInputHandlerComponent`로 옮기고 컨트롤러 변경 델리게이트로 IMC 추가·제거. IMC 추가·제거는 엔진 Blueprint 노드 사용법을 설명서에 적는다 | IN-1 | 컴포넌트로 옮긴 뒤에도 이동·시점이 동작하고, Blueprint에서 엔진 노드로 IMC를 빼면 해당 행동이 막히고 다시 넣으면 돌아온다 |
| IN-3 | 높음 | 입력 태그와 그래프 연결: 입력 바인딩(InputAction + `ETriggerEvent` → `Input.*`), Input → Trigger 매핑, 기본 그래프 슬롯, 그래프 시작·트리거 전달, 샘플 태그와 콤보 그래프 | IN-2, "KataGraph 발동 방식" 확정 | 키 입력으로 그래프 진입과 콤보 전이, 공격 중 회피 캔슬이 동작한다. [#25](https://github.com/jaykop/Kata/issues/25) Alias의 런타임 확인과 [#8](https://github.com/jaykop/Kata/issues/8)의 버퍼 없는 조작감 확인을 겸한다 |
| IN-4 | 보통 | 락온 입력: 획득·해제, 좌·우 전환 InputAction 연결 | IN-1, [#13](https://github.com/jaykop/Kata/issues/13) | 락온 획득·좌우 전환·해제가 입력으로 동작한다 |
| IN-5 | 보통 | 마무리: #19 완료 조건 점검, 설명서·구현 상태 정리 | IN-3, IN-4 | 사용자 실행 확인 후 #19를 닫고 결정 이유를 devlog로 옮긴다 |

## 영향과 제한

- 모듈 경계: 코어 `Kata`는 변경하지 않는다. 캔슬은 이미 구현된 Alias로 처리한다.
- 의존: `KataFramework`에 EnhancedInput만 추가된다. KataGraph·KataTargeting 의존은 [#17](https://github.com/jaykop/Kata/issues/17)에서 추가됐다.
- SubGraph 평탄화([#25](https://github.com/jaykop/Kata/issues/25) G3)는 입력 계층에 영향이 없다. 샘플 그래프가 커지면 나중에 나눌 수 있다.
- 자원 수명: 캐릭터가 넣은 기본 IMC는 빙의 해제 시 제거한다.
- IMC를 제거했다가 다시 넣을 때 눌린 키가 재발동하지 않도록 `FModifyContextOptions`의 눌린 키 무시 옵션을 확인한다.
