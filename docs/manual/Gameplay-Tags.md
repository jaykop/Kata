# 게임플레이 태그 사용법

갱신: 2026-10-11  
대상: 프로젝트에 게임플레이 태그를 추가하고 C++에서 참조하는 사용자  
적용 기준: [게임플레이 태그 생성 구현 기록](../devlog/2026-09-24-Gameplay-Tag-Generation.md)  
확인 상태: 2026-09-24 사용자 확인(Rider 빌드, 에디터 Gameplay Tag Manager). Game 타깃 빌드와 패키징은 미확인

## 목적과 준비

프로젝트의 게임플레이 태그는 `Config/Tags` 아래에 카테고리별 ini로 나누어 관리한다.
C++에서 참조할 태그는 `Config/Tags/Native`에 두며, 빌드할 때 `KataTag.A.B` 형태로 접근하는 코드가 자동으로 생성된다.
Kata 플러그인은 게임용 태그를 정의하지 않는다. 플러그인의 API와 에셋 프로퍼티는 프로젝트가 정한 `FGameplayTag` 값을 받는다.
조건 테스트는 기존 프로젝트 태그를 조회하며 별도 Test 태그를 등록하지 않는다.

| 위치 | 용도 | C++ 참조 |
|---|---|---|
| `Config/Tags/Native/*.ini` | C++에서 참조하는 태그 | `KataTag.A.B`로 참조 가능 |
| `Config/Tags/*.ini` | 에디터와 에셋에서만 쓰는 태그 | 생성하지 않음 |
| `Config/DefaultGameplayTags.ini` | 에디터 기본 소스의 태그와 엔진 GameplayTags 설정. 로컬 카메라 샘플의 `Camera.Rail.Backview`와 태그 이름 변경 리다이렉트를 둔다 | 생성하지 않음 |

GAS Gameplay Event는 `Event` 루트와 `Config/Tags/Native/Event.ini`, StateTree 이벤트는 `StateTree.Event` 루트와 `Config/Tags/Native/StateTree.ini`로 나누어 관리한다.
2026-10-02 `GameplayEvent.Hit`를 `Event.Hit`로 옮겼고, 저장된 에셋의 이전 값은 `DefaultGameplayTags.ini`의 `GameplayTagRedirects`가 새 이름으로 읽는다.
에셋을 다시 저장하면 새 이름으로 기록된다.

별도 준비는 필요 없다. `ProjectKata.uproject`의 PreBuildSteps가 모든 빌드(Rider, Visual Studio, `Scripts/Build.ps1`) 직전에
`Scripts/Generate-NativeGameplayTags.ps1`을 실행한다.

이 자동 연결은 ProjectKata 샘플의 설정이다. 다른 프로젝트에 플러그인만 복사하면 생성 단계가 추가되지는 않는다.
재사용 코드에서는 샘플의 KataTags.h나 KataTag 전역을 참조하지 않고 태그 값을 매개변수·설정으로 받는다.

## 사용 순서

### 코드 태그 추가

1. 에디터를 닫는다. 새 태그는 전역 정의를 추가하므로 Live Coding 대신 일반 빌드를 사용한다.
2. `Config/Tags/Native`에 카테고리 이름의 ini를 만들거나 기존 파일을 연다. 형식은 다음과 같다.
   ```ini
   [/Script/GameplayTags.GameplayTagsList]
   GameplayTagList=(Tag="Event.HitReaction.Light",DevComment="Hit reaction event for a light stagger")
   ```
3. 빌드한다. 빌드 출력에 `[KataTags] N tag(s) from M file(s), K node(s): updated.`가 나오면 `Source/ProjectKata/KataTags.h`와
   `KataTags.cpp`가 갱신된 것이다. 태그를 바꾸지 않았으면 `unchanged`가 나오고 파일을 다시 쓰지 않는다.
4. C++에서 다음처럼 사용한다.
   ```cpp
   #include "KataTags.h"

   // 최하위 태그와 중간 노드 모두 FGameplayTag가 필요한 자리에 넘길 수 있다.
   const bool bIsHitReaction = Tag.MatchesTag(KataTag.Event.HitReaction);
   Container.AddTag(KataTag.Event.HitReaction.Light);
   ```

