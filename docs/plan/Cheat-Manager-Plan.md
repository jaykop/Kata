# 치트 매니저 계획

작성: 2026-10-11  
갱신: 2026-10-11  
연결 이슈: [#54 치트 매니저 (플러그인별 CheatManagerExtension)](https://github.com/jaykop/Kata/issues/54)  
현재 상태 근거: 이 문서 작성 시점의 소스(`KataAttributeSet_Base`, `KataHitHandler`, `KataDeathComponent`, `KataGasBridge`, `KataAIController`, `KataPlayerTargetingComponent`)  
대체 관계: 없음

## 목적과 현재 상태

전투·AI를 확인할 때 PC를 죽지 않게 하거나 바로 죽이고, 적을 즉시 사망·그로기 상태로 만들고, AI를 멈추는 일을 콘솔 한 줄로 하고 싶다.
지금은 이런 경로가 없다. 엔진 `UCheatManager`의 `God`은 `bCanBeDamaged`만 바꾸므로 GE로 들어오는 Kata 대미지를 막지 못하고, `DestroyTarget`은 사망 흐름(#41)을 거치지 않는다.

치트 로직은 각 기능을 구현한 플러그인의 `UCheatManagerExtension`에 Exec 함수로 둔다. 콘솔에서 바로 쓸 수 있고, 인게임 디버그 메뉴([계획](In-Game-Debug-Menu-Plan.md))의 버튼도 같은 함수를 부른다.

## 범위

- 포함
  - PC: 사망, 무적(피격 판정 무시), 대미지 무시(피격 반응은 재생), 무한 스태미나, 부활.
  - 대상: 사망, 즉시 그로기.
  - AI: PC를 Perception 대상에서 제외, AI 정지·재개.
  - 공통: 쿨다운 무시.
  - 대상 선택자: `Self`, `Target`(PC 타게팅 컴포넌트의 현재 대상), `Nearest`, `All`.
- 제외
  - 인게임 UI. [인게임 디버그 메뉴 계획](In-Game-Debug-Menu-Plan.md)에서 다룬다.
  - 무한 마나, 대미지·Poise 배율, 적 무적, 장비 교체, 태그·GE 부여. 첫 범위가 확인된 뒤 추가한다.
  - 비용 무시. 비용 정책이 [#9](https://github.com/jaykop/Kata/issues/9)에서 보류 중이다.

## 배치와 연결 지점

| 치트 | 둘 곳 | 연결 지점 |
|---|---|---|
| 사망 | KataFramework | `UKataDeathComponent::Kill` |
| 무적 | KataFramework | 히트 처리기 실행 전 단계에서 대상이 무적 태그를 가지면 건너뛴다. 처리기(GE 적용·이벤트 전송)가 하나도 실행되지 않는다 |
| 대미지 무시 | KataFramework | `UKataAttributeSet_Base::PostGameplayEffectExecute`에서 Damage 메타 값을 Health에 반영하지 않는다. GE는 실행되므로 히트 반응 GE 컴포넌트가 동작한다 |
| 무한 스태미나 | KataFramework | Stamina 값 변경 델리게이트에서 최대치로 되돌린다. 비용 지불 흐름은 그대로 지나간다 |
| 부활 | KataFramework | 결정 필요(아래 표) |
| 즉시 그로기 | KataFramework | Groggy 값을 최대로 올리는 경로. 기존 Stance 판정을 거치게 한다 |
| Perception 제외 | KataAI | `UAIPerceptionSystem`에서 PC 자극원을 해제하고, 끄면 다시 등록한다 |
| AI 정지·재개 | KataAI | `AKataAIController`의 StateTree 컴포넌트를 `PauseLogic`·`ResumeLogic`으로 멈추고 이동을 멈춘다 |
| 쿨다운 무시 | KataRuntime | `KataGas::IsOnCooldown`이 보는 비 Shipping CVar. 엔진 `AbilitySystem.IgnoreCooldowns`는 Kata 자체 쿨다운 판정을 거치지 않으므로 쓸 수 없다 |
| 대상 선택자 | KataTargeting | 각 확장이 공유하는 해석 함수. `Target`은 `UKataTargetingComponent::GetCurrentTarget`을 쓴다. KataFramework와 KataAI가 모두 KataTargeting에 의존하므로 여기에 둔다 |

- 확장은 `UCheatManager::RegisterForOnCheatManagerCreated`로 각 모듈 시작 시 등록한다. 별도 CheatManager 클래스 지정은 필요 없다.
- 코드는 `UE_WITH_CHEAT_MANAGER`로 감싸 Shipping에서 빠지게 한다. 무적·대미지 무시 판정처럼 런타임 경로에 들어가는 검사도 같은 조건으로 감싼다.
- 토글 상태는 대상 액터의 ASC에 붙인 Loose Gameplay Tag로 둔다. 상태를 GAS Inspector로 볼 수 있고, 공유 에셋에 실행 상태를 두지 않는다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| 로직 위치 | 확정 | 기능별 플러그인의 `UCheatManagerExtension`. 인게임 UI는 샘플 DeveloperTool 모듈(2026-10-11 사용자 결정) |
| 이슈 분할 | 확정 | 치트와 인게임 UI를 두 이슈로 나눈다(2026-10-11 사용자 결정) |
| 무적·대미지 무시 태그 | 결정 필요 | Kata Combat 설정에 `Invincible Tag`·`Ignore Damage Tag`를 추가하고 샘플이 `Status` 아래 태그를 정의하는 안. 플러그인은 태그를 정의하지 않는 규칙을 따른다 |
| 무적의 일반 상태화 | 결정 필요 | 회피 무적 프레임처럼 게임플레이에서도 쓸 일반 무적 상태로 둘지, 치트 전용으로 둘지. 일반 상태로 두면 `UE_WITH_CHEAT_MANAGER` 조건을 빼야 한다 |
| 쿨다운 무시 범위 | 결정 필요 | 전역 CVar(적 포함 전원) / PC만 |
| 부활 방식 | 결정 필요 | 사망 컴포넌트에 부활 API 추가(사망 태그·Hurt Box·이동·AI 복원 필요, 범위 큼) / GameMode로 PC 재생성(간단, 위치·상태 초기화) |
| 콘솔 명령 이름 | 제안 | `KataKill <Selector>`, `KataGod`, `KataNoDamage`, `KataInfiniteStamina`, `KataRevive`, `KataGroggy <Selector>`, `KataHideFromAI`, `KataPauseAI <Selector>`, CVar `Kata.IgnoreCooldowns` |
| 토글 인자 | 제안 | 인자가 없으면 토글, `0`/`1`이면 지정 |

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| A1 | 높음 | KataTargeting 대상 선택자, KataFramework 확장의 사망·즉시 그로기 | 없음 | 콘솔로 `Self`·`Target`·`Nearest`·`All` 대상이 사망 흐름과 그로기 반응을 탄다 |
| A2 | 높음 | 무적·대미지 무시 판정 | 태그 결정 | 무적이면 피격 반응·대미지가 모두 없고, 대미지 무시면 반응만 재생된다 |
| A3 | 보통 | 무한 스태미나 | 없음 | 스태미나를 쓰는 액션 후에도 값이 최대로 남는다 |
| A4 | 보통 | KataAI 확장: Perception 제외, AI 정지·재개 | 없음 | 제외 중 적이 PC를 새로 인지하지 않고, 정지한 AI가 행동하지 않는다 |
| A5 | 보통 | KataRuntime 쿨다운 무시 CVar | 범위 결정 | 켠 동안 쿨다운 중인 액션이 바로 시작된다 |
| A6 | 낮음 | 부활 | 방식 결정 | 사망한 PC가 조작 가능한 상태로 돌아온다 |

## 영향과 제한

- 플러그인 경계: 각 확장은 자기 플러그인 기능만 다룬다. KataAI는 KataFramework에 의존하지 않으므로(위성 → 통합 역참조 금지) 공용 대상 선택자는 KataTargeting에 둔다.
- Perception 해제는 이미 인지된 기억을 지우지 않는다. 제외 직후 잠시 추적이 남을 수 있다.
- 무적 판정을 히트 처리기 앞에 두면 Kata 히트 트레이스 밖에서 직접 적용하는 GE는 막지 못한다. 그런 경로가 생기면 대미지 무시 쪽 판정을 같이 켠다.
- 공개 API: Kata Combat 설정에 태그 항목이 늘어난다. 기존 에셋 직렬화에는 영향이 없다.

## 사용자 확인 항목

- 구현 완료: 각 작업의 콘솔 명령이 존재하고 Shipping 구성에서 빠진다.
- 실행 확인(사용자): PIE에서 각 명령의 결과와, 토글을 끈 뒤 원래 상태로 돌아오는지.
- 이 목록만으로 에이전트가 빌드·테스트를 하지 않는다.

## 완료 시 갱신할 문서

- 연결 이슈: 구현·확인 상태.
- 새 manual `Cheats.md`: 명령 목록, 대상 선택자, Shipping 제외.
- [Gameplay Tags](../manual/Gameplay-Tags.md): 태그 루트 표의 `Status` 항목에 새 설정 태그.
- devlog: 무적·대미지 무시 판정 위치와 엔진 `God`을 쓰지 않은 이유.
- [문서 목록](../README.md): 새 manual 링크.
