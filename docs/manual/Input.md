# 입력 사용법

갱신: 2026-10-10  
대상: 플레이어 캐릭터에 Enhanced Input을 연결하는 사용자. KataFramework 모듈  
적용 기준: [#19](https://github.com/jaykop/Kata/issues/19) IN-1 기본 입력 설정, IN-2 입력 처리 컴포넌트 분리, IN-3 입력 태그와 그래프 연결, IN-4 락온 입력, IN-5 확인 정리. 결정 기록은 [입력 계층 결정 기록](../devlog/2026-10-10-Input-Layer.md). 게임패드는 [#44](https://github.com/jaykop/Kata/issues/44) DS-1·DS-2·DS-3·DS-6  
확인 상태: 2026-09-27 사용자 Editor 빌드, `LV_TestMap` PIE에서 WASD 이동·마우스 시점과 마우스 왼쪽 공격 액션 실행, `TransitionWindow.Combo`(현재 `Window.Transition.Combo`) 창을 통한 공격 1 → 2 콤보 전이, 태그 선택 목록의 필터링 확인. 2026-10-04 사용자 Editor 빌드·PIE에서 Jump Action과 Cancel Bindings를 통한 이동·점프 캔슬 확인. 2026-10-05 락온 카메라 확인에서 락온 입력 경로 사용. 2026-10-10 사용자 PIE에서 엔진 노드로 IMC를 빼고 넣는 동작(누르던 키 재발동 없음), 공격 중 Any State Alias를 통한 회피 캔슬, 락온 방향 회피 확인. 2026-10-10 사용자 Editor 빌드·PIE에서 듀얼센스를 뽑았을 때 입력이 해제되는 것 확인. 폰 교체, 게임패드 매핑별 동작, Game 타깃은 미확인

## 목적과 준비

폰의 `UKataInputHandlerComponent`에 입력 설정 에셋과 콤보 그래프를 지정해 이동·시점을 조작하고 입력으로 그래프를 구동한다.
컴포넌트는 폰이 로컬 플레이어 컨트롤러에 빙의되는 동안 입력 설정의 기본 IMC를 추가하고, 빙의가 풀리면 제거한다.
컨트롤러가 조종 폰을 바꾸면 이전 폰의 IMC가 먼저 빠지고 새 폰의 IMC가 추가된다.

`AKataPlayerCharacter`는 이 컴포넌트를 기본으로 가지고 있다. 다른 폰에 붙일 때는 폰의 `SetupPlayerInputComponent`에서
`SetupPlayerInput`을 호출해야 바인딩이 만들어진다.

준비 조건은 다음과 같다.

- 프로젝트 설정의 Default Player Input Class가 `EnhancedPlayerInput`, Default Input Component Class가 `EnhancedInputComponent`여야 한다.
  이 프로젝트의 `Config/DefaultInput.ini`는 이미 그렇게 설정되어 있다.
- 폰은 `AKataPlayerCharacter` 또는 그 파생 Blueprint를 쓴다. C++ 폰에 직접 붙이는 경우는 위 호출이 필요하다.

## 사용 순서

1. InputAction을 만든다. 이동과 시점은 Value Type을 `Axis2D`로 둔다.
2. Input Mapping Context를 만들고 키를 InputAction에 연결한다.
   - 이동: W는 Swizzle Input Axis Values(YXZ), S는 Swizzle과 Negate, A는 Negate, D는 모디파이어 없음.
   - 시점: Mouse XY 2D-Axis에 Negate를 두고 Y만 켠다.
3. 콘텐츠 브라우저에서 Data Asset → `Kata Input Config`를 만들고 기본 IMC, Move Action, Look Action을 지정한다.
4. `AKataPlayerCharacter` 파생 Blueprint의 Components에서 `KataInputHandlerComponent`를 선택하고 Kata → Input → Input Config에 3의 에셋을 지정한다.
5. 그래프를 구동하려면 아래 "입력으로 그래프 구동"을 따라 태그와 입력 설정, 컴포넌트의 Graph를 채운다.
6. 캐릭터 데이터 테이블을 쓰지 않으면 GameMode의 Default Pawn Class를 4의 Blueprint로, Player Controller Class를 `Kata Player Controller`로 지정한다.
   데이터 테이블을 쓰면 [캐릭터 데이터 사용법](Character-Data.md)에 따라 PC 행의 Character Class·Input Config·Graph와 `AKataGameMode`의 Player Character Row를 지정한다.
   맵의 World Settings → GameMode Override에 선택한 GameMode를 지정하고 PIE로 확인한다.

샘플은 `/Game/KataSample/Input`의 `IA_Move`, `IA_Look`, `IA_AttackLight`, `IA_Dodge`, `IMC_Default`, `DA_InputConfig`와
`/Game/KataSample`의 `BP_SamplePC`, `BP_KataTestGameMode`, `KG_KataGraph_Test`, `/Game/KataSample/Maps/LV_TestMap`이다.
`Content/KataSample`는 쿠킹에서 제외되므로 패키징된 게임에는 들어가지 않는다. 현재 `.gitignore`가 `/Content/`를 제외하므로 이 샘플은 저장소에 포함되지 않는다.
PC 행의 Input Config와 Graph를 지정하면 생성 중 입력 처리 컴포넌트에 적용한다. 행에서 비워 둔 항목은 캐릭터 Blueprint의 컴포넌트 기본값을 유지한다. 빙의 중 `SetInputConfig` 또는 `SetGraph`를 호출하면 경고를 남기고 변경을 무시한다.

## 주요 설정과 실행 규칙

| UI 항목 또는 API | 의미·입력 | 기본값·빈 값·실패 시 동작 |
|---|---|---|
| `UKataInputConfig` → Default Mapping Contexts | 빙의될 때 추가할 IMC와 우선순위. 우선순위가 클수록 먼저 입력을 받는다 | 비어 있으면 IMC를 추가하지 않는다. 빈 항목은 건너뛴다 |
| `UKataInputConfig` → Move Action | Axis2D. X는 오른쪽, Y는 앞쪽. 컨트롤 회전의 Yaw 기준 방향으로 이동한다 | 비어 있으면 이동을 바인딩하지 않는다 |
| `UKataInputConfig` → Jump Action | 누르면 `ACharacter::Jump`, 떼면 `StopJumping` | 비어 있으면 바인딩하지 않는다. 폰이 ACharacter가 아니면 무시한다. Kata 액션 실행 중에는 점프하지 않는다 |
| `UKataInputConfig` → Look Action | Axis2D. X는 Yaw, Y는 Pitch에 더한다. 상하 반전은 IMC 모디파이어로 정한다 | 비어 있으면 시점을 바인딩하지 않는다 |
| `UKataInputHandlerComponent` → Input Config | 이 폰의 입력 설정 | 비어 있으면 입력을 바인딩하지 않고 `LogKataFramework` 경고를 남긴다 |
| `UKataInputHandlerComponent::SetupPlayerInput` | 폰의 `SetupPlayerInputComponent`에서 호출한다. 전달받은 입력 컴포넌트에 InputAction을 바인딩한다 | `UEnhancedInputComponent`가 아니면 경고를 남기고 바인딩하지 않는다 |
| `UKataInputConfig` → Input Bindings | InputAction과 Trigger Event(기본 Started)가 발생하면 낼 `Input.*` 태그 | InputAction이나 태그가 빈 항목은 바인딩하지 않는다 |
| `UKataInputConfig` → Cancel Bindings | InputAction과 `Window.Cancel.*` 태그. 누를 때는 새로 누른 요청, 누르고 있는 동안에는 홀드 요청으로 `TryCancelKata`를 호출한다 | InputAction이나 태그가 빈 항목은 바인딩하지 않는다. 다른 바인딩보다 먼저 처리한다 |
| `UKataInputConfig` → Trigger Mappings | `Input.*` 태그를 그래프에 보낼 `Trigger.*` 태그로 바꾸는 규칙 | 목록에 없는 Input 태그는 그래프로 보내지 않는다. 같은 Input 태그가 여러 번 있으면 앞의 항목을 쓴다 |
| `UKataInputHandlerComponent` → Graph | 입력 트리거로 구동할 콤보 그래프 | 비어 있으면 입력을 그래프로 보내지 않는다 |
| `SendGraphTrigger(TriggerTag)` | Trigger 태그를 폰의 그래프로 보낸다. Blueprint에서도 호출할 수 있다 | Graph나 폰의 `UKataGraphComponent`가 없으면 false. 트리거가 전이를 일으키거나 예약하면 true |
| `GetInputConfig()`, `GetGraph()`, `AKataPlayerCharacter::GetInputHandlerComponent()` | 입력 설정, 그래프, 입력 처리 컴포넌트를 돌려준다 | 지정하지 않았으면 null |

- 여러 캐릭터가 같은 입력 설정 에셋을 공유할 수 있다. 에셋에는 실행 중 상태를 저장하지 않는다.
- Input Config는 빙의 중에 바꾸지 않는다. 추가한 IMC와 제거할 IMC가 어긋난다.
- 바인딩은 빙의할 때마다 엔진이 새로 만드는 `UEnhancedInputComponent`에 속하므로 따로 해제하지 않는다.
  이 입력 컴포넌트는 바인딩을 보관할 뿐이고, 무엇을 바인딩할지는 `UKataInputHandlerComponent`가 정한다.
- 로컬 플레이어 컨트롤러가 아닌 컨트롤러(AI 등)에 빙의되면 IMC를 추가하지 않는다.

## 입력으로 그래프 구동

1. 프로젝트 `Config/Tags`의 `Input.ini`에 `Input.*` 태그를, `Trigger.ini`에 `Trigger.*` 태그를 추가한다.
   Input 태그는 플레이어가 무엇을 눌렀는지, Trigger 태그는 그래프가 받는 전이 사건이다.
2. InputAction을 만들어 IMC에 키를 연결한다. 누름 입력은 Value Type을 `Digital (bool)`로 둔다.
3. 입력 설정의 Input Bindings에 InputAction, Trigger Event, Input 태그를 넣고 Trigger Mappings에 Input 태그와 Trigger 태그를 짝지어 넣는다.
4. 그래프 엣지의 Trigger Event Tag에 Trigger 태그를 지정한다. 입력으로 시작할 액션은 Entry에서 나가는 엣지에 둔다.
5. 입력 처리 컴포넌트의 Graph에 그래프를 지정한다. 폰에는 `UKataGraphComponent`와 `UKataActionComponent`가 있어야 한다. `AKataCharacter`는 둘 다 가지고 있다.

입력이 들어오면 컴포넌트는 다음 순서로 처리한다.

- 그래프가 실행 중이 아니면 Graph를 대상 없이 새로 시작한다. 콤보가 끝나면 그래프도 끝나므로 다음 입력 때마다 새로 시작된다.
  대상은 액션의 `Resolve Target` Command가 정한다([타게팅 사용법](Targeting.md)).
- Trigger 태그를 `SendTrigger`로 보낸다. 방금 시작한 그래프가 그 트리거로 진입하지 못하면 `Cancelled`로 멈춘다.
- 실행 중인 그래프라면 현재 액션의 엣지와 Alias 엣지로 전이를 판단한다. 판정 규칙은 [런타임 사용법](Runtime-Usage.md#콤보-그래프-실행)을 따른다.

태그 선택기의 필터는 에디터에 표시할 목록만 제한한다. 그래프 엣지와 `SendTrigger`·`SendGraphTrigger` 핀은 `Trigger.*`만, 입력 설정의 Input 태그 항목은 `Input.*`만 보인다.
실행 중 비교에는 영향이 없으므로 코드에서 다른 루트의 태그를 넘기면 그대로 비교한다.

## 입력으로 액션 캔슬

그래프 전이 없이 액션을 끊고 이동·점프 같은 캐릭터 기본 동작으로 돌아갈 때 쓴다. 다른 Kata 액션(회피 등)으로 넘어가는 캔슬은 그래프 엣지와 `Window.Transition.*` 창을 쓴다.
샘플 회피는 공격 액션이 여는 `Window.Transition.Dodge` 창과 Any State Alias 엣지로 캔슬한다. 구성은 [타게팅 사용법](Targeting.md#입력-방향별-회피)을 따른다.

1. 프로젝트 `Config/Tags/Window.ini`에 `Window.Cancel.*` 태그를 둔다. 샘플은 `Window.Cancel.Move`, `Window.Cancel.Jump`다.
2. 입력 설정의 Cancel Bindings에 InputAction과 캔슬 태그를 짝지어 넣는다. 예: `IA_Move` → `Window.Cancel.Move`, `IA_Jump` → `Window.Cancel.Jump`.
3. 액션 타임라인에 Cancel Window 태스크를 추가하고 끊을 수 있는 구간에 놓는다. Windows에 태그를 넣고, 이동처럼 누르고 있던 입력으로도 끊으려면 Cancel While Held를 켠다.
4. 점프를 쓰려면 입력 설정의 Jump Action을 지정한다.

- 캔슬은 같은 입력의 다른 처리보다 먼저 일어난다. 점프 입력이 액션을 끊으면 같은 프레임에 점프가 나간다. 창이 닫혀 있으면 액션이 계속되고 점프도 나가지 않는다.
- 이동 입력은 액션 실행 중에도 원래대로 적용된다. 루트 모션 몽타주가 재생 중이면 몽타주가 이동을 덮는다.
- 창이 열리기 전에 누른 입력을 저장하지 않는다. Cancel While Held를 끈 창은 열려 있는 동안 새로 누른 입력만 받는다.

## 락온 입력

1. 서로 다른 Digital (bool) InputAction 세 개를 만들고 IMC에 키를 연결한다.
2. Input Config의 Input → Lock On에 Toggle Lock Action, Switch Lock Left Action, Switch Lock Right Action을 지정한다.
3. 폰에 `UKataPlayerTargetingComponent`가 있어야 한다. Lock On Preset, Switch Left Preset, Switch Right Preset과 대상의 타겟 지점을 [타게팅 사용법](Targeting.md)에 따라 설정한다.

| 입력 설정 | 누를 때의 동작 | 실패 시 동작 |
|---|---|---|
| Toggle Lock Action | 락온 중이면 `ReleaseLock`, 아니면 `AcquireLock` | 획득 후보가 없으면 락온하지 않는다 |
| Switch Lock Left Action | `SwitchLockLeft`로 카메라 기준 왼쪽 지점으로 전환 시도 | 락온이 없거나 후보가 없으면 상태를 유지한다 |
| Switch Lock Right Action | `SwitchLockRight`로 카메라 기준 오른쪽 지점으로 전환 시도 | 락온이 없거나 후보가 없으면 상태를 유지한다 |

세 입력은 `Started`에 바인딩되므로 입력 시작 시 한 번 처리한다. Input Bindings·Trigger Mappings에 등록할 필요는 없다.
필드가 비어 있으면 해당 입력은 바인딩하지 않으며, 폰에 PC용 타게팅 컴포넌트가 없으면 무시한다.
같은 액터의 다른 부위도 전환 후보이며, 좌우 판정과 우선순위는 방향별 Preset이 정한다.
락온 중 수동 시점 입력은 무시하고 이동·전투 입력은 유지한다.
카메라 추적·구도 설정은 [카메라 사용법](Camera.md#락온-카메라), 마커는 [HUD 사용법](HUD.md)을 따른다.

## 게임패드와 듀얼센스

게임패드 입력은 엔진 표준 게임패드 키(`Gamepad_*`)로 들어오므로 IMC에 키를 매핑하면 된다. 입력 처리 컴포넌트와 입력 설정은 키보드와 같다.
Xbox 패드는 엔진의 XInput 경로로, 듀얼센스(DualSense·DualSense Edge·DualShock 4)는 샘플 프로젝트에서 활성화한
`WindowsDualsense_ds5w` 플러그인으로 들어온다. 이 플러그인은 Win64 전용이며 Kata 플러그인은 이 플러그인을 참조하지 않는다.
도입 경위와 원본 대비 수정 사항은 [듀얼센스 플러그인 결정 기록](../devlog/2026-10-10-DualSense-Plugin.md)과
[UPSTREAM.md](../../Plugins/WindowsDualsense_ds5w/UPSTREAM.md)를 따른다.

샘플 `IMC_Default`의 게임패드 매핑은 다음과 같다. 키보드·마우스 매핑과 함께 들어 있다.

| InputAction | 게임패드 키 | 듀얼센스 버튼 | 모디파이어 |
|---|---|---|---|
| `IA_Move` | `Gamepad_Left2D` | 왼쪽 스틱 | 없음 |
| `IA_Look` | `Gamepad_Right2D` | 오른쪽 스틱 | Scale By Delta Time, Scalar(64, 48, 1) |
| `IA_AttackLight` | `Gamepad_RightShoulder` | R1 | 없음 |
| `IA_Dodge` | `Gamepad_FaceButton_Right` | ○ | 없음 |
| `IA_ToggleLock` | `Gamepad_RightThumbstick` | R3 | 없음 |
| `IA_SwitchLockLeft`, `IA_SwitchLockRight` | `Gamepad_RightStick_Left`, `Gamepad_RightStick_Right` | 오른쪽 스틱을 좌·우로 튕기기 | 없음 |

- 스틱 시점에는 Scale By Delta Time을 둔다. 마우스는 직전 프레임 이후 움직인 양이 들어오지만, 스틱은 기울인 정도가 매 프레임 그대로 들어온다. 보정하지 않으면 프레임레이트가 높을수록 빨리 회전한다.
  회전 속도는 Scalar로 조절한다. 이 프로젝트는 `bEnableLegacyInputScales`가 켜져 있어 Scalar(64, 48)이 초당 좌우 약 160도, 상하 약 120도다.
- 스틱 상하는 반전하지 않는다. 마우스처럼 반전하려면 Negate를 추가하고 Y만 켠다.
- 락온 중에는 수동 시점 입력을 무시하므로 오른쪽 스틱을 튕겨 대상을 바꿀 수 있다. 락온하지 않은 상태에서도 스틱을 좌우로 절반 넘게 기울이면 전환 입력이 들어가지만, 락온이 없으면 전환은 상태를 바꾸지 않는다.
- 듀얼센스를 뽑으면 플러그인이 스틱·트리거를 0으로, 눌린 버튼을 해제 상태로 보낸다. 다시 꽂으면 1초 안에 같은 장치로 다시 연결된다.

## IMC로 행동 제어

행동을 막거나 허용할 때는 IMC를 추가·제거한다. Kata 전용 함수는 없으며 엔진 노드를 쓴다.

1. `Get Enhanced Input Local Player Subsystem`에서 서브시스템을 얻는다.
2. `Remove Mapping Context`로 IMC를 빼면 그 IMC에 매핑된 행동이 멈춘다. `Add Mapping Context`로 다시 넣으면 돌아온다.
3. 같은 키를 쓰는 IMC를 더 높은 우선순위로 추가해 아래 IMC의 입력을 가릴 수도 있다(InputAction의 Consume Input).

같은 IMC를 여러 곳에서 추가·제거하는 경우의 참조 횟수는 관리하지 않는다. 호출하는 쪽이 짝을 맞춘다.
어떤 폰을 조종하든 유지해야 하는 IMC는 폰의 기본 IMC가 아니라 컨트롤러 쪽에서 관리한다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| PIE에서 캐릭터가 생성되지 않는다 | 맵의 GameMode가 샘플 GameMode가 아니다 | World Settings의 GameMode Override를 확인한다 |
| 로그에 `Input Config is not set` | 입력 처리 컴포넌트에 입력 설정이 없다. 입력 설정이 캐릭터에서 컴포넌트로 옮겨지면서 IN-1 때 지정한 값은 사라졌다 | `KataInputHandlerComponent`의 Input Config를 다시 지정한다 |
| 로그에 `not a UEnhancedInputComponent` | 프로젝트 입력 컴포넌트 클래스가 Enhanced Input이 아니다 | 프로젝트 설정 → Input의 Default Classes를 확인한다 |
| 시점 상하가 반대 | IMC의 Negate 설정 | Mouse XY 매핑의 Negate에서 Y만 켠다 |
| 화면이 캐릭터 눈 위치에서 보인다 | 카메라 컴포넌트가 없다. 카메라는 [#20](https://github.com/jaykop/Kata/issues/20) 범위다 | 필요하면 Blueprint에 SpringArm과 Camera를 붙인다 |
| 공격 키를 눌러도 액션이 실행되지 않는다 | Trigger Mappings가 비었거나 Input 태그가 맞지 않는다. Trigger Mappings를 `TMap`에서 목록으로 바꾼 뒤 이전 값은 사라졌다. 또는 그래프 Entry 엣지에 그 Trigger 태그가 없거나 Graph가 비었다 | 입력 설정의 두 목록, 엣지의 Trigger Event Tag, 컴포넌트의 Graph를 확인한다 |
| 입력이 처리되지 않는 구간이 있다 | 트리거는 도착한 순간 한 번만 평가한다. 창 밖 입력은 버린다 | 입력 버퍼는 [#8](https://github.com/jaykop/Kata/issues/8)에서 다룬다 |
| 듀얼센스 입력이 두 번 들어오거나 엉뚱한 버튼이 눌린다 | Steam Input이나 DS4Windows가 듀얼센스를 XInput 패드로 바꿔 엔진에 따로 보낸다 | 에디터·게임 실행 중에는 Steam Input과 DS4Windows를 끈다 |
| 듀얼센스를 연결해도 반응하지 않는다 | `WindowsDualsense_ds5w` 플러그인이 꺼져 있거나 Win64가 아니다. IMC에 게임패드 키가 없을 수도 있다 | Edit → Plugins에서 플러그인을 확인하고, IMC에 위 게임패드 매핑이 있는지 확인한다 |
| 락온 입력이 동작하지 않는다 | Input Config의 락온 Action·IMC 매핑이 비었거나 PC용 타게팅 컴포넌트가 없다. 획득 Preset에 지점 후보가 없을 수도 있다 | 위 락온 입력 설정과 [타게팅 사용법](Targeting.md)을 확인한다 |

## 확인 상태와 근거

2026-09-27·10-04·10-10 사용자 PIE 확인 범위는 문서 머리의 확인 상태를 따른다. 폰 교체 때의 IMC 교체, 게임패드 매핑별 동작, Game 타깃은 확인하지 않았다.

- [KataInputHandlerComponent.h](../../Plugins/KataFramework/Source/KataFramework/Public/Input/KataInputHandlerComponent.h): 바인딩, 기본 IMC 추가·제거, 그래프 구동, 락온 입력.
- [KataInputConfig.h](../../Plugins/KataFramework/Source/KataFramework/Public/Input/KataInputConfig.h): 입력 설정 에셋.
- [입력 계층 결정 기록](../devlog/2026-10-10-Input-Layer.md): 계층 위치, Input·Trigger 태그 분리, IMC 행동 제어, 그래프 발동 방식.
- [작업 상태](https://github.com/jaykop/Kata/issues/19), 게임패드는 [#44](https://github.com/jaykop/Kata/issues/44).