에디터의 Project Settings → GameplayTags → `Manage Gameplay Tags...`에서 태그를 추가할 때 Source로 `Native` 폴더의 ini를 고를 수도 있다.
추가한 태그는 다음 빌드부터 C++에서 쓸 수 있다.

### 에디터 전용 태그 추가

`Config/Tags`에 카테고리별 ini를 두거나 `Manage Gameplay Tags...`에서 해당 ini를 Source로 골라 추가한다. 코드는 생성되지 않는다.

## 주요 설정과 규칙

| 항목 | 의미 | 주의 |
|---|---|---|
| `GameplayTagList=(Tag="...",DevComment="...")` | `Config/Tags` 개별 소스의 태그 한 줄 | **`+`, `-` 등 ini 명령 기호를 붙이지 않는다.** 붙이면 빌드가 실패한다. 엔진 설정 파일 `DefaultGameplayTags.ini`의 배열 항목은 이 규칙의 대상이 아니며 `+GameplayTagList`를 사용한다 |
| ini 파일 이름 | 엔진의 태그 소스 이름 | `Config/Tags` 전체(하위 폴더 포함)에서 유일해야 한다. `Native`와 에디터 쪽에 같은 이름을 쓰지 않는다. 접두사 규약은 두지 않는다 |
| 태그 조각 | `A.B.C`의 각 부분 | 영문자로 시작하고 영문자·숫자·밑줄만 쓴다. 밑줄로 끝나거나 연속 밑줄을 쓸 수 없다. C++ 키워드와 `check`, `TEXT` 같은 엔진·Windows 매크로 이름은 쓸 수 없다 |
| DevComment | 태그 설명 | 생성 헤더의 멤버 주석과 네이티브 태그 설명으로 전달된다. 한국어를 쓸 수 있다 |
| `KataTag.A` | 중간 노드 | 구조체이며 `FGameplayTag`로 암시적으로 변환된다. `auto X = KataTag.A`는 태그가 아니라 구조체가 되므로 `FGameplayTag X = KataTag.A`로 쓴다 |

### 플러그인이 정한 태그 루트

Kata 플러그인의 태그 프로퍼티와 Blueprint 매개변수는 `Categories` 메타로 선택기를 아래 루트로 좁힌다. 프로젝트는 해당 용도의 태그를 이 루트 아래에 정의한다.

| 루트 | 쓰는 곳 |
|---|---|
| `Trigger` | KataGraph 엣지의 Trigger Event Tag, `SendTrigger` |
| `Window.Transition` | KataGraph 엣지의 Required Action Window Tag, Transition Window 태스크의 Window Tag |
| `Window.Cancel` | Cancel Window 태스크의 Cancel Tag, 입력 설정의 Cancel Bindings, `TryCancelKata` |
| `Equipment.Slot` | 장비 행의 Allowed Slots·부품 Slot, 장착 컴포넌트의 Slot Sockets·Default Slot, `Equip`·`Unequip`·`GetEquipmentInSlot`의 슬롯 |
| `Equipment.Type` | 장비 행의 Equipment Type, Anim Layer Setup의 Weapon Layers 키 |
| `Status` | 장비 행의 Granted Tags, Kata Combat 설정의 Super Armor Tag·Dead Status Tag. 샘플은 `Config/Tags/Status.ini`에 피격 상태 태그와 `Status.Dead`를 둔다 |
| `Identity` | 캐릭터 행과 Gameplay Data 프리뷰 셋업의 Identity Tags |
| `HurtBox` | HurtBox 컴포넌트의 Hurt Box Tags, HitBox 프리셋의 Hurt Box Tag Query. 샘플은 `Config/Tags/HurtBox.ini`에 `HurtBox.WeakPoint`·`HurtBox.Disabled`를 둔다 |
| `SetByCaller` | Apply Gameplay Effect 히트 처리기·태스크의 Set By Caller Magnitudes, Kata Combat 설정의 Damage·Poise Damage·Groggy Damage Set By Caller Tag. 샘플은 `Config/Tags/Native/SetByCaller.ini`에 `SetByCaller.Damage`·`SetByCaller.Poise`·`SetByCaller.Groggy`를 둔다 |
| `Impact` | Kata Combat 설정의 Default Impact Tag와 Impact Responses의 Impact Tag. 샘플은 `Config/Tags/Impact.ini`에 `Impact.Light`·`Impact.Heavy`·`Impact.Knock.Back`·`Impact.Knock.Down`을 둔다 |
| `Event` | Kata Combat 설정의 Impact Responses의 Reaction Event Tag, Flinch·Groggy Event Tag, Death Event Tag. 샘플은 `Config/Tags/Native/Event.ini`에 `Event.HitReaction.*`과 `Event.Death`를 둔다 |
| `GameplayCue` | Kata Combat 설정의 Impact Responses의 Hit Cue Tag. 샘플은 `Config/Tags/GameplayCue.ini`에 `GameplayCue.Hit.*`을 둔다 |
| `Faction` | 타게팅 컴포넌트의 Faction, Kata Factions의 팩션 목록·관계표, 팩션 조회 함수의 태그 입력 |
| `StateTree.Slot` | Kata AI Data의 Linked StateTree Slots 항목 Slot. 샘플은 `Config/Tags/Native/StateTree.ini`에 `StateTree.Slot.Routine`·`StateTree.Slot.Combat`를 둔다 |

