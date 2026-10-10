# 캐릭터 사망 흐름과 제거

작성: 2026-10-11  
갱신: 2026-10-11  
유형: 구현 기록, 결정 기록  
대상: KataFramework 사망 처리·PC 재시작·스포너 사망 기록, KataTargeting Owned Tags 필터, KataAI AI 정지 API, 샘플 데이터  
기준: [#41](https://github.com/jaykop/Kata/issues/41) 작업 트리. 이 기록과 같은 커밋에 반영한다

## 배경과 결론

`UKataAttributeSet_Base::OnOutOfHealth`는 Health가 0이 될 때 신호를 보냈지만 받는 코드가 없었다. 그래서 시체가 계속 피해를 받고 타게팅 후보에 남았으며, 거리 관리 스포너는 시체를 거리로 제거한 뒤 다시 생성했다. PC가 죽어도 다시 시작하는 경로가 없었다.

이번 작업에서 사망 판정, 정리, 사망 연출(Action·Ragdoll), 시체 유지와 DimOut, 제거, 스포너 사망 기록, PC 재시작을 구현했다. 처리 주체는 검토 중 두 번 바뀌었다. 최종 구성은 공용 규칙을 `UKataDeathComponent`, 캐릭터 전용 정리를 `AKataCharacter`, 연출을 `UKataDeathAbility`가 맡는 구조다. 설계안은 구현 전 plan에서 정했으며, plan은 이슈를 닫으면서 삭제하고 결정 이유를 이 기록으로 옮긴다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 사망 판정 | `OnOutOfHealth`를 받는 코드가 없음 | `UKataDeathComponent`가 BeginPlay에서 구독한다. `OnOutOfHealth`는 ASC가 세트를 const로만 돌려주므로 `mutable`로 바꿨다 |
| 죽은 상태 표시 | 없음 | Kata Combat 설정의 Dead Status Tag를 ASC에 Loose Tag로 붙인다. 샘플은 `Status.Dead` |
| 사망 시 정리 | 없음 | 다음 틱에 Ability 취소와 Hurt Box 충돌 해제를 한다. 캐릭터는 정리 신호를 받아 Kata·그래프 중단, 캡슐의 Pawn 충돌 무시, 플레이어 입력 끄기, 자기 락온 해제를 하고, AI 캐릭터는 `DisableKataAI`를 호출한다 |
| 사망 연출 | 없음 | 사망 이벤트(샘플 `Event.Death`)로 `UKataDeathAbility`의 Blueprint 자식을 활성화한다. 연출 방식은 None·Action·Ragdoll·Action Then Ragdoll이다 |
| 시체 제거 | 없음 | 시체 유지 뒤 `UAbilityTask_KataDimOut`으로 머티리얼 파라미터를 올리고, 제거 진입점에서 스포너 처리기 또는 Destroy로 제거한다 |
| 타게팅 | 시체가 후보에 남음 | `UKataTargetingFilterTask_OwnedTags`를 추가했다. 락온은 기존 `LockBreakTags`에 사망 태그를 넣는다 |
| AI | 사망 후에도 StateTree·인지가 동작 | `AKataAIController::DisableKataAI`를 추가했다 |
| 스포너 | 사망을 모름 | `On Character Died`, `Get Dead Character Count`를 추가했다. 시체 제거는 공용 디스폰 큐로 처리하고, 거리 평가는 죽은 기록을 건너뛴다 |
| PC | 재시작 없음 | `AKataPlayerController`가 사망을 받아 `AKataGameMode::RequestPlayerRestart`를 호출한다. GameMode는 지연 뒤 행 기반 생성으로 새 캐릭터를 만들고, 시체 빙의를 정상적으로 푼 다음 새 캐릭터에 빙의시킨다 |

## 주요 결정과 이유

### 처리 주체

처음에는 `UKataDeathComponent` 하나가 판정부터 연출, 제거까지 맡는 안을 냈다. GA를 쓰지 않은 근거는 두 가지였다. 사망 정리가 모든 Ability를 취소하면 사망 GA도 함께 취소된다는 점, 그리고 Activation Blocked Tags로 사망이 막힐 수 있다는 점이다.

사용자가 "이 프로젝트는 GA가 적고 Component만 늘어난다"는 의문을 제기해 다시 검토했다. 두 근거는 모두 약했다. 첫째는 `CancelAbilities`의 Ignore 인자로, 둘째는 기반 클래스의 데이터 검증으로 해결된다. 기존 Component들은 대상 수명 동안 유지되는 상태, 매 프레임 도는 인프라, 씬 프리미티브라서 Component가 맞았다. 이 프로젝트에서 GA가 적은 것은 Component가 GA를 대신해서가 아니라, Kata Action이 행동 단위를 맡기 때문이라고 판단했다. Kata Action과 GA의 경계 규칙 자체는 별도 작업에서 다룬다.

검토 결과 연출은 GA로 옮겼다. 피격 반응과 같은 이벤트 → GA → `UAbilityTask_PlayKataAction` 경로를 재사용하고, 캐릭터별 연출 차이는 GA 자식을 Gameplay Data로 부여해 조립할 수 있기 때문이다. 이 결정으로 초안에 있던 사망 데이터 에셋과 행 슬롯이 빠졌다.

상태와 규칙은 한때 `AKataCharacter`에 두려 했다. 그러나 2026-10-11 사용자가 캐릭터가 아닌 피해 대상(파괴 가능한 오브젝트, 부위 파괴 등)이 생길 여지가 충분하다고 했다. 그래서 ASC와 Base 세트에만 의존하는 공용 Component로 되돌리고, 캐릭터 전용 정리만 캐릭터에 남겼다. 피격·피해 경로는 이미 캐릭터에 묶여 있지 않았다. Hit handler는 Actor 인터페이스로 ASC를 찾고, 피해 식은 Combat 세트가 없으면 Defense를 0으로 계산한다.

부위 파괴는 피해 대상이지만 사망 주체가 아니라고 정리했다. 부위가 부서져도 Actor는 살아 있기 때문이다. 같은 패턴을 따르는 별도 시스템으로 다루며, 부위별 체력을 어디에 둘지는 그 작업에서 정한다. 부위를 별도 Actor와 ASC로 만들면 이번 사망 Component를 그대로 쓸 수 있다.

### 그 밖의 결정

| 결정 | 구분 | 이유 |
|---|---|---|
| 시체는 유지 시간 뒤 Material DimOut을 거쳐 제거 | 사용자 결정 | 2026-10-10 |
| 연출은 Action, 사망 순간 Ragdoll, Action 뒤 Ragdoll을 옵션으로 제공 | 사용자 결정 | 2026-10-10 |
| 풀링 제외 | 사용자 결정 | 행 조립·Ability·태스크·StateTree 상태를 빠짐없이 되돌리는 비용이 생성 비용보다 크다. 비동기 로드와 스포너 타임슬라이싱이 생성 비용을 이미 줄인다. 제거 진입점을 하나로 모아 나중에 바꿀 자리만 남겼다 |
| 스포너는 사망 기록만 남김 | 사용자 결정 | 재생성 정책은 범위 밖이다 |
| PC 사망과 재시작 포함 | 사용자 결정 | 2026-10-10 |
| Ragdoll 임펄스 없음, DimOut은 Dynamic Material Instance | 사용자 결정 | 2026-10-10 |
| KillZ와 Kill Volume 제외 | 사용자 결정 | Kill Volume을 만들 때, 엔진도 감당하지 못하는 월드 밖 영역 처리와 함께 다룬다 |
| 캐릭터가 아닌 대상 샘플 제외 | 사용자 결정 | 2026-10-11. 공용 경로는 캐릭터로만 확인한다 |
| `OnOutOfHealth` 콜백에서는 태그만 붙이고 정리는 다음 틱 | 구현 판단 | 콜백이 GE 실행 도중에 불린다. 같은 프레임의 다음 피해 GE를 막으려고 태그만은 즉시 붙인다 |
| 사망 GA가 없거나 도중에 끝나도 제거는 반드시 요청 | 구현 판단 | 죽은 대상이 영구히 남지 않게 한다 |
| 실제 제거는 다음 틱 | 구현 판단 | Ability 종료나 델리게이트 안에서 호출되어도 호출자보다 소유자가 먼저 사라지지 않게 한다 |
| 플레이어 Pawn은 빙의가 풀릴 때까지 제거를 미룸 | 구현 판단 | 빙의 중인 Pawn을 지우면 PlayerController가 Pawn 없이 남는다. 시체를 비추는 카메라도 유지된다 |
| 재시작 때 `UnPossess` 후 `SetPawn` | 구현 판단 | 기존 생성 완료 처리는 `SetPawn`만 바꿨다. 그러면 시체가 이전 Controller를 계속 가리켜 빙의 해제 알림이 오지 않고, 제거를 영원히 기다린다 |
| PC 입력은 Pawn 입력을 꺼서 막음 | 구현 중 조정 | plan에서는 PlayerController가 막기로 했으나, 입력을 Pawn의 Input Handler가 받으므로 캐릭터 정리에서 `DisableInput`을 호출한다 |
| AI는 StateTree Disabled 상태 대신 `DisableKataAI`로 정지 | 구현 판단 | 시체는 트리를 계속 돌릴 이유가 없다. Disabled 상태는 기절처럼 다시 깨어나는 경우에 남겨 둔다 |
| DimOut 디더 입력을 `1.5 − 2 × DimOut`으로 정함 | 샘플 구성 판단 | 엔진 `DitherTemporalAA`의 결과는 `Alpha Threshold + 디더 − 0.5`다. `1 − DimOut`을 넣으면 DimOut 0에서도 클립 값 근처의 픽셀이 빠질 수 있다. 기존 알파가 있는 머티리얼은 `Min`으로 합쳐 원래 알파 테스트를 유지한다 |

## 근거

- [KataDeathComponent.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Death/KataDeathComponent.cpp): 구독, 사망 확정, 다음 틱 정리, 사망 이벤트, 제거 대기와 실행.
- [KataDeathAbility.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Death/KataDeathAbility.cpp): 연출 방식별 처리, 시체 유지, DimOut, 종료 시 제거 요청.
- [KataCharacter.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataCharacter.cpp): `HandleDeathCleanup`.
- [KataCharacterSpawner.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataCharacterSpawner.cpp): 사망 기록, 시체 제거 처리기, 거리 평가 제외.
- [KataGameMode.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Player/KataGameMode.cpp): 재시작 예약, 죽은 폰 교체.
- [KataPlayerController.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Player/KataPlayerController.cpp): 폰 사망 구독.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 코드 전체 | 사용자 Editor 빌드 보고(2026-10-11) | 성공 | Game 타깃 빌드 |
| NPC 사망·시체 피해 차단 | 사용자 PIE | Ragdoll로 쓰러지고 시체에 피격 연출이 나지 않음 | Action·Action Then Ragdoll 연출 |
| 락온·소프트 타겟 | 사용자 PIE | 락온 대상이 죽으면 해제되고 소프트 타겟이 시체를 고르지 않음 | |
| 시체 제거 | 사용자 PIE | BlackKnight 시체가 DimOut 뒤 NPC·AIController와 함께 제거됨 | SilverKnight·Hound는 DimOut 머티리얼이 없어 그대로 사라진다 |
| 거리 관리 스포너 | 사용자 PIE | 죽은 NPC가 다시 생성되지 않음 | |
| PC 재시작 | 사용자 PIE | 조작 정지, 3초 뒤 새 캐릭터, AI가 죽은 PC를 쫓지 않음 | |
| Ragdoll 품질 | 사용자 PIE | 매우 조악함. 임시 품질로 둔다 | |
| 캐릭터가 아닌 Actor, `Kill` | 없음 | 미확인 | 전체 |

에이전트는 빌드·테스트를 실행하지 않았다.

## 남은 제한과 후속 작업

- 부활은 지원하지 않는다. `OnOutOfHealth`가 Health 회복으로 다시 준비되는 기존 동작과 Loose Tag 방식을 후속 부활의 확장 자리로 남겼다.
- 엔진 KillZ는 사망 흐름을 거치지 않는다. Kill Volume 작업에서 함께 다룬다.
- 풀링, 스포너 재생성 정책, 체크포인트, HUD 표시는 범위 밖이다.
- 샘플 Physics Asset은 MCP로 메시에서 자동 생성한 그대로이며 조정하지 않았다. 사망 Action 샘플은 몽타주가 없어 만들지 않았다.
- 부위 파괴는 별도 작업이다.

## 연관 문서 반영

| 문서 | 반영 내용 |
|---|---|
| [연결 이슈](https://github.com/jaykop/Kata/issues/41) | 결과 요약 댓글과 닫기. 게시는 사용자 확인 후 |
| [사망 사용법](../manual/Death.md) | 신규 |
| [스포너 사용법](../manual/Spawner.md) | 사망 이벤트·조회, 거리 관리의 시체 재생성 제한 해소 |
| [타게팅 사용법](../manual/Targeting.md) | Kata Filter Owned Tags |
| [AI 사용법](../manual/AI.md) | `DisableKataAI` |
| [Gameplay Data 사용법](../manual/Gameplay-Data.md) | 사망 GA 부여와 Granted Abilities 실행 확인 |
| [게임플레이 태그 사용법](../manual/Gameplay-Tags.md) | `Status.Dead`, `Event.Death`, Kata Combat 사망 설정의 태그 루트 |
| [Attribute 사용법](../manual/Attributes.md) | `OnOutOfHealth`를 받는 쪽 |
| 캐릭터 사망 흐름과 제거 계획(`docs/plan/Character-Death-Plan.md`) | 삭제. 결정 이유는 이 기록으로 옮겼다 |
