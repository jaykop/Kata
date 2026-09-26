# Kata 에셋과 런타임 사용법

갱신: 2026-09-25  
적용 기준: 현재 KataRuntime·KataGraph 소스. 이번 문서 갱신의 빌드·실행 확인은 미실시.

현재 기본 저작 단위는 UKataAction 객체를 저장한 전용 uasset이다. 액션마다 Blueprint 정의 클래스를 만들 필요가 없다.
에셋 생성과 UI 사용법은 [Editor-Usage.md](Editor-Usage.md)를 참조한다. 아래 API는 소스 구현 상태이며 빌드·실행 검증은 사용자가 담당한다.

## 구성

| 타입 | 역할 |
|---|---|
| UKataAction | 고유 설정, 로컬 태스크, ParentAction과 오버라이드를 저장하는 원본 에셋 |
| UKataResolvedAction | 부모·자식을 합친 실행용 사본. SourceAction으로 원본 참조 |
| UKataActionInstance | 실행 시간, 루프, 태스크 인스턴스, GAS 상태 |
| UKataTask / UKataTaskInstance | 태스크 설정 / 개별 실행 상태 |
| UKataActionComponent | 캐릭터의 시작 판정, 생성·종료 관리와 Subsystem 미지원 월드의 대체 Tick |
| UKataExecutionWorldSubsystem | 월드 내 활성 인스턴스를 Execution Priority와 시작 순서로 진행 |
| UAbilityTask_PlayKataAction | Gameplay Ability에서 에셋 실행 |
| UKataGraphInstance | 현재 그래프 노드, 예약 전이와 액션 인스턴스를 보관 |
| UKataGraphComponent | UKataActionComponent에 그래프 시작·트리거 전달 API를 연결 |

UKataAction은 UObject를 직접 상속하는 단일 클래스다. 에디터에서 만드는 것은 클래스나 Blueprint가 아니라 객체 에셋이다.
ParentAction은 같은 에셋 타입의 부모 객체를 참조한다. 이 상속 관계는 C++/Blueprint 클래스 상속과 무관하다.

## 게임에서 실행

액터에 UKataActionComponent와 유효한 ASC가 필요하다. ASC가 PlayerState 등에 있으면 Context에 명시적으로 전달한다.

UKataActionComponent는 2026-09-26 UKataComponent에서 이름을 바꿨다. 기존 에셋과 Blueprint는 DefaultEngine.ini의 Redirect로 새 이름을 따른다.
AKataCharacter의 `GetKataComponent`는 `GetActionComponent`가 됐다. 컴포넌트의 서브오브젝트 이름도 `KataActionComponent`로 바꿨으므로
Blueprint에서 부모 컴포넌트의 기본값을 바꿔 둔 기록은 이어지지 않는다. 그런 Blueprint는 기본값을 다시 설정하거나 새로 만든다.

~~~cpp
#include "Action/KataAction.h"
#include "Runtime/KataActionComponent.h"

UKataActionInstance* Instance = nullptr;
const EKataStartResult Result = ActionComponent->PlayKataActionOnSelf(
    AttackKataAsset, TargetActor, Instance);
~~~

세부 Context가 필요하면 PlayKataAction(Asset, Context, OutInstance)를 사용한다.
예제의 ActionComponent·AttackKataAsset·TargetActor는 호출자가 준비한 참조다. Build.cs에 KataRuntime 의존성을 추가한다.
반환값이 `EKataStartResult::Started`인지 먼저 확인한다. 성공해도 즉시 종료됐을 수 있으므로
계속 실행 중인지는 `Instance->IsRunning()`으로 확인한다. 새 예제의 컴파일·실행은 이번에 수행하지 않았다.
시작 가능 여부만 확인하려면 CanPlayKataAction을 사용한다.
Blueprint에서 표시 이름은 Play Kata, Play Kata On Self, Can Play Kata이며 입력으로 Kata 에셋을 받는다.

