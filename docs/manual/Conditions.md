# Kata 기본 조건

갱신: 2026-09-25  
적용 기준: 현재 KataConditions 소스. 이번 문서 갱신은 빌드·실행 미확인.

`KataConditions` 모듈은 Tag, Attribute, Distance, Angle, Group 조건을 제공한다. 모든 조건은 `UKataCondition`을 상속하며 C++·Blueprint에서 확장할 수 있다. 조건은 값을 읽어 판정하고 게임 상태는 변경하지 않는다.

## 사용과 확장

호스트가 `FKataConditionContext`의 `SelfActor`, `TargetActor`를 채운 뒤 `Evaluate` 또는 `IsSatisfied`를 호출한다. `Evaluate`는 Pass/Fail/Invalid와 진단용 `Reason` 이름을 반환하고, `IsSatisfied`는 최종 Pass만 true로 반환한다.

ASC가 별도 PlayerState 등에 있으면 `SelfAbilitySystem`·`TargetAbilitySystem`을 직접 전달한다. 유효한 명시적 ASC를 우선 사용하고, 없으면 Actor의 Ability System 인터페이스 및 컴포넌트를 조회한다. 자동 조회가 임의의 PlayerState 연결까지 추론하지는 않는다. Context 참조는 Weak Pointer이며, 호출하는 호스트가 실제 Actor·ASC의 수명을 관리한다.

```cpp
UPROPERTY(EditAnywhere, Instanced, Category = "Conditions")
TObjectPtr<UKataCondition> Condition;

// 조건을 소유한 컴포넌트에서 실행자와 현재 대상을 전달한다.
FKataConditionContext Context;
Context.SelfActor = GetOwner();
Context.TargetActor = CurrentTarget;
const bool bAllowed = IsValid(Condition) && Condition->IsSatisfied(Context);
```

예제는 조건을 소유한 컴포넌트 내부 호출 일부이며 GetOwner·CurrentTarget은 호출자가 준비한다.
`KataCondition.h`, `KataConditionTypes.h`를 포함하고 KataConditions 모듈에 의존한다.
에셋·컴포넌트의 Instanced 프로퍼티에서 조건을 편집한다. Kata에서는 StartCondition과 그래프 노드·엣지 조건에 사용할 수 있다.

게임 프로젝트의 C++ 조건은 `EvaluateCondition_Implementation`을 오버라이드한다. BP 자식 클래스에서는 `Evaluate Condition` 함수를 오버라이드하고 `FKataConditionResult`를 만들어 반환한다. `EvaluateCondition`은 **Invert 적용 전** 결과를 반환한다. 외부 호출자는 항상 `Evaluate`/`IsSatisfied`를 사용한다.

`bInvert`는 Pass와 Fail만 반전한다. 필수 데이터 누락, 잘못된 설정, 계산 불가를 나타내는 Invalid는 반전하지 않는다. 조건 객체에 캐릭터별 상태나 마지막 결과를 저장하지 않는다.

## Tag

- Subject: Self / Target.
- Tags: `FGameplayTagContainer`. 빈 목록은 Invalid.
- MatchMode: Any / All.
- Exact Match 꺼짐: 보유한 자식 태그가 검사하는 부모 태그에 매칭된다.
- Exact Match 켜짐: 정확히 일치하는 태그만 인정한다.
- 검사 대상은 선택한 ASC가 현재 보유한 태그다. Actor의 일반 Tags 배열과는 다르다.

Any + Invert는 지정 태그가 하나도 없을 때 통과한다. All + Invert는 지정 태그 중 하나 이상이 없을 때 통과한다.

## Attribute

- Subject: Self / Target.
- Absolute: 선택한 `Attribute` 값을 비교한다.
- Ratio: `Attribute / MaxAttribute` 값을 비교한다. `0.3`은 30%이며 0~1로 강제 제한하지 않는다.
- Comparison: LessThan, LessOrEqual, GreaterThan, GreaterOrEqual, Equal, NotEqual.
- Equal/NotEqual만 `EqualityTolerance`를 사용한다. 나머지는 지정한 비교 연산의 경계 포함 여부를 그대로 따른다.
- GAS의 현재 값을 읽는다. `FGameplayAttributeData`의 BaseValue를 읽는 모드는 제공하지 않는다.
- AttributeSet의 숫자 Attribute를 지원한다. ASC 시스템 내부 필드는 지원 범위에서 제외한다.
- 미선택 Attribute, 설치되지 않은 AttributeSet, 0 이하의 Max, NaN·무한대 값은 Invalid.

