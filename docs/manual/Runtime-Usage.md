# Kata 에셋과 런타임 사용법

갱신: 2026-09-20

현재 기본 저작 단위는 UKataAction 객체를 저장한 전용 uasset이다. 액션마다 Blueprint 정의 클래스를 만들 필요가 없다.
에셋 생성과 UI 사용법은 [Editor-Usage.md](Editor-Usage.md)를 참조한다. 아래 API는 소스 구현 상태이며 빌드·실행 검증은 사용자가 담당한다.

## 구성

| 타입 | 역할 |
|---|---|
| UKataAction | 고유 설정, 로컬 태스크, ParentAction과 오버라이드를 저장하는 원본 에셋 |
| UKataResolvedAction | 부모·자식을 합친 실행용 사본. SourceAction으로 원본 참조 |
| UKataActionInstance | 실행 시간, 루프, 태스크 인스턴스, GAS 상태 |
| UKataTask / UKataTaskInstance | 태스크 설정 / 개별 실행 상태 |
| UKataComponent | 캐릭터의 시작 판정, 생성·Tick·종료 관리 |
| UAbilityTask_PlayKataAction | Gameplay Ability에서 에셋 실행 |

UKataAction은 UObject를 직접 상속하는 단일 클래스다. 에디터에서 만드는 것은 클래스나 Blueprint가 아니라 객체 에셋이다.
ParentAction은 같은 에셋 타입의 부모 객체를 참조한다. 이 상속 관계는 C++/Blueprint 클래스 상속과 무관하다.

## 게임에서 실행

액터에 UKataComponent와 유효한 ASC가 필요하다. ASC가 PlayerState 등에 있으면 Context에 명시적으로 전달한다.

~~~cpp
#include "Definition/KataAsset.h"
#include "Runtime/KataComponent.h"

UKataActionInstance* Instance = nullptr;
const EKataStartResult Result = KataComponent->PlayKataActionOnSelf(
    AttackKataAsset, TargetActor, Instance);
~~~

세부 Context가 필요하면 PlayKataAction(Asset, Context, OutInstance)를 사용한다.
시작 가능 여부만 확인하려면 CanPlayKataAction을 사용한다.
Blueprint에서 표시 이름은 Play Kata, Play Kata On Self, Can Play Kata이며 입력으로 Kata 에셋을 받는다.

실행할 때마다 부모 에셋의 최신 값으로 해석한다. 에셋 경로에는 기존 클래스 캐시를 사용하지 않는다.
실행이 시작된 뒤 원본을 수정해도 실행 중 인스턴스의 정의는 바뀌지 않는다.
Instance->GetKataAction()으로 원본, GetResolvedDefinition()으로 이 실행에서 사용하는 병합 결과를 얻는다.

Gameplay Ability에서는 UAbilityTask_PlayKataAction::PlayKataAction을 사용한다.
Ability의 Avatar에 KataComponent가 있어야 한다. 비용은 호출 Ability의 책임이다.
Kata 쿨다운을 활성화했다면 호출 Ability에서 같은 쿨다운을 다시 적용하지 않는다.
Kata 종료와 Ability 종료는 별개다.

### Cooldown Policy

기본 설정은 다음 세 항목이다.

- Enabled: 이 Kata에 쿨다운을 사용할지 결정한다.
- Duration: 쿨다운 시간(초)이다. 활성화했다면 0보다 커야 한다.
- Start Time: On Activation은 Kata 시작 순간, On End는 Kata 종료 순간부터 시간을 잰다. On End에는 완료·중단·취소가 모두 포함된다.

시간은 내부 Duration Gameplay Effect로 ASC에 저장된다. 사용자가 별도의 Gameplay Effect를 만들거나 Effect Level을 맞출 필요는 없다.
Shared Group Tags는 고급 선택 사항이다. 비워 두면 원본 Kata 에셋만 다시 실행하지 못하고, 같은 태그를 지정한 여러 Kata는 쿨다운을 공유한다.

예를 들어 LightAttack과 HeavyAttack에 모두 `Cooldown.Weapon.Sword`를 지정하면 둘 중 하나를 사용한 동안 다른 하나도 시작할 수 없다.
개별 쿨다운이라면 Shared Group Tags를 비워 둔다.

## 설정과 상속

KataTags, ActivationRequiredTags, ActivationBlockedTags, ActiveGrantedTags, StartCondition, BlockingPolicy,
CooldownPolicy, LoopPolicy를 원본 에셋에 저장한다.

- 루트 에셋은 자신의 설정 전체를 사용한다.
- 자식은 ParentAction의 현재 설정을 가져오고 OverriddenSettings에 기록한 값만 덮어쓴다.
- BlockingPolicy·CooldownPolicy·LoopPolicy의 필드는 LoopPolicy.MaxLoopCount처럼 따로 기록한다.
- 배열, 태그 컨테이너, 조건 객체 내부 변경은 해당 프로퍼티 전체를 오버라이드한다.
- TimelineTasks는 해당 에셋에서 추가한 태스크만 보관한다.
- 상속 태스크의 수정·비활성화·제거는 TaskId와 TaskOverrides로 기록한다.
- 부모 값과 같은 값을 입력했더라도 명시적으로 Reset하지 않으면 오버라이드를 유지한다.
- 부모 순환 참조는 실행 해석 오류다.

고유 설정에는 실행 중 값을 쓰지 않는다. 공유 조건도 평가 시 부작용을 만들지 않아야 한다.

## 실행 계약과 제한

기존 스케줄러와 GAS 처리 경로를 유지한다. Phase, OrderHint, AfterStart/AfterCompletion 의존성을 사용하며
AfterMeshPose는 실제 엔진 갱신 시점에 연결되기 전까지 오류로 처리한다.
쿨다운 상태는 GAS가 소유하고 루프는 하나의 인스턴스 안에서 반복한다.

기본 태스크는 Play Montage다. 프로젝트에서 UKataTask와 UKataTaskInstance를 확장할 수 있으며
에디터의 Add Task에서 네이티브·Blueprint 태스크 클래스를 선택한다.
Instanced 객체를 포함한 Map/Set 및 구조체 전체의 복잡한 소유권 복제는 아직 지원하지 않는다.
콤보 그래프(KataGraph)·입력 버퍼·다중 액션 채널은 아직 없다.

태스크의 Single Frame을 켜면 Duration과 무관하게 시작한 프레임에서 Tick을 한 번만 받고 끝난다.
Duration 0인 순간 태스크는 Tick을 한 번도 받지 않으므로 서로 다른 경로다.

Blueprint/CDO 기반 UKataDefinition 경로는 제거했다. 모든 콘텐츠와 API가 UKataAction 경로를 사용한다.