실행할 때마다 부모 에셋의 최신 값으로 해석한다. 에셋 경로에는 기존 클래스 캐시를 사용하지 않는다.
실행이 시작된 뒤 원본을 수정해도 실행 중 인스턴스의 정의는 바뀌지 않는다.
Instance->GetKataAction()으로 원본, GetResolvedDefinition()으로 이 실행에서 사용하는 병합 결과를 얻는다.

Gameplay Ability에서는 UAbilityTask_PlayKataAction::PlayKataAction을 사용한다.
Ability의 Avatar에 UKataActionComponent가 있어야 한다. 비용은 호출 Ability의 책임이다.
Kata 쿨다운을 활성화했다면 호출 Ability에서 같은 쿨다운을 다시 적용하지 않는다.
Kata 종료와 Ability 종료는 별개다.
Ability가 먼저 종료되면 AbilityTask는 실행 중 Kata를 Interrupted로 끝낸다. ExternalCancel은 Cancelled로 연결한다.
일반 종료 알림은 Completed→OnCompleted, Branched→OnBranched, 나머지→OnInterrupted다. 시작 거절은 OnFailed다.
현재 Activate 안에서 이미 끝난 인스턴스는 종료 사유와 무관하게 OnCompleted(Completed)로 알리는 제한이 있다.

시작 거절 사유는 InvalidDefinition, InvalidContext, MissingAbilitySystem, ResolveFailed, MissingRequiredTags,
BlockedByTags, BlockedByActiveKata, ConditionFailed, OnCooldown이다. Kata 자체 Cost 정책은 아직 없다.
CanPlayKataAction은 판정만 수행하며 Pre Commands나 효과를 실행하지 않는다.
중단은 `StopKata(Reason)`을 사용한다. 순간 액션 종료를 받으려면 Play 호출 전에 컴포넌트 이벤트를 구독한다.

KataFramework의 AKataCharacter는 ASC, UKataActionComponent, UKataGraphComponent, UKataTargetingComponent, UKataHitBoxComponent와
Actor Info 초기화를 제공한다. 각 컴포넌트는 `GetActionComponent`·`GetGraphComponent`·`GetTargetingComponent`·`GetHitBoxComponent`로 얻는다.
`IGenericTeamAgentInterface`를 구현해 타게팅 컴포넌트의 Faction에 해당하는 팀 번호를 돌려준다. 팩션은 컴포넌트의 Faction으로만 바꾼다.
PC는 AKataPlayerCharacter를 쓴다. 같은 타게팅 컴포넌트 자리에 UKataPlayerTargetingComponent를 만들며 `GetPlayerTargetingComponent`로 얻는다.
다른 역할의 캐릭터는 생성자에서 `ObjectInitializer.SetDefaultSubobjectClass(AKataCharacter::TargetingComponentName)`로 타입을 바꾼다.
이 컴포넌트들을 파생 BP에 따로 추가하면 중복되므로 추가하지 않는다. 메시·AnimBP·AttributeSet·게임별 초기화와 입력 연결은 직접 구성한다.
HurtBox는 부위별로 붙이므로 기본 구성에 없다.

### Cooldown Policy

기본 설정은 다음 세 항목이다.

- Enabled: 이 Kata에 쿨다운을 사용할지 결정한다.
- Duration: 쿨다운 시간(초)이다. 활성화했다면 0보다 커야 한다.
- Start Time: On Activation은 Kata 시작 순간, On End는 Kata 종료 순간부터 시간을 잰다. On End에는 Branched를 포함한 모든 종료 사유가 포함된다.

시간은 내부 Duration Gameplay Effect로 ASC에 저장된다. 사용자가 별도의 Gameplay Effect를 만들거나 Effect Level을 맞출 필요는 없다.
Shared Group Tags는 고급 선택 사항이다. 비워 두면 원본 Kata 에셋만 다시 실행하지 못하고, 같은 태그를 지정한 여러 Kata는 쿨다운을 공유한다.

