# 입력 사용법

갱신: 2026-09-27  
대상: 플레이어 캐릭터에 Enhanced Input을 연결하는 사용자. KataFramework 모듈  
적용 기준: [#19](https://github.com/jaykop/Kata/issues/19) IN-1 기본 입력 설정, IN-2 입력 처리 컴포넌트 분리  
확인 상태: 2026-09-27 사용자 Editor 빌드, `LV_TestMap` PIE에서 WASD 이동과 마우스 시점 확인(컴포넌트 분리 후 재확인). 엔진 노드로 IMC를 빼고 넣는 동작, 폰 교체, 게임패드, Game 타깃은 미확인

## 목적과 준비

폰의 `UKataInputHandlerComponent`에 입력 설정 에셋을 지정해 이동과 시점을 조작한다.
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
5. GameMode의 Default Pawn Class를 4의 Blueprint로, Player Controller Class를 `Kata Player Controller`로 지정한다.
   맵의 World Settings → GameMode Override에 그 GameMode를 지정하고 PIE로 확인한다.

샘플은 `/Game/KataTest/Input`의 `IA_Move`, `IA_Look`, `IMC_Default`, `DA_InputConfig`와
`/Game/KataTest`의 `BP_SamplePC`, `BP_KataTestGameMode`, `/Game/KataTest/Maps/LV_TestMap`이다.
`Content/KataTest`는 쿠킹에서 제외되므로 패키징된 게임에는 들어가지 않는다. 현재 `.gitignore`가 `/Content/`를 제외하므로 이 샘플은 저장소에 포함되지 않는다.

## 주요 설정과 실행 규칙

| UI 항목 또는 API | 의미·입력 | 기본값·빈 값·실패 시 동작 |
|---|---|---|
| `UKataInputConfig` → Default Mapping Contexts | 빙의될 때 추가할 IMC와 우선순위. 우선순위가 클수록 먼저 입력을 받는다 | 비어 있으면 IMC를 추가하지 않는다. 빈 항목은 건너뛴다 |
| `UKataInputConfig` → Move Action | Axis2D. X는 오른쪽, Y는 앞쪽. 컨트롤 회전의 Yaw 기준 방향으로 이동한다 | 비어 있으면 이동을 바인딩하지 않는다 |
| `UKataInputConfig` → Look Action | Axis2D. X는 Yaw, Y는 Pitch에 더한다. 상하 반전은 IMC 모디파이어로 정한다 | 비어 있으면 시점을 바인딩하지 않는다 |
| `UKataInputHandlerComponent` → Input Config | 이 폰의 입력 설정 | 비어 있으면 입력을 바인딩하지 않고 `LogKataFramework` 경고를 남긴다 |
| `UKataInputHandlerComponent::SetupPlayerInput` | 폰의 `SetupPlayerInputComponent`에서 호출한다. 전달받은 입력 컴포넌트에 InputAction을 바인딩한다 | `UEnhancedInputComponent`가 아니면 경고를 남기고 바인딩하지 않는다 |
| `GetInputConfig()`, `AKataPlayerCharacter::GetInputHandlerComponent()` | 입력 설정과 입력 처리 컴포넌트를 돌려준다 | 입력 설정이 없으면 null |

- 여러 캐릭터가 같은 입력 설정 에셋을 공유할 수 있다. 에셋에는 실행 중 상태를 저장하지 않는다.
- Input Config는 빙의 중에 바꾸지 않는다. 추가한 IMC와 제거할 IMC가 어긋난다.
- 바인딩은 빙의할 때마다 엔진이 새로 만드는 `UEnhancedInputComponent`에 속하므로 따로 해제하지 않는다.
  이 입력 컴포넌트는 바인딩을 보관할 뿐이고, 무엇을 바인딩할지는 `UKataInputHandlerComponent`가 정한다.
- 로컬 플레이어 컨트롤러가 아닌 컨트롤러(AI 등)에 빙의되면 IMC를 추가하지 않는다.

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
| 공격 입력, 락온 입력이 없다 | 아직 구현하지 않았다 | [입력 계층 계획](../plan/Input-Plan.md)을 따른다 |
