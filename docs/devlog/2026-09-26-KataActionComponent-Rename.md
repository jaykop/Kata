# UKataComponent를 UKataActionComponent로 이름 변경

작성: 2026-09-26  
갱신: 2026-09-26  
유형: 구현 기록, 결정 기록  
대상: KataRuntime의 액션 실행 컴포넌트와 이를 참조하는 KataGraph·KataEditor·KataFramework·ProjectKataTesting  
기준: `d447205` 이후 작업 트리. 다른 세션의 미커밋 변경 없음

## 배경과 결론

Kata가 플러그인 전체를 가리키는 이름이 되면서 `UKataComponent`만으로는 KataAction을 실행하는 컴포넌트라는 역할이 드러나지 않았다.
[#16](https://github.com/jaykop/Kata/issues/16)에서 이름을 `UKataActionComponent`로 바꿨다. `UKataGraphComponent`, `UKataTargetingComponent`와
이름 짓는 방식도 맞췄다. 동작은 바꾸지 않았다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| 클래스와 파일 | `UKataComponent`, `Runtime/KataComponent.h/.cpp` | `UKataActionComponent`, `Runtime/KataActionComponent.h/.cpp`. 표시 이름 Kata Action Component |
| 캐릭터 API | `AKataCharacter::KataComponent`, `GetKataComponent()` | `ActionComponent`, `GetActionComponent()` |
| 테스트 액터 | `AKataTestActor::KataComponent`, `GetKataComponent()` | `ActionComponent`, `GetActionComponent()` |
| 그래프 내부 이름 | `ResolveKataComponent`, 그래프 인스턴스 멤버 `KataComponent` | `ResolveActionComponent`, `ActionComponent`. Blueprint에 노출되지 않아 Redirect 없음 |
| Redirect | 없음 | 클래스, 캐릭터·테스트 액터 멤버 프로퍼티, 캐릭터 `GetKataComponent` 함수 |
| 서브오브젝트 이름 | `CreateDefaultSubobject`의 `"KataComponent"` | `"KataActionComponent"` |
| 델리게이트 타입 | `FKataComponentStartedSignature`, `FKataComponentEndedSignature` | `FKataActionComponentStartedSignature`, `FKataActionComponentEndedSignature` |

## 주요 결정과 이유

- 서브오브젝트 이름 변경(2026-09-26 사용자 결정): 처음에는 Blueprint가 부모 컴포넌트의 기본값을 바꿔 둔 기록이 서브오브젝트 이름으로
  연결되므로 이름을 유지하기로 했다. 사용자는 에셋이 적은 지금이 호환 부담이 가장 작고, 필요하면 에셋을 새로 만드는 편이 낫다고 판단해
  서브오브젝트 이름도 `KataActionComponent`로 바꾸기로 했다. 그런 기록을 가진 Blueprint는 기본값을 다시 설정하거나 새로 만든다.
- 델리게이트 타입 이름도 `FKataActionComponentStartedSignature`, `FKataActionComponentEndedSignature`로 바꿨다(2026-09-26 사용자 결정).
  처음에는 Blueprint 이벤트 바인딩 호환을 이유로 뺐으나, 서브오브젝트 이름과 같은 판단으로 지금 함께 바꿨다.
  델리게이트 프로퍼티 이름 `OnKataStarted`·`OnKataEnded`는 그대로다.
- `PlayKataAction`, `StopKata`, `OnKataStarted` 같은 컴포넌트 API 이름도 바꾸지 않았다.
- 테스트 액터의 `GetKataComponent`는 `UFUNCTION`이 아니어서 함수 Redirect를 두지 않았다.

## 근거

- [KataActionComponent.h](../../Plugins/Kata/Source/KataRuntime/Public/Runtime/KataActionComponent.h): 새 클래스 선언.
- [DefaultEngine.ini](../../Config/DefaultEngine.ini): `ClassRedirects`, `PropertyRedirects`, `FunctionRedirects`.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 코드 참조 | 저장소 검색, 에이전트 | 옛 이름이 남지 않았다 | — |
| 빌드 | 2026-09-26 사용자 Editor 빌드 | 델리게이트 타입 변경까지 성공 | — |
| 에셋 호환 | 2026-09-26 사용자 확인 | 기존 액션·그래프 에셋이 열린다(델리게이트 타입 변경 전 확인) | 델리게이트 바인딩 Blueprint |

## 남은 제한과 후속 작업

- 부모 컴포넌트 기본값을 바꿔 둔 Blueprint는 다시 설정하거나 새로 만든다.
- 캐릭터 조합은 [#17](https://github.com/jaykop/Kata/issues/17)에서 새 이름을 기준으로 진행한다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [현재 구현 상태](Implementation-Status.md) | 새 이름과 Redirect, 확인 상태 |
| [런타임 사용법](../manual/Runtime-Usage.md) | 새 이름, 예제, 기존 에셋 처리 |
| [에디터 사용법](../manual/Editor-Usage.md) | 새 이름 |
| 계획 문서 | Plugin-Modularization·Targeting·Hit-Trace Plan의 이름 |
| 날짜별 과거 devlog | 당시 기록이므로 옛 이름을 유지 |