예를 들어 LightAttack과 HeavyAttack에 모두 `Cooldown.Weapon.Sword`를 지정하면 둘 중 하나를 사용한 동안 다른 하나도 시작할 수 없다.
개별 쿨다운이라면 Shared Group Tags를 비워 둔다.

## 설정과 상속

KataTags, ActivationRequiredTags, ActivationBlockedTags, ActiveGrantedTags, StartCondition, BlockingPolicy,
CooldownPolicy, LoopPolicy, PreCommands, PostCommands를 원본 에셋에 저장한다.

- 루트 에셋은 자신의 설정 전체를 사용한다.
- 자식은 ParentAction의 현재 설정을 가져오고 OverriddenSettings에 기록한 값만 덮어쓴다.
- BlockingPolicy·CooldownPolicy·LoopPolicy의 필드는 LoopPolicy.MaxLoopCount처럼 따로 기록한다.
- 배열, 태그 컨테이너, 조건 객체 내부 변경은 해당 프로퍼티 전체를 오버라이드한다.
  PreCommands·PostCommands도 목록 전체가 한 값이다. 자식이 목록을 고치면 부모 목록 대신 자식 목록을 쓴다.
- TimelineTasks는 해당 에셋에서 추가한 태스크만 보관한다.
- 상속 태스크의 수정·비활성화·제거는 TaskId와 TaskOverrides로 기록한다.
- 부모 값과 같은 값을 입력했더라도 명시적으로 Reset하지 않으면 오버라이드를 유지한다.
- 부모 순환 참조는 실행 해석 오류다.

고유 설정에는 실행 중 값을 쓰지 않는다. 공유 조건도 평가 시 부작용을 만들지 않아야 한다.

## 실행 계약과 제한

Phase, OrderHint, AfterStart/AfterCompletion 의존성을 사용하며
AfterMeshPose는 실제 엔진 갱신 시점에 연결되기 전까지 오류로 처리한다.
쿨다운 상태는 GAS가 소유하고 루프는 하나의 인스턴스 안에서 반복한다.

Play Kata는 GAS 활성 상태를 적용하고 실행 가능한 시각 0의 Task를 즉시 시작한다.
일반 지속 Task는 Start → Tick(0), Single Frame Task는 Start → Tick(0) → End,
순간 Task는 Start → End로 처리한다. 완료 의존성을 기다리거나 시작 콜백에서 끝난 Task에는 Tick을 강제하지 않는다.
최초 시작을 해당 게임 프레임의 갱신으로 기록하므로 같은 프레임에 다시 Tick하지 않고, 다음 프레임부터 시간을 진행한다.
길이 0 액션이나 시작 콜백에서 끝난 액션은 Play Kata가 반환되기 전에 종료 이벤트가 발생할 수 있다.

각 Task의 Tick은 게임 프레임당 최대 한 번이다. 정상 실행 순서는 Start → Tick, Tick, Tick → End이며,
한 프레임 안에서 시작하고 끝나는 지속 태스크는 Start → Tick → End로 처리한다.
Duration 0인 순간 태스크는 Start → End로 처리하고, 취소·소유자 파괴 시에는 추가 Tick 없이 정리한다.
Tick의 DeltaTime은 이번 프레임에서 해당 Task가 실행한 구간의 길이다. 액션 최초 시작 시에는 0을 전달한다.
프레임 끝에 시작한 Task와 Single Frame Task도 DeltaTime이 0일 수 있다.

종료하는 Task는 End 직전에 Tick하고 계속 실행되는 Task는 프레임의 타임라인 진행 끝에서 Tick한다.
따라서 다른 Task의 모든 중간 상태를 Tick에서 관측하는 구조는 아니다.
완료 의존성은 실행 갱신 중 충족되면 같은 갱신에서 이어서 처리한다. 몽타주 종료 등 갱신 밖에서
완료가 통지되면 후속 Task의 시작은 다음 실행 갱신에서 처리한다.

