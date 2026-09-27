# 입력 사용법

갱신: 2026-09-27  
대상: 플레이어 캐릭터에 Enhanced Input을 연결하는 사용자. KataFramework 모듈  
적용 기준: [#19](https://github.com/jaykop/Kata/issues/19) IN-1 기본 입력 설정  
확인 상태: 2026-09-27 사용자 Editor 빌드, `LV_TestMap` PIE에서 WASD 이동과 마우스 시점 확인. 게임패드와 Game 타깃은 미확인

## 목적과 준비

`AKataPlayerCharacter`에 입력 설정 에셋을 지정해 이동과 시점을 조작한다.
캐릭터는 플레이어 컨트롤러에 빙의되는 동안 입력 설정의 기본 IMC를 추가하고, 빙의가 풀리면 제거한다.

준비 조건은 다음과 같다.

- 프로젝트 설정의 Default Player Input Class가 `EnhancedPlayerInput`, Default Input Component Class가 `EnhancedInputComponent`여야 한다.
  이 프로젝트의 `Config/DefaultInput.ini`는 이미 그렇게 설정되어 있다.
- 폰은 `AKataPlayerCharacter` 또는 그 파생 Blueprint여야 한다.

## 사용 순서

1. InputAction을 만든다. 이동과 시점은 Value Type을 `Axis2D`로 둔다.
2. Input Mapping Context를 만들고 키를 InputAction에 연결한다.
   - 이동: W는 Swizzle Input Axis Values(YXZ), S는 Swizzle과 Negate, A는 Negate, D는 모디파이어 없음.
   - 시점: Mouse XY 2D-Axis에 Negate를 두고 Y만 켠다.
3. 콘텐츠 브라우저에서 Data Asset → `Kata Input Config`를 만들고 기본 IMC, Move Action, Look Action을 지정한다.
4. `AKataPlayerCharacter` 파생 Blueprint의 Class Defaults에서 Kata → Input → Input Config에 3의 에셋을 지정한다.
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
| `AKataPlayerCharacter` → Input Config | 이 캐릭터의 입력 설정 | 비어 있으면 입력을 바인딩하지 않고 `LogKataFramework` 경고를 남긴다 |
| `GetInputConfig()` | 지정된 입력 설정을 돌려준다 | 없으면 null |

- 여러 캐릭터가 같은 입력 설정 에셋을 공유할 수 있다. 에셋에는 실행 중 상태를 저장하지 않는다.
- Input Config는 빙의 중에 바꾸지 않는다. 추가한 IMC와 제거할 IMC가 어긋난다.
- 바인딩은 빙의할 때마다 새로 만들어지는 PlayerInputComponent에 속하므로 따로 해제하지 않는다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| PIE에서 캐릭터가 생성되지 않는다 | 맵의 GameMode가 샘플 GameMode가 아니다 | World Settings의 GameMode Override를 확인한다 |
| 로그에 `Input Config is not set` | 캐릭터 Blueprint에 입력 설정이 없다 | Class Defaults의 Input Config를 지정한다 |
| 로그에 `not a UEnhancedInputComponent` | 프로젝트 입력 컴포넌트 클래스가 Enhanced Input이 아니다 | 프로젝트 설정 → Input의 Default Classes를 확인한다 |
| 시점 상하가 반대 | IMC의 Negate 설정 | Mouse XY 매핑의 Negate에서 Y만 켠다 |
| 화면이 캐릭터 눈 위치에서 보인다 | 카메라 컴포넌트가 없다. 카메라는 [#20](https://github.com/jaykop/Kata/issues/20) 범위다 | 필요하면 Blueprint에 SpringArm과 Camera를 붙인다 |
| 공격 입력, IMC 추가·제거 API, 락온 입력이 없다 | 아직 구현하지 않았다 | [입력 계층 계획](../plan/Input-Plan.md)을 따른다 |