샘플 프로젝트는 `Config/Tags/Faction.ini`에서 `Faction.Player`·`Faction.Enemy`·`Faction.Neutral`을 정의한다. 태그 추가만으로 팀 번호·관계가 설정되지는 않는다. [팩션 사용법](Factions.md)에 따라 목록에 등록하고 각 캐릭터에 지정한다. 이번 태그·선택기 메타 변경의 빌드·UI 확인은 미실시다.

### ASC에 넣는 태그

캐릭터의 ASC에는 `Status`와 `Identity` 두 루트의 태그만 넣는다.

| 루트 | 의미 | 예 |
|---|---|---|
| `Status` | 장착, 행동, 효과처럼 실행 중에 붙고 떨어지는 상태 | `Status.Wielding.Sword`, `Status.Cooldown.Dodge` |
| `Identity` | 캐릭터가 존재하는 동안 바뀌지 않는 특성 | `Identity.Undead` |

- 쿨다운 GE가 부여하는 태그도 `Status.Cooldown` 아래에 둔다. GAS 예제에서 흔히 쓰는 `Cooldown` 루트는 쓰지 않는다.
- 아이템 분류(`Equipment.Type`)나 슬롯(`Equipment.Slot`)처럼 데이터를 고르는 키는 ASC에 넣지 않는다.
- 팩션은 `UKataTargetingComponent`의 Faction이 가지므로 `Identity`에 다시 두지 않는다.
- 피격 상태는 `Status.HitReaction` 아래에 둔다. 하위는 `Flinch`·`Light`·`Heavy`·`Knock.Back`·`Knock.Down`·`Groggy`다.
  부모 `Status.HitReaction`은 제어권을 유지하는 흔들림 `Flinch`까지 포함한다. "제어권을 잃었는가"는 부모 태그가 아니라 `Flinch`를 뺀 하위 태그를 나열해 확인한다.
- `Status.Stance.RecoveryDelay`는 피격 직후 Poise 회복과 Groggy 감소를 미루는 동안 붙는다. 회복 GE가 Ongoing Tag Requirements로 읽는다([Attribute 사용법](Attributes.md)).
- `Status.Dead`는 사망 컴포넌트가 죽은 대상의 ASC에 Loose Tag로 붙인다. 피해 GE의 Ignore Tags, `Kata Filter Owned Tags`, 락온의 Lock Break Tags가 이 태그로 시체를 알아본다([사망 사용법](Death.md)).
- 피격 반응 설계는 [피격 반응 계획](../plan/Hit-Reaction-Plan.md)을 따른다. 태그와 설정 값만 추가했으며 이 태그를 붙이고 읽는 반응 기능은 아직 없다.