Loop의 Max Loop Count는 최초 실행을 포함한 총 실행 횟수다. 3이면 총 3회 실행하고, 0이면 외부에서 멈출 때까지 반복한다.
회차가 끝나면 실행기가 Task를 정리하고 다음 프레임에 타임라인을 처음부터 실행한다.
끝을 넘긴 시간은 다음 회차에 넘기지 않으므로 프레임 지연에 따라 반복 완료까지 걸리는 실제 시간이 늘어날 수 있다.
Task는 Loop 여부를 알 필요가 없으며, 시작 조건·GAS 활성화·쿨다운은 회차마다 다시 적용하지 않는다.
Task의 Restart On Loop는 기본으로 켜져 있어 회차마다 다시 실행한다. 끄면 첫 회차에서만 실행하고
이후 회차에서는 건너뛴다. 반복 액션의 도입부처럼 한 번만 나가야 하는 Task에 사용한다.
건너뛴 Task의 완료를 기다리는 Task는 두 번째 회차부터 함께 시작하지 않으므로 의존 관계를 함께 확인한다.
Max Iterations Per Tick은 제거했다. 길이가 0인 Loop 타임라인은 지원하지 않는다.

프리뷰 탐색은 화면 한 프레임 안에서 여러 시뮬레이션 프레임을 진행할 수 있다. 프리뷰의 Task Tick 제한은
화면 갱신 횟수가 아니라 각 시뮬레이션 프레임을 기준으로 적용한다.

기본 태스크와 확장 방법은 아래 절 및 [태스크 제작](Task-Authoring.md)을 따른다.
설정 상속에서 Instanced 객체는 객체 배열과, 필드로 Instanced 객체를 직접 가진 구조체 배열까지 복제한다.
Map/Set 안이나 더 깊이 중첩된 Instanced 객체의 소유권 복제는 아직 지원하지 않는다.
### Pre·Post Command

UKataCommand는 액션의 시작 또는 종료 시점에 한 번 실행하고 같은 프레임 안에 끝나는 로직이다.
지속 시간이 있거나 효과를 유지해야 하는 로직은 타임라인 태스크로 만든다.

- PreCommands: 시작 조건을 통과한 뒤 GAS 활성 태그를 적용하고, 타임라인보다 먼저 선언 순서대로 실행한다.
  반복 액션이어도 첫 시작에서만 실행한다. 명령이 액션을 끝내면 남은 명령과 타임라인은 실행하지 않는다.
- PostCommands: 종료 시 타임라인 태스크를 정리한 뒤, GAS 활성 태그를 거두기 전에 선언 순서대로 실행한다.
  항목의 End Reasons로 실행할 종료 사유를 고르며 비워 두면 모든 사유에서 실행한다.
  시작하지 못하고 끝난 액션에서는 실행하지 않는다.
- 대상 변경: PreCommands 안에서만 `UKataActionInstance::SetTargetActor`로 이번 실행의 대상을 바꿀 수 있다.
  그 밖의 시점에서는 경고를 남기고 무시한다. PostCommands는 `GetEndReason`으로 종료 사유를 읽는다.
- 확장: C++에서는 `Execute_Implementation`을, Blueprint에서는 Execute 이벤트를 재정의한다.
  Blueprint Execute에서 Delay 같은 지연 노드로 이후 프레임에 작업을 예약하지 않는다. 실행이 끝나면 월드 컨텍스트가 없다.
- 제공하는 구체 Command는 아직 없다. 프로젝트나 위성 플러그인이 필요한 Command를 만든다.

### 콤보 그래프 실행

그래프를 실행할 액터에는 UKataActionComponent와 UKataGraphComponent가 모두 필요하다.
Start Kata Graph 또는 Start Kata Graph On Self로 UKataGraphInstance를 만들고, 입력·AI·Anim Notify 등에서
SendTrigger로 Gameplay Tag를 전달한다.

