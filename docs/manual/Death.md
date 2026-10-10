# 사망 사용법

갱신: 2026-10-11  
대상: KataFramework의 사망 컴포넌트·사망 Ability·DimOut Ability Task, PC 재시작, 스포너 사망 기록. KataTargeting의 Owned Tags 필터, KataAI의 AI 정지 API  
적용 기준: [#41 캐릭터 사망 흐름과 제거](https://github.com/jaykop/Kata/issues/41), [사망 흐름 구현 기록](../devlog/2026-10-11-Character-Death.md)  
확인 상태: 2026-10-11 사용자가 Editor 빌드와 PIE에서 NPC Ragdoll 사망과 시체 무피격, 락온 해제와 소프트 타겟 제외, DimOut 후 NPC·AIController 제거, 거리 관리 스포너의 재생성 없음, PC 입력 정지와 재시작을 확인했다. 캐릭터가 아닌 Actor의 사망과 Action 연출은 실행 미확인이다

## 목적과 준비

Health가 0이 된 대상을 죽은 상태로 확정하고, 사망 연출과 시체 유지, DimOut을 거쳐 제거한다. 플레이어 캐릭터는 지연 뒤 다시 시작한다.
대상은 `UKataAttributeSet_Base`를 가진 ASC가 있는 Actor다. 캐릭터가 아닌 피해 대상(파괴 가능한 오브젝트 등)도 같은 컴포넌트를 쓸 수 있다.

- `AKataCharacter`는 `Kata Death` 컴포넌트(`UKataDeathComponent`)를 기본으로 가진다. 캐릭터가 아닌 Actor에는 직접 추가한다.
- 사망 컴포넌트의 BeginPlay 전에 ASC에 Base 세트가 있어야 한다. 캐릭터는 행의 Gameplay Data가 세트를 추가한다. 세트가 없는 Actor에서는 사망 컴포넌트가 아무것도 하지 않는다.
- Project Settings > Kata Combat > Death에서 Dead Status Tag와 Death Event Tag를 지정한다. 플러그인은 태그를 정의하지 않으며 샘플은 `Status.Dead`와 `Event.Death`를 쓴다.

## 사용 순서

1. Project Settings > Kata Combat > Death에서 Dead Status Tag(`Status` 하위), Death Event Tag(`Event` 하위)를 고른다. Ragdoll 충돌 프로필을 바꾸려면 Ragdoll Collision Profile을 바꾼다(기본 `Ragdoll`).
2. `Kata Death Ability`의 Blueprint 자식을 만든다.
   - Ability Triggers에 Trigger Source `Gameplay Event`, Trigger Tag로 Death Event Tag를 넣는다.
   - Death에서 Presentation과 Directional Actions를, Death > Corpse에서 Corpse Lifetime, Dim Out Parameter Name, Dim Out Duration을 정한다.
   - Activation Blocked Tags는 비워 둔다. 차단 태그로 사망 Ability가 막히면 대상은 연출 없이 바로 제거된다. 태그가 있으면 데이터 검증이 오류로 알린다.
3. 사망 Ability를 캐릭터 행이 쓰는 Gameplay Data의 Granted Abilities에 넣는다. 여러 캐릭터가 같은 Ability를 공유해도 된다.
4. 피해 GE의 Target Tag Requirements > Application Tag Requirements > Ignore Tags에 Dead Status Tag를 넣는다. 회복 GE가 있으면 같은 태그를 넣는다. 시체가 피해와 회복을 받지 않게 하기 위해서다.
5. 소프트 타겟·락온·좌우 전환·AI Targeting Preset에 `Kata Filter Owned Tags`를 넣고 Excluded Tags에 Dead Status Tag를 지정한다. 후보를 모으는 태스크 바로 뒤에 두면 이후 태스크의 계산이 줄어든다.
   PC의 `Kata Player Targeting Component` > Lock Break Tags에도 Dead Status Tag를 넣어 락온 중인 대상이 죽으면 락온이 풀리게 한다.
6. Ragdoll 연출을 쓰려면 캐릭터 메시에 Physics Asset을 지정한다. 없으면 경고를 남기고 이동만 멈춘 채 선 자세로 남는다.
7. DimOut을 쓰려면 머티리얼을 준비한다. 방법은 [DimOut 머티리얼 준비](#dimout-머티리얼-준비)를 따른다.
8. GE를 거치지 않고 죽이려면 사망 컴포넌트의 `Kill`을 호출한다.

캐릭터가 아닌 Actor는 `IAbilitySystemInterface`로 ASC를 돌려주고, Gameplay Data 등으로 Base 세트를 추가하고, `Kata Death` 컴포넌트와 공격을 받을 `Kata Hurt Box`를 붙인다.
사망 Ability는 Presentation을 None으로 두거나 Start Presentation을 재정의해 메시 교체·파편 생성 같은 연출을 만든다. 재정의한 연출은 끝날 때 Finish Presentation을 호출해야 시체 단계로 넘어간다.

## 처리 순서

1. Health가 0이 되어 Base 세트의 `OnOutOfHealth`가 오거나 `Kill`이 불리면 죽은 상태가 되고, Dead Status Tag가 ASC에 Loose Tag로 붙는다. 같은 프레임에 이어서 들어오는 피해 GE는 이 태그로 막힌다.
2. 다음 틱에 모든 Ability를 취소하고 소유자의 Hurt Box 충돌을 끈다. 이어서 정리 신호를 보낸다. 캐릭터는 이 신호를 받아 현재 Kata와 그래프를 멈추고, 캡슐이 다른 Pawn을 막지 않게 하고, 플레이어 입력을 끄고, 자기 락온을 푼다. AI 캐릭터는 `DisableKataAI`도 호출한다.
3. `On Death`를 보낸 뒤 Death Event Tag로 Gameplay Event를 보낸다. 활성화된 Ability가 없으면 경고를 남기고 연출 없이 제거한다.
4. 사망 Ability가 연출을 진행한다. 연출이 끝나면 Corpse Lifetime만큼 기다린 뒤 DimOut을 진행한다.
5. DimOut이 끝나면 사망 Ability가 제거를 요청하고 끝난다. 사망 Ability가 어떤 이유로 끝나든 제거 요청은 한 번 나간다.
6. 제거는 다음 틱에 진행한다. 스포너가 만든 NPC는 스포너의 공용 제거 큐가 NPC와 소유 AIController를 함께 정리하고, 그 밖의 Actor는 Destroy한다. 플레이어가 조종 중인 Pawn은 빙의가 풀릴 때까지 제거를 미룬다.

## 주요 설정과 실행 계약

| UI 항목 또는 API | 의미·입력 | 기본값·빈 값·실패 시 동작 |
|---|---|---|
| Kata Combat > Dead Status Tag | 죽은 대상의 ASC에 붙이는 Loose Tag | 비어 있으면 경고 후 태그 없이 진행하므로 시체가 피해·타게팅에서 빠지지 않는다 |
| Kata Combat > Death Event Tag | 사망 정리 뒤 보내는 Gameplay Event | 비어 있으면 연출 없이 바로 제거한다 |
| Kata Combat > Ragdoll Collision Profile | Ragdoll 전환 때 메시에 적용할 충돌 프로필 | `Ragdoll` |
| `Is Dead` | Health 0이나 `Kill` 직후부터 true | 부활이 없으므로 다시 false가 되지 않는다 |
| `Get Death Instigator` / `Get Death Hit Result` | 마지막 피해의 Instigator와 HitResult | 없으면 nullptr·빈 값. `Kill`이면 Instigator만 있다 |
| `Kill` | Health 기본값을 0으로 맞추고 같은 사망 처리로 들어간다 | 이미 죽었거나 ASC·Base 세트가 없으면 false. Health를 올리는 지속 효과가 있어도 사망은 진행한다 |
| `Request Removal` | 시체 제거 요청. 사망 Ability가 호출한다 | 죽지 않았거나 이미 요청했으면 무시한다 |
| `On Death` | 정리가 끝난 뒤, 사망 이벤트 직전에 한 번 | HUD나 게임 규칙을 연결한다 |
| `OnDeathCleanup` / `OnDeathNative` (C++) | 사망 이벤트 전 소유자 전용 정리 신호 / `On Death`보다 먼저 오는 C++ 알림 | 캐릭터와 스포너·PlayerController가 쓴다 |
| `RemovalHandler` (C++) | 시체 제거를 대신 처리할 함수. 처리했으면 true | 바인딩된 객체가 사라졌거나 false면 Destroy한다 |
| Kata Death Ability > Presentation | None, Action, Ragdoll, Action Then Ragdoll | Ragdoll. Action 계열을 캐릭터가 아닌 대상에서 실행하면 경고 후 None으로 처리한다 |
| Kata Death Ability > Directional Actions | 피격 방향(Front·Back·Left·Right)별 사망 Kata. 방향은 마지막 피해의 HitResult와 Instigator로 고른다 | 고른 방향이 비면 Front. Front도 없거나 시작이 거절되면, Ragdoll을 포함하는 옵션은 Ragdoll로, Action은 이동만 멈추고 시체 단계로 넘어간다 |
| Kata Death Ability > Corpse Lifetime | 연출이 끝난 뒤 DimOut 전까지 기다리는 시간(초) | 5. Action은 Action 종료, Ragdoll은 전환 시점부터 센다 |
| Kata Death Ability > Dim Out Parameter Name / Dim Out Duration | 0에서 1로 올릴 머티리얼 스칼라 파라미터와 걸리는 시간(초) | `DimOut`, 1. 이름이 비거나 시간이 0이면 DimOut 없이 제거한다 |
| `Start Presentation` / `Finish Presentation` | 연출 시작(재정의 가능) / 연출 종료 알림 | 재정의한 연출은 끝날 때 `Finish Presentation`을 호출해야 한다 |
| `Start Ragdoll` / `Stop Character Movement` | Ragdoll 전환 / 이동 정지. 정적 Blueprint 함수 | Physics Asset이 없으면 Ragdoll 대신 이동만 멈춘다 |
| Kata Dim Out (Ability Task) | Avatar의 메시 머티리얼 중 파라미터가 있는 슬롯만 Dynamic Material Instance로 바꿔 선형으로 올린다 | 파라미터가 없는 슬롯은 건드리지 않는다. Ability가 먼저 끝나면 그 값에서 멈춘다 |
| Kata Filter Owned Tags > Excluded Tags | 후보 ASC에 하나라도 있으면 후보에서 뺀다 | 비어 있으면 거르지 않는다. ASC가 없는 후보는 남긴다 |
| `Disable Kata AI` | StateTree·인지·이동·Focus를 멈추고 같은 빙의 동안 다시 시작하지 않는다 | 다른 Pawn에 빙의하면 정상적으로 시작한다 |
| Kata Game Mode > Player Restart Delay | 플레이어 캐릭터가 죽은 뒤 다시 시작할 때까지의 시간(초) | 3 |
| `Request Player Restart` | 지연 뒤 Player Start에서 다시 시작한다. `AKataPlayerController`가 빙의한 폰의 사망 때 호출한다 | 이미 예약됐거나, 예약 시간에 살아 있는 폰을 조종 중이면 아무것도 하지 않는다 |
| 스포너 `On Character Died` / `Get Dead Character Count` | [스포너 사용법](Spawner.md)을 따른다 | |

### DimOut 머티리얼 준비

머티리얼에 Dim Out Parameter Name과 같은 이름의 스칼라 파라미터(기본값 0)를 추가하고, 디더로 Opacity Mask를 만든다.

1. Blend Mode를 Masked로 둔다.
2. 엔진 Material Function `DitherTemporalAA`의 Alpha Threshold에 `1.5 − 2 × DimOut`을 넣는다.
3. 기존 Opacity Mask가 없으면 디더 결과를 Opacity Mask에 연결한다. 기존 Opacity Mask(텍스처 알파 등)가 있으면 `Min(기존 값, 디더 결과)`를 연결한다.

`DitherTemporalAA`의 결과는 `Alpha Threshold + 디더 − 0.5`다. `1 − DimOut`을 그대로 넣으면 DimOut이 0일 때도 결과가 클립 값 근처까지 내려가 일부 픽셀이 빠질 수 있다.
위 식은 DimOut 0에서 결과가 1 이상, 1에서 0 미만이 되므로 클립 값과 관계없이 완전히 보였다가 완전히 사라진다. `Min`은 DimOut 0일 때 기존 알파 테스트 결과를 그대로 유지한다.
Opaque 머티리얼을 Masked로 바꾸면 렌더링 비용이 늘 수 있다. 샘플은 BlackKnight의 Body 0~3과 대검 머티리얼에만 적용했다.

### PC 사망과 재시작

`AKataPlayerController`가 빙의한 폰의 사망 컴포넌트를 구독하고, 죽으면 `AKataGameMode`에 재시작을 요청한다.
시체 빙의는 새 캐릭터가 준비될 때까지 유지해 카메라가 시체를 계속 비춘다. 행 기반 생성이 끝나면 GameMode가 시체 빙의를 풀고 새 캐릭터에 빙의시키며, 그 뒤 시체는 사망 Ability의 흐름대로 사라진다.
새 캐릭터는 행에서 새로 조립되므로 Attribute와 장비가 초기 상태다. 시작 위치는 Player Start이며 체크포인트는 없다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 죽자마자 연출 없이 사라진다 | 사망 Ability가 부여되지 않았거나 트리거 태그가 Death Event Tag와 다르다. Death Event Tag가 비어 있다 | 로그 `activated no death ability`를 확인하고 Gameplay Data와 Ability Triggers를 맞춘다 |
| Ragdoll로 쓰러지지 않고 선 채로 멈춘다 | 메시에 Physics Asset이 없다 | 로그 `the mesh has no Physics Asset`을 확인하고 Physics Asset을 지정한다 |
| 시체가 계속 맞거나 타게팅된다 | Dead Status Tag가 비었거나, 피해 GE의 Ignore Tags·Preset의 Owned Tags 필터에 태그를 넣지 않았다 | 사용 순서 1·4·5를 확인한다 |
| 사망 Kata가 재생되지 않는다 | 사망 Kata의 Activation Blocked Tags에 Dead Status Tag가 있거나 시작 조건이 맞지 않는다 | 로그 `could not start its death action`의 결과 값을 확인한다. 사망 Kata에는 사망 상태 태그를 막는 조건을 넣지 않는다 |
| DimOut이 보이지 않고 그대로 사라진다 | 머티리얼에 같은 이름의 스칼라 파라미터가 없다 | [DimOut 머티리얼 준비](#dimout-머티리얼-준비)를 따른다 |
| PC가 다시 시작하지 않는다 | GameMode가 `AKataGameMode`가 아니다 | 로그 `not a Kata Game Mode`를 확인한다 |
| GE 없이 Health를 0으로 바꿨는데 죽지 않는다 | `OnOutOfHealth`는 GE 실행 경로에서만 온다 | `Kill`을 쓴다 |
| 월드 밖으로 떨어진 Pawn이 재시작하지 않는다 | 엔진 KillZ(`FellOutOfWorld`)는 Pawn을 바로 Destroy하며 사망 흐름을 거치지 않는다 | 현재 미지원이다. Kill Volume 작업에서 함께 다룬다 |
| 죽은 캐릭터를 되살릴 수 없다 | 부활은 지원하지 않는다 | 플레이어는 재시작으로 새 캐릭터를 받는다. NPC는 스포너로 새로 생성한다 |
| 샘플 Ragdoll이 어색하다 | 샘플 Physics Asset은 메시에서 자동 생성한 그대로이며 조정하지 않았다 | 임시 품질이다. 바디와 제약을 조정하면 나아진다 |

## 확인 상태와 근거

2026-10-11 사용자가 Editor 빌드 성공을 보고하고, 샘플에서 다음 다섯 항목을 PIE로 확인했다. NPC를 죽였을 때 Ragdoll로 쓰러지고 시체에 피격 연출이 나지 않았다. 락온 중이던 대상이 죽으면 락온이 풀리고 소프트 타겟이 시체를 고르지 않았다. BlackKnight 시체가 유지 시간 뒤 DimOut으로 사라지고 NPC와 AIController가 정리됐다. 거리 관리 스포너에서 NPC를 죽이고 멀어졌다 돌아와도 다시 생성되지 않았다. PC가 죽으면 조작이 멈추고 3초 뒤 새 캐릭터로 다시 시작했으며 AI가 죽은 PC를 쫓지 않았다.
Ragdoll 품질은 사용자가 매우 조악하다고 보고했으며 임시 품질로 둔다. Action·Action Then Ragdoll 연출, 캐릭터가 아닌 Actor의 사망, `Kill` 호출은 실행으로 확인하지 않았다. 에이전트는 빌드·테스트를 실행하지 않았다.

- [KataDeathComponent.h](../../Plugins/KataFramework/Source/KataFramework/Public/Death/KataDeathComponent.h): 사망 판정·정리·이벤트·제거.
- [KataDeathAbility.h](../../Plugins/KataFramework/Source/KataFramework/Public/Death/KataDeathAbility.h): 연출 방식, 시체 유지, DimOut, 제거 요청.
- [AbilityTask_KataDimOut.h](../../Plugins/KataFramework/Source/KataFramework/Public/Death/AbilityTask_KataDimOut.h): 머티리얼 파라미터 진행.
- [KataCharacter.h](../../Plugins/KataFramework/Source/KataFramework/Public/Character/KataCharacter.h): `GetDeathComponent`, `HandleDeathCleanup`.
- [KataGameMode.h](../../Plugins/KataFramework/Source/KataFramework/Public/Player/KataGameMode.h): `RequestPlayerRestart`, Player Restart Delay.
- [KataTargetingFilterTask_OwnedTags.h](../../Plugins/KataTargeting/Source/KataTargeting/Public/Targeting/Tasks/KataTargetingFilterTask_OwnedTags.h): 태그 필터.
- [KataAIController.h](../../Plugins/KataAI/Source/KataAI/Public/Controller/KataAIController.h): `DisableKataAI`.
- [사망 흐름 구현 기록](../devlog/2026-10-11-Character-Death.md): 처리 주체 결정과 이유.
- [작업 상태](https://github.com/jaykop/Kata/issues/41).