예: `Stamina >= 20`, `Health / MaxHealth <= 0.3`.

## Distance

Self와 Target에 각각 `SocketName`으로 기준점을 지정한다.

- `SocketName`이 비어 있으면 Actor 위치를 사용한다.
- `SocketName`이 있으면 Actor가 `ACharacter`여야 하며 `ACharacter::GetMesh()`의 Socket 위치를 사용한다. 다른 컴포넌트는 선택할 수 없다.
  캐릭터가 아니거나 Mesh가 없으면 Invalid(`MissingCharacterMesh`)다.
- Socket이 없으면 Invalid(`MissingSocket`)이며 Actor·Mesh 위치로 대체하지 않는다. 스켈레탈 메시의 본 이름도 엔진의 `DoesSocketExist`/`GetSocketLocation`이 지원하는 방식으로 사용할 수 있다.
- 2D는 두 기준점의 월드 위치에서 XY 거리, 3D는 XYZ 거리를 계산한다.
- 거리 단위는 cm다. `Comparison`과 `CompareDistance`로 판정한다. 기본값은 `Distance <= 200cm`다.
- 비교 연산은 LessThan, LessOrEqual, GreaterThan, GreaterOrEqual, Equal, NotEqual을 지원하며 Attribute와 같은 `EKataNumericComparison`을 사용한다.
- Equal/NotEqual에만 `EqualityTolerance`를 적용한다. 기본값은 1cm이며 대소 비교에는 적용하지 않는다.
- 음수·NaN·무한대 기준 거리, 같음 비교의 잘못된 허용 오차는 Invalid다.
- 기존 MinDistance/MaxDistance 범위 설정은 제거했다. 기존 에셋에 해당 값을 저장한 경우 비교 연산과 기준 거리를 다시 지정해야 하며 자동 변환은 제공하지 않는다.
- 같은 위치의 두 Actor는 거리 0으로 유효하다.

## Angle

SelfActor의 위치·ForwardVector를 기준으로 TargetActor의 위치를 판정한다. Angle에는 별도 Socket 옵션이 없다.

- `HalfAngleDegrees`: 중심에서 한쪽까지의 각도. 45°는 전체 90° 범위다. 0~180°를 지원한다.
- `YawOffsetDegrees`: 기준 방향을 회전한다. 양수 90°는 정면에서 오른쪽, 음수 90°는 왼쪽이다. 판정 원점을 이동시키지 않는다.
- 2D: Forward와 Target 방향을 XY 평면에 투영하고 월드 +Z 축을 기준으로 YawOffset을 적용한다.
- 3D: SelfActor의 로컬 Up 축을 기준으로 YawOffset을 적용하고 3D 원뿔 범위를 평가한다. Pitch/Roll이 있는 Actor도 그 자세를 따른다.
- 경계는 포함한다. 부동소수점 오차를 위한 고정 0.0001° 여유만 사용하며 별도 게임플레이 Threshold는 없다.
- Self와 Target이 같은 위치이거나 2D 투영 후 방향이 0이면 Invalid. Self의 Forward가 수직이어서 2D 방향을 정할 수 없는 경우도 Invalid다.

## Group

Conditions 배열에 자식 조건을 넣고 Mode를 All 또는 Any로 지정한다. All은 모두 Pass, Any는 하나가 Pass면 통과한다.
배열 순서대로 단축 평가하므로 All의 첫 Fail 또는 Any의 첫 Pass 뒤에 있는 조건은 평가하지 않는다.
평가한 자식이 Invalid이면 그 결과를 전파한다. 뒤에 있는 모든 자식의 Invalid를 항상 찾는 방식은 아니다.
자식은 Evaluate로 호출해 각각의 Invert를 반영하고, 그룹 자체의 Invert는 합성 결과에 한 번 적용한다.
빈 배열·null 항목·직접 자기 참조는 설정 오류다. 중첩 그룹을 만들 때 순환 참조를 구성하지 않는다.
에디터 IsDataValid는 소유한 자식도 검사한다.

## 공용 판정 함수

`FunctionLibraries/KataFL_Condition.h`의 `UKataFL_Condition`은 C++와 BP가 공유하는 순수 함수다.
BP Category는 Kata|Condition이다. 함수는 Invert를 처리하거나 공유 조건 객체에 결과를 저장하지 않는다.