진입 노드에서는 Required Action Window Tag와 Timing을 무시한다. 액션 노드에서는 Trigger Event Tag가 사건을,
Required Action Window Tag가 현재 액션이 사건을 받을 수 있는 구간을 뜻한다. 여러 후보가 맞으면 Priority가 큰 엣지를,
같으면 저장된 자식·엣지 순서가 앞선 것을 선택한다. Immediate는 현재 액션을 Branched로 끝내고 즉시 다음 액션을
시작하며, OnActionEnd는 현재 액션이 정상 완료될 때까지 대기한다.

Trigger Event Tag가 빈 자동 전이는 그래프 진입 시점 또는 현재 액션의 정상 완료 시점에 평가한다. 액션이 중단·취소되면
그래프도 같은 사유로 끝난다. 트리거는 호출 시점에만 평가하며 입력 버퍼와 다중 액션 채널은 아직 지원하지 않는다.

그래프는 시작 때 받은 Context를 첫 액션에 넘긴다. 이후 전이에서는 엣지의 Keep Target(기본값 켬)이 켜져 있으면
현재 대상을 다음 액션에 넘기고, 꺼져 있으면 대상을 비워 넘긴다. 경유 노드를 지나는 전이는 현재 노드에서 나가는 첫 엣지의 값을 따른다.
액션이 PreCommands에서 대상을 바꾸면 그래프가 그 대상을 기록해 다음 전이에 이어서 쓴다. 파괴된 대상은 자동으로 비워진다.

태스크의 Single Frame을 켜면 Duration과 무관하게 시작한 프레임에서 Tick을 한 번만 받고 끝난다.
Duration 0인 순간 태스크는 Tick을 한 번도 받지 않으므로 서로 다른 경로다.

Blueprint/CDO 기반 UKataDefinition 경로는 제거했다. 모든 콘텐츠와 API가 UKataAction 경로를 사용한다.

진입 함수는 `StartGraph`·`StartGraphOnSelf`이며 `KataGraphComponent.h`를 포함하고 KataGraph에 의존한다.
StartGraph 성공은 그래프 생성 성공이다. 트리거 진입 엣지만 있으면 입력을 기다린다.
새 유효 OnActionEnd 트리거는 이전 예약을 교체한다. Conduit에서는 실행 가능한 Action 노드까지 해석하며
경유 엣지의 Window·Timing은 무시한다. 동기 전이가 32단계를 넘으면 ContractError로 종료한다.
Transition Window의 PreAcceptSeconds는 시간 판정 값이며 입력 저장 기능이 아니다.
현재 SendTrigger는 호출 시각으로 한 번 판정하므로 창이 열리기 전에 실패한 입력을 나중에 재평가하지 않는다.

## 기본 태스크

Add Task에서 추가한 뒤 Timeline Details에서 설정한다. Play Montage는 기본 Duration이 0이므로
실제 재생할 구간에 맞는 Duration을 지정한다.

| 태스크 | 주요 설정 | 실행·종료 계약 |
|---|---|---|
| Play Montage | Montage, Play Rate, Start Section, Montage End Policy, Stop Montage When Task Ends | Avatar의 AnimInstance에서 재생하며 OwningAbility가 있으면 ASC 경로를 쓴다. 기본 정책은 ContinueTimeline이며 기본적으로 태스크 종료 때 정지한다 |
| Send Gameplay Event | Event Tag, Event Target, Event Magnitude, Optional Object | 시작 시 대상 ASC에 한 번 보내고 즉시 완료한다. Duration을 늘려도 반복하지 않는다 |
| Apply Gameplay Effect | Effect Class, Effect Target, Effect Level, Remove Policy | 실행자 ASC가 적용한다. 기본 UseEffectDuration은 GE 수명을 따르고 RemoveOnTaskEnd는 받은 ASC에서 자신이 적용한 핸들을 제거한다 |
| Apply Loose Tag | Tags, Tag Target | 시작에 붙이고 종료에 같은 ASC에서 같은 수만큼 회수한다. 기본 Duration 0.2초. Duration 0·Single Frame·빈 태그는 설정 오류다 |
| Transition Window | Window Tag, Pre Accept Seconds | 액션 인스턴스에 창을 연다. 기본 Duration 0.2초이며 Duration 0·Single Frame·빈 태그는 오류다. ASC의 Loose Tag와는 다른 상태다 |