선언하지 않은 중간 부모도 네이티브 태그로 정의된다. `Event.HitReaction.Light`만 적어도 `Event`와 `Event.HitReaction`이 함께 생긴다.
`A_B.C`와 `A.B_C`처럼 C++ 이름이 같아지는 태그, 대소문자만 다른 중복 태그도 빌드 오류가 된다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 빌드 출력에 `Remove the '+' prefix` 오류 | 엔진은 `Config/Tags`의 개별 ini를 ini 명령 기호 없이 읽는다. `+GameplayTagList`는 무시되고, 에디터가 같은 파일에 태그를 추가하면 그 줄이 지워진다 | 줄 앞의 기호를 지운다 |
| `Tag ini file name ... is used more than once` 오류 | 엔진은 태그 소스를 파일 이름으로만 구분해 같은 이름의 파일을 하나로 합친다 | 한쪽 파일 이름을 바꾼다 |
| `KataTags.h`를 찾을 수 없음 | 생성 파일은 커밋하지 않으며 첫 빌드 전에는 없다 | 한 번 빌드한다 |
| 새 태그가 자동 완성에 없음 | 생성은 빌드할 때 일어난다 | 빌드 후 IDE 인덱싱을 기다린다 |
| Tag Manager에서 코드 태그의 Source가 ini가 아니라 `ProjectKata`로 보임 | 엔진이 네이티브 태그를 먼저 등록하고 처음 등록된 소스를 표시한다 | 정상이다. 코드 태그는 에디터에서 삭제·이름 변경이 제한된다 |
| 코드 태그 이름을 바꾼 뒤 컴파일 오류 | 생성 멤버 이름이 바뀌었다 | 참조 코드를 고친다. 저장된 에셋의 태그 값은 `GameplayTagRedirects`로 따로 옮긴다 |

생성 스크립트는 Win64 PowerShell 기준이다. 다른 플랫폼용 스크립트는 없다.

## 확인 상태와 근거

2026-09-24 사용자가 Rider 빌드와 에디터에서 다음을 확인했다: 태그가 없을 때의 빈 생성, `Native` ini로부터의 계층 생성과
DevComment 전달, Gameplay Tag Manager 표시, `Native` 폴더 ini에 에디터로 태그를 추가해도 기존 줄이 유지됨.
Game 타깃 빌드, 오류 입력에 대한 빌드 실패 출력, 패키징은 확인하지 않았다.

- [Scripts/Generate-NativeGameplayTags.ps1](../../Scripts/Generate-NativeGameplayTags.ps1): 생성 규칙과 오류 조건.
- [게임플레이 태그 생성 구현 기록](../devlog/2026-09-24-Gameplay-Tag-Generation.md): 결정 이유와 `+` 접두사 문제.
- [작업 상태](https://github.com/jaykop/Kata/issues).



### StateTree.Event

StateTree 이벤트 전용 루트다. Config/Tags/Native/StateTree.ini에 StateTree.Event.AI.TargetAcquired / TargetLost / TargetChanged와 StateTree.Event.Camera.Reselect를 정의한다. AI 대상 이벤트는 고정 이름으로 사용하며 AI Data별 설정은 없다. 프로젝트 C++는 KataTag.StateTree.Event.*로 접근한다. KataAI는 프로젝트 모듈에 의존하지 않고 등록된 이름을 조회한다. AI.Event.*와 Event.AI.* Redirect는 제공하지 않는다. 기존 StateTree 전이와 카메라 Status Tag Watcher의 Reselect Event Tag는 새 태그로 직접 지정한다.

### StateTree.Slot

마스터 StateTree에서 몬스터별로 교체할 Linked Asset 상태를 가리키는 루트다. Config/Tags/Native/StateTree.ini에 StateTree.Slot.Routine(비전투 루틴)과 StateTree.Slot.Combat(전투 방식)을 정의한다. 마스터 트리의 Linked Asset 상태 Tag와 `UKataAIData`의 Linked StateTree Slots 항목에 같은 태그를 지정한다. KataAI는 `Categories="StateTree.Slot"` 메타로 선택기만 제한하고 태그를 정의하지 않는다. 사용법은 [AI 사용법](AI.md)을 따른다.
