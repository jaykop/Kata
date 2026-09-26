# KataFramework 캐릭터 조합

작성: 2026-09-26  
갱신: 2026-09-26  
유형: 구현 기록  
대상: KataFramework `AKataCharacter`, `AKataPlayerCharacter`  
기준: #17 구현 작업 트리. 같은 시점의 다른 작업(Hit Trace, 입력 계획)의 미커밋 변경은 포함하지 않는다.

## 배경과 결론

`AKataCharacter`는 ASC와 `UKataActionComponent`만 가지고 있어서, 그래프 실행과 타게팅·팩션 판정을 쓰려면 파생 BP마다 컴포넌트를
직접 붙여야 했다. 팩션 판정은 액터나 컨트롤러의 팀 인터페이스를 거치는데, 이 인터페이스를 구현한 캐릭터도 없었다([#13](https://github.com/jaykop/Kata/issues/13) TG-5).
[#17](https://github.com/jaykop/Kata/issues/17)에서 공용 캐릭터가 코어와 위성 플러그인의 컴포넌트를 C++ 멤버로 갖추게 하고,
PC용 파생 캐릭터를 추가했다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| `AKataCharacter` 컴포넌트 | ASC, `UKataActionComponent` | `UKataGraphComponent`, `UKataTargetingComponent`, `UKataHitBoxComponent`를 추가하고 Getter를 제공한다 |
| 팀 인터페이스 | 없음 | `IGenericTeamAgentInterface::GetGenericTeamId`가 타게팅 컴포넌트의 `GetFactionTeamId()`를 돌려준다 |
| 생성자 | 기본 생성자 | `FObjectInitializer`를 받는다. 타게팅 컴포넌트 이름을 `TargetingComponentName`으로 공개한다 |
| `AKataPlayerCharacter` | 없음 | 타게팅 컴포넌트를 `UKataPlayerTargetingComponent`로 만들고 `GetPlayerTargetingComponent()`를 제공한다 |
| 플러그인 의존 | Kata, GameplayAbilities, TargetingSystem | KataTargeting 플러그인, `AIModule`·`KataGraph`·`KataTargeting` 모듈을 추가했다 |

## 주요 결정과 이유

- **타게팅 컴포넌트는 기반 타입 멤버 하나로 둔다.** 역할별 캐릭터는 `SetDefaultSubobjectClass`로 같은 자리에 파생 타입을 만든다.
  팀 인터페이스, 이후의 대상 결정 Command, AI 캐릭터가 기반 타입만 알면 된다. 역할별 멤버를 따로 두면 컴포넌트가 두 개가 된다. #17의 확정 결정이다.
- **팩션 값은 컴포넌트에만 둔다.** 캐릭터는 팀 번호를 전달만 하고 `SetGenericTeamId`를 재정의하지 않는다.
- **HitBox 컴포넌트를 기본으로 포함한다(2026-09-26 사용자 결정).** 캐릭터당 하나이고 설정 없이 캐릭터 Mesh를 쓴다.
  판정 첫 프레임을 위해 액터 수명 내내 포즈를 기록해야 하므로 캐릭터가 늘 가지는 편이 맞다.
  HurtBox는 부위별로 여러 개 붙이는 컴포넌트라 기본 구성에 넣지 않았다.
- **기존 `BP_SampleCharacter`의 부모를 바꾸지 않았다.** 사용자가 `AKataPlayerCharacter`를 부모로 하는 PC용 샘플 BP를 새로 만들었다.
- **`AKataPlayerController`는 [#19](https://github.com/jaykop/Kata/issues/19) 입력 계층에서 만든다.**
  컨트롤러의 역할은 입력 연결이고, 팀 판정은 캐릭터의 인터페이스로 충분하다. AI용 컨트롤러의 팀 인터페이스는 KataAI([#22](https://github.com/jaykop/Kata/issues/22))에서 만든다.
- 새 멤버 추가와 생성자 시그니처 변경뿐이므로 Redirect는 추가하지 않았다. 기존 서브오브젝트 이름은 유지한다.

## 근거

- [KataCharacter.h](../../Plugins/KataFramework/Source/KataFramework/Public/Character/KataCharacter.h): 멤버, Getter, 팀 인터페이스, `TargetingComponentName`.
- [KataPlayerCharacter.h](../../Plugins/KataFramework/Source/KataFramework/Public/Character/KataPlayerCharacter.h): PC용 타게팅 컴포넌트 지정.
- [KataFramework.Build.cs](../../Plugins/KataFramework/Source/KataFramework/KataFramework.Build.cs): 모듈 의존.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 빌드 | 사용자 Editor 빌드 보고(2026-09-26) | 성공 | Game 타깃 |
| 기존 에셋 | 사용자 에디터 확인 | 문제없이 열림. `BP_SampleCharacter`에 BP로 추가했던 HitBox 컴포넌트는 사용자가 제거 | - |
| PC 캐릭터 | 사용자가 `AKataPlayerCharacter` 파생 BP 생성 | 타게팅 컴포넌트 Details에 PC 항목이 표시됨 | 락온·소프트 타겟 런타임 |
| 팀 인터페이스 | 소스 작성 | - | 실제 액터 사이의 팩션 판정 |

에이전트는 빌드·테스트를 실행하지 않았다.

## 남은 제한과 후속 작업

- 입력과 `AKataPlayerController`: [#19](https://github.com/jaykop/Kata/issues/19).
- 대상 결정 Command: [#13](https://github.com/jaykop/Kata/issues/13) TG-4.
- AI 캐릭터와 AIController 팀 인터페이스: [#22](https://github.com/jaykop/Kata/issues/22).

## 연관 문서 반영

| 문서 | 반영 내용 |
|---|---|
| [현재 구현 상태](Implementation-Status.md) | KataFramework 범위·의존, 팀 인터페이스, 확인 범위 |
| [런타임 사용법](../manual/Runtime-Usage.md) | 캐릭터 구성과 역할별 타입 교체 |
| [에디터 사용법](../manual/Editor-Usage.md) | 프리뷰 캐릭터 준비 |
| [타게팅 사용법](../manual/Targeting.md)·[팩션 사용법](../manual/Factions.md) | 캐릭터 팀 인터페이스와 확인 상태 |
| [타게팅 시스템 설계](../plan/Targeting-Plan.md) | TG-5를 #17·#22로 이관 |