Event Target·Effect Target·Tag Target은 Avatar, Owner, ContextTarget 중에서 고른다.
Avatar와 Owner의 Actor는 다를 수 있지만 현재 실행 ASC는 공유한다. ContextTarget의 ASC는 대상 액터에서 조회한다.
실행 Context에는 대상 ASC 지정 필드가 없고, 대상이 없어도 Self로 자동 대체하지 않는다.

몽타주 종료 정책은 ContinueTimeline, FinishTaskOnMontageEnd, EndKataOnMontageEnd다.
마지막 정책은 자연 종료면 Completed, 중단됐으면 Interrupted로 액션을 끝낸다.
Stop Blend Out Time이 음수면 몽타주 설정을 사용한다. 정지할 때는 자신이 시작한 재생 인스턴스 ID를 확인한다.
AnimInstance 누락·재생 실패는 경고 후 해당 태스크 완료로 처리하므로 Kata 시작 성공만으로 몽타주 성공을 판단하지 않는다.

이벤트 Payload는 EventTag, 실행 Avatar인 Instigator, 수신 Actor인 Target, Magnitude, OptionalObject를 채운다.
이벤트 수신 Ability·구독은 사용자가 준비한다. Instant GE는 활성 효과가 남지 않아 RemoveOnTaskEnd로 되돌릴 수 없다.
일반 GE 적용 태스크에 비용 판정·SetByCaller 비용 정책은 포함되지 않는다.

## 개발 하네스

ProjectKataTesting은 bBuildDeveloperTools가 켜진 대상의 DeveloperTool 모듈이다.
AKataTestActor의 ActionToPlay에 에셋을 지정하거나 BuiltInAction으로 코드 예제를 고른다.
BuiltInAction이 None이 아니면 매번 생성한 Basic·Override·Dependency·Loop·Invalid 예제가 우선한다.

콘솔은 `Kata.Resolve <이름|에셋경로>`, `Kata.Play [이름|에셋경로]`, `Kata.List`, `Kata.Stop`이다.
List는 내장 예제와 로드된 에셋을 출력한다. 이번에 하네스를 실행하지는 않았다.
Content/KataTest는 NeverCook이며 cooked Game용 개발 하네스 사용 정책은 별도로 정해야 한다.

## 확인 상태와 근거

- [KataActionComponent.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataActionComponent.cpp): 시작 판정·동기 시작.
- [AbilityTask_PlayKataAction.cpp](../../Plugins/Kata/Source/KataRuntime/Private/GAS/AbilityTask_PlayKataAction.cpp): Ability 연결·즉시 종료 알림 제한.
- [KataRuntimeTypes.cpp](../../Plugins/Kata/Source/KataRuntime/Private/KataRuntimeTypes.cpp): Actor·ASC 선택.
- [KataGraphInstance.cpp](../../Plugins/Kata/Source/KataGraph/Private/KataGraphInstance.cpp): 전이·예약·대상 유지.
- [현재 구현 상태](../devlog/Implementation-Status.md): 사용자 확인 범위.
- [에셋 이전 안내](Asset-Migration.md): 옛 타입·설정 처리.

이번에는 소스 기준으로 문서를 갱신했다. 과거 사용자 빌드 성공을 모든 태스크·Command·그래프의 실행 확인으로 확대하지 않는다.
