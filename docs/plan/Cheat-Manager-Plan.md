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
  - PC: 사망, 무적(피격 판정 무시), 대미지 무시(피격 반응은 재생), 무한 스태미나, 재생성.
  - 대상: 사망, 즉시 그로기.
  - AI: PC를 Perception 대상에서 제외, AI 정지·재개.
  - 대상 선택자: `Self`, `Target`(PC 타게팅 컴포넌트의 현재 대상), `Nearest`, `All`.
- 제외
  - 인게임 UI. [인게임 디버그 메뉴 계획](In-Game-Debug-Menu-Plan.md)에서 다룬다.
  - 무한 마나, 대미지·Poise 배율, 적 무적, 장비 교체, 태그·GE 부여. 첫 범위가 확인된 뒤 추가한다.
  - 비용 무시. 비용 정책이 [#9](https://github.com/jaykop/Kata/issues/9)에서 보류 중이다.
  - 쿨다운 무시(2026-10-11 사용자 결정으로 제외).
  - 무적·대미지 무시 태그를 실제로 쓰는 게임플레이 기능(회피 무적 프레임 등). 이번에는 판정 경로만 만든다.

## 배치와 연결 지점

| 치트 | 둘 곳 | 연결 지점 |
|---|---|---|
| 사망 | KataFramework | `UKataDeathComponent::Kill` |
| 무적 | KataFramework | `UKataHitSubsystem::DispatchPendingHits`에서 대상 ASC가 치트 무적 태그를 가지면 그 히트의 처리기를 모두 건너뛴다. GE 적용·이벤트 전송이 하나도 실행되지 않는다. 게임플레이 무적은 기존 방식(아래 태그 항목)을 그대로 쓴다 |
| 대미지 무시 | KataFramework | `UKataAttributeSet_Base::PostGameplayEffectExecute`에서 대미지 무시 태그(게임플레이용 또는 치트용)가 있으면 Damage 메타 값을 Health에 반영하지 않는다. GE는 실행되므로 히트 반응 GE 컴포넌트가 동작한다 |
| 무한 스태미나 | KataFramework | 확장이 PC ASC의 Stamina 값 변경 델리게이트를 구독해 최대치로 되돌린다. 비용 지불 흐름은 그대로 지나간다 |
| 재생성 | KataFramework | `AKataGameMode`에 지연 없이 PC를 다시 만드는 경로를 추가하고 확장이 호출한다. 사망 시 자동 재시작(`RequestPlayerRestart`)과 같은 생성 경로를 쓴다 |
| 즉시 그로기 | KataFramework | Groggy를 Max Groggy로 올리고 Kata Combat 설정의 `Groggy Event Tag` 이벤트를 보낸다. 히트 반응 GE 컴포넌트가 Groggy 가득 참일 때 보내는 것과 같은 이벤트다 |
| Perception 제외 | KataAI | `UKataAITargetingComponent`가 시각 결과를 반영하기 전에 숨긴 액터를 거르고, 켤 때 각 AI의 시야 기록과 기억 위치를 지운다. 엔진 자극원 해제는 이미 인지한 AI와 이후 스폰된 AI에 적용되지 않아 쓰지 않는다(2026-10-11 실행 확인 결과) |
| AI 정지·재개 | KataAI | `AKataAIController`의 StateTree 컴포넌트를 `PauseLogic`·`ResumeLogic`으로 멈추고 이동을 멈춘다 |
| 대상 선택자 | KataTargeting | 각 확장이 공유하는 해석 함수. `Target`은 `UKataTargetingComponent::GetCurrentTarget`을 쓴다. KataFramework와 KataAI가 모두 KataTargeting에 의존하므로 여기에 둔다 |

- 확장은 `UCheatManager::RegisterForOnCheatManagerCreated`로 각 모듈 시작 시 등록한다. 별도 CheatManager 클래스 지정은 필요 없다.
- 치트 코드는 `UE_WITH_CHEAT_MANAGER`로 감싸 Shipping에서 빠지게 한다. 무적·대미지 무시 판정 중 게임플레이용 태그 검사는 항상 동작하고, 치트용 태그 검사만 같은 조건으로 감싼다.
- PC 대상 토글(무적, 대미지 무시, 무한 스태미나, Perception 제외)의 켜짐 여부는 PlayerController 소유의 확장 인스턴스가 기억한다. 효과는 Pawn의 ASC에 Loose Gameplay Tag나 구독으로 걸고, Pawn이 바뀌면(사망 후 재생성) 새 Pawn에 다시 건다. 상태를 GAS Inspector로 볼 수 있고, 공유 에셋에 실행 상태를 두지 않는다.

### 태그

게임플레이용과 치트용 태그를 분리한다(2026-10-11 사용자 결정). 플러그인은 태그를 정의하지 않고 Kata Combat 설정에 항목만 둔다.

게임플레이 무적은 이미 있다. 피해 GE의 Target Tag Requirements(Application) Ignore에 `Status.Invincible`을 넣어 GE 적용 자체를 막는다([Attributes](../manual/Attributes.md)). 이 방식은 바꾸지 않고, 새 설정 항목도 만들지 않는다.

| 설정 항목 | 용도 | 샘플 태그 | 판정 위치 |
|---|---|---|---|
| `Ignore Damage Tag` | 게임플레이 대미지 무시 | `Status.IgnoreDamage` | AttributeSet Health 반영 |
| `Cheat Invincible Tag` | 치트 무적 | `Cheat.Invincible` | 히트 처리기 실행 전 |
| `Cheat Ignore Damage Tag` | 치트 대미지 무시 | `Cheat.IgnoreDamage` | AttributeSet Health 반영 |

대미지 무시는 두 태그 중 하나라도 있으면 적용한다. 치트용 항목은 Shipping에서 검사하지 않는다.
치트 무적은 게임플레이 무적과 달리 피해 GE 설정에 기대지 않으므로, 이벤트 전송 처리기나 무적 조건을 넣지 않은 GE도 모두 막는다. 새 루트 `Cheat`는 [Gameplay Tags](../manual/Gameplay-Tags.md) 루트 목록에 추가한다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| 로직 위치 | 확정 | 기능별 플러그인의 `UCheatManagerExtension`. 인게임 UI는 샘플 DeveloperTool 모듈(2026-10-11 사용자 결정) |
| 이슈 분할 | 확정 | 치트와 인게임 UI를 두 이슈로 나눈다(2026-10-11 사용자 결정) |
| 무적·대미지 무시 태그 | 확정 | 게임플레이용과 치트용을 분리한다(2026-10-11 사용자 결정). 게임플레이 무적은 기존 `Status.Invincible` GE 조건을 쓰고, 나머지 세 항목을 Kata Combat 설정에 추가한다. 위 태그 표 |
| 쿨다운 무시 | 확정 | 범위에서 제외(2026-10-11 사용자 결정) |
| 재생성 방식 | 확정 | GameMode로 PC를 다시 만든다. 위치·상태는 초기화된다(2026-10-11 사용자 결정) |
| 콘솔 명령 이름 | 제안 | `KataKill <Selector>`, `KataGod`, `KataNoDamage`, `KataInfiniteStamina`, `KataRespawn`, `KataGroggy <Selector>`, `KataHideFromAI`, `KataPauseAI <Selector>` |
| 토글 인자 | 제안 | 인자가 없으면 토글, `0`/`1`이면 지정 |

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| A1 | 높음 | KataTargeting 대상 선택자, KataFramework 확장의 사망·즉시 그로기 | 없음 | 콘솔로 `Self`·`Target`·`Nearest`·`All` 대상이 사망 흐름과 그로기 반응을 탄다 |
| A2 | 높음 | 무적·대미지 무시 판정과 설정 태그 | 없음 | 무적이면 피격 반응·대미지가 모두 없고, 대미지 무시면 반응만 재생된다 |
| A3 | 보통 | 무한 스태미나 | 없음 | 스태미나를 쓰는 액션 후에도 값이 최대로 남는다 |
| A4 | 보통 | KataAI 확장: Perception 제외, AI 정지·재개 | 없음 | 제외 중 적이 PC를 새로 인지하지 않고, 정지한 AI가 행동하지 않는다 |
| A5 | 낮음 | 재생성 | 없음 | 살아 있거나 죽은 PC가 지연 없이 Player Start에서 새 캐릭터로 시작한다 |

## 영향과 제한

- 플러그인 경계: 각 확장은 자기 플러그인 기능만 다룬다. KataAI는 KataFramework에 의존하지 않으므로(위성 → 통합 역참조 금지) 공용 대상 선택자는 KataTargeting에 둔다.
- Perception 제외는 KataAI 타게팅에서만 거른다. 엔진 Perception을 직접 읽는 로직에는 적용되지 않는다.
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