| 함수 | 입력과 역할 |
|---|---|
| CompareValue | Value·ReferenceValue·Comparison·EqualityTolerance로 수치를 비교한다 |
| CheckDistance | 두 FVector 위치, Space, ReferenceDistance·비교·허용 오차를 받는다. Actor·Socket 해석은 호출자가 담당한다 |
| CheckAngle | SourceActor·TargetActor, Space, HalfAngleDegrees·YawOffsetDegrees로 범위를 판정한다 |
| CheckTag | ASC·Tags, Any/All·Exact Match로 보유 Gameplay Tag를 판정한다 |
| ValidateComparison | C++ 전용 설정 검사. 비교 방식·허용 오차가 유효하면 NAME_None을 반환한다 |

앞의 네 함수는 bool과 OutError를 반환한다. OutError가 None이면 bool은 정상적인 일치·불일치이고,
None이 아니면 잘못된 입력이다. bool=false만으로 불충족과 오류를 합치지 않는다.
Condition UObject는 이 결과를 Pass·Fail·Invalid로 바꾸고 Context 변환과 Invert를 담당한다.

```cpp
#include "FunctionLibraries/KataFL_Condition.h"

// 호출자가 얻은 두 위치를 2D 거리 200cm 이내인지 판정한다. 실행 미확인 예제다.
FName Error;
const bool bInRange = UKataFL_Condition::CheckDistance(
    SelfLocation, TargetLocation, EKataConditionSpace::Plane2D,
    200.0, EKataNumericComparison::LessOrEqual, 1.0, Error);
const bool bAllowed = Error.IsNone() && bInRange;
```

## 진단과 검증

Reason은 프로젝트가 추가할 수 있는 `FName`이다. 대표 값은 `MissingAbilitySystem`, `MissingAttribute`, `InvalidRatioMaximum`, `MissingTargetActor`, `MissingSocket`, `MissingCharacterMesh`, `UndefinedTargetDirection`이다. 정상적인 불충족은 `ConditionNotMet`, Invert로 뒤집힌 성공은 `InvertedCondition`으로 표시한다.

기본 조건은 `IsDataValid`에서 설정을 검사하며 같은 검사를 런타임 진입점에서도 수행한다. 런타임에서만 알 수 있는 Actor·ASC·Socket의 존재 여부는 Evaluate에서 확인한다. 호스트 에셋이 인라인 조건을 소유하면 자신의 검증 코드에서 각 조건의 `IsDataValid`를 호출해야 한다.

자동화 테스트 그룹은 `Kata.Conditions`다. Editor의 Session Frontend에서 실행하거나 다음과 같이 headless 실행할 수 있다. 먼저 Editor 빌드를 완료해야 한다.

```powershell
$engineRoot = 'C:\Program Files\Epic Games\UE_5.8'
& "$engineRoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "$PWD\ProjectKata.uproject" -unattended -nullrhi -nosplash -nosound -nop4 `
  '-ExecCmds=Automation RunTests Kata.Conditions' `
  '-TestExit=Automation Test Queue Empty' `
  "-ReportExportPath=$PWD\Saved\Automation\KataConditions"
```

테스트는 개발용 자동화 테스트가 활성화된 빌드에서만 포함된다. 테스트 태그 `Kata.Tests.Condition.*`도 같은 빌드에서만 등록된다. 테스트는 엔진의 테스트 AttributeSet과 임시 월드·Actor·ASC를 사용하고, 프로젝트 게임플레이 에셋을 요구하지 않는다.

2026-09-24 사용자는 Distance 변경 후 빌드, Kata.Conditions.Distance.* 두 테스트, 에디터 Socket 판정을 확인했다.
이 기록은 Group·모든 조건·BP 공용 함수 노드의 실행 확인을 뜻하지 않는다. 이번 문서 작업에서는 테스트를 실행하지 않았다.

- [KataFL_Condition.h](../../Plugins/Kata/Source/KataConditions/Public/FunctionLibraries/KataFL_Condition.h): 공용 함수 계약.
- [KataCondition_Group.cpp](../../Plugins/Kata/Source/KataConditions/Private/Conditions/KataCondition_Group.cpp): 단축 평가와 오류 전파.
- [KataCondition_Distance.cpp](../../Plugins/Kata/Source/KataConditions/Private/Conditions/KataCondition_Distance.cpp): 현행 Socket 기준점.
- [에셋 이전 안내](Asset-Migration.md): 제거된 위치 선택·거리 범위 설정 처리.
