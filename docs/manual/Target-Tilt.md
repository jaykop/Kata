# 대상 방향 Tilt 사용법

갱신: 2026-10-10  
대상: KataFramework의 `Kata Task: Target Tilt`, `UKataTiltComponent`, `Kata Tilt` AnimGraph 노드, `UKataAnimInstance`의 Tilt 설정과 KataTargeting의 `ResolveAimLocation`을 쓰는 사용자  
적용 기준: [#14 대상 방향 Tilt 보정](https://github.com/jaykop/Kata/issues/14), [대상 방향 Tilt 계획](../plan/Target-Tilt-Plan.md), [대상 방향 Tilt 구현 기록](../devlog/2026-10-10-Target-Tilt.md)  
확인 상태: 2026-10-10 사용자가 Editor 빌드, PIE에서 BlackKnight가 경사면 아래의 대상 쪽으로 상체를 기울이는 것, `Kata.Tilt.Debug` 표시를 확인했다. 락온 부위 조준, AI, 취소 시 블렌드 아웃, 추적 고정, 액션 편집기 프리뷰와 시간 탐색, Game 타깃 빌드는 미확인

## 목적과 준비

공격 같은 액션 중 상체(척추 체인)를 대상 쪽으로 기울여 공격 궤적이 대상에 닿게 한다.
원래 애니메이션 위에 "애니메이션이 가정한 조준 방향"과 "실제 대상 방향"의 Pitch 차이만 더한다.
그래서 평지에서 같은 체격의 대상에게는 기울지 않고, 경사면·락온 부위·체격 차이만큼 기운다. PC와 AI가 같은 태스크를 쓴다.
몸 전체의 방향(Yaw)은 다루지 않는다. 대상 쪽으로 몸을 돌리려면 `Resolve Facing`이나 `Rotate To Facing`을 함께 쓴다([타게팅 사용법](Targeting.md)).

- 캐릭터는 `AKataCharacter` 계열이어야 한다. 이 클래스가 `UKataTiltComponent`(`KataTilt`)를 기본으로 가진다.
- 메시의 Anim Blueprint는 `Kata Anim Instance`를 부모로 하는 캐릭터(스켈레톤)별 자식 ABP여야 한다. 본 체인을 그 자식 ABP에 적기 때문이다.
- 공격 액션의 PreCommands에 `Resolve Target`이 있어야 한다. 대상이 없으면 기울지 않는다.

## 사용 순서

1. Template ABP(예: `ABP_CharacterBase`)의 AnimGraph에서 `Slot`과 `Output Pose` 사이에 **Kata Tilt** 노드를 둔다. 노드 메뉴의 Skeletal Controls 분류에 있다. 로컬 포즈와 연결하면 `Local To Component`·`Component To Local` 노드가 자동으로 들어간다.
2. 노드의 `Pitch` 핀을 `Tilt Pitch`에, `Alpha` 핀을 `Tilt Alpha`에 연결하고 컴파일한다.
3. 캐릭터별 자식 ABP(예: `ABP_BlackKnight`)의 Class Defaults에서 `Kata|Tilt` > `Tilt Bone Chain`을 루트 쪽 본부터 채운다. 아래 "체인 잡는 법"을 따른다.
4. 캐릭터 BP의 메시 Anim Class(행으로 생성하면 캐릭터 행의 Anim Blueprint)를 3의 자식 ABP로 지정한다.
5. 공격 액션의 타임라인에 `Kata Task: Target Tilt`를 두고 구간을 준비 동작부터 휘두름이 끝날 때까지로 잡는다.
6. 경사면, 계단, 키 큰 대상의 락온 부위처럼 대상과 높이 차이가 있는 곳에서 공격한다. 콘솔 `Kata.Tilt.Debug 1`로 겨누는 지점과 각도를 볼 수 있다.

### 체인 잡는 법

- 골반 위 첫 척추 본부터 팔(쇄골)이 붙은 본까지 잡는다. 골반을 넣으면 다리가 같이 돌고, 팔이 붙은 본까지 가지 않으면 무기가 Pitch 일부만 받는다.
- 가중치는 상대값이다. 체인 안에서 합이 1이 되도록 정규화하므로, 체인 끝 본과 그 아래의 팔·무기는 Pitch 전체만큼 돈다. 어깨가 앞뒤로 많이 밀리면 위쪽 본의 가중치를 늘린다.
- 샘플 DS3 스켈레톤은 `Root` 아래에서 `Pelvis`(다리)와 `Spine` → `Spine1`(쇄골·목)로 갈라진다. BlackKnight는 `Spine` 1, `Spine1` 1을 쓰고, SilverKnight도 필요하면 같게 설정한다. StarvedHound는 Tilt를 쓰지 않는다.

## 각도 계산

공격하는 캐릭터의 발 높이(캡슐 바닥)를 기준으로 세 점을 잡고, 원점에서 두 조준점을 본 각도의 차이를 목표 Pitch로 쓴다. 위쪽이 양수다.

- 원점: 공격하는 캐릭터의 수평 위치에서 높이 "자기 발 + `Aim Origin Height`".
- 가정 조준점: 대상의 수평 위치에서 높이 "자기 발 + `Expected Target Height`". 애니메이션이 가정한 키의 대상이 공격하는 캐릭터와 같은 지면에 섰을 때의 조준점이다.
- 실제 조준점: `ResolveAimLocation`이 부위 지점(PC의 락온 지점)을 돌려주면 그 위치다. 아니면 대상 캡슐의 세로 범위에서 "대상 발 + `Expected Target Height`"에 가장 가까운 높이다.

| 상황 | 결과 |
|---|---|
| 평지, 같은 체격 | 두 조준점이 같아 0이다 |
| 경사면 | 대상 발 높이 차이만큼 기운다 |
| PC가 부위에 락온 | 그 부위 쪽으로 기운다 |
| 락온하지 않은 거대한 대상 | 기준 키 높이(다리 쪽)를 겨누므로 평지에서 0이다 |
| 대상 캡슐이 기준 키보다 작음 | 캡슐 꼭대기를 겨누므로 아래로 기운다 |
| 거대 AI가 PC를 공격 | 그 AI의 `Expected Target Height`를 사람 키로 두면 평지에서 0, PC가 높은 곳에 있으면 위로 기운다 |

## 주요 설정과 실행 계약

| UI 항목 또는 API | 의미·입력 | 기본값·빈 값·실패 시 동작 |
|---|---|---|
| Target Tilt > Duration | Tilt를 유지할 구간 | 0.5초. Single Frame·0은 설정 오류 |
| Blend In Time / Blend Out Time | 구간 시작부터 Tilt가 완전히 적용될 때까지의 시간 / 구간이 끝나거나 액션이 취소된 뒤 완전히 풀릴 때까지의 시간 | 0.1초 / 0.2초. 블렌드 아웃은 구간 밖으로 이어지며, 그동안 마지막 각도를 유지한다 |
| Track Until End | 켜면 구간 끝까지 매 Tick 각도를 다시 구한다 | 켬 |
| Track Duration | Track Until End를 끄면 구간 시작부터 이 시간 동안만 다시 구하고 이후 고정한다 | 0.2초. 0이면 시작 때 한 번. 공격이 닿기 전에 끝나게 잡으면 휘두르는 동안 회피가 성립한다 |
| Max Pitch Speed | 적용 각도가 목표로 다가가는 초당 최대 각도 | 360°/s. 0이면 제한하지 않는다 |
| Max Pitch Up / Max Pitch Down | 목표 각도의 한도 | 30° / 30° |
| Min Horizontal Distance | 각도 계산에 쓰는 최소 수평 거리. 대상이 바로 위·아래에 있을 때 각도가 커지는 것을 막는다 | 50cm |
| Override Expected Target Height / Expected Target Height | 이 액션에서만 기준 키를 바꾼다(내려찍기 등) | 끔 / 0(자기 캡슐 절반 높이) |
| ABP > Tilt Bone Chain | 회전할 본 이름과 가중치 | 비어 있으면 Tilt를 적용하지 않는다. 이름 없음·중복·가중치 합 0은 데이터 검증 오류 |
| ABP > Aim Origin Height | 발에서 조준 원점까지의 높이 | 0이면 자기 캡슐 절반 높이 |
| ABP > Expected Target Height | 이 애니메이션 세트가 가정한 대상의 조준 높이(대상 발 기준) | 0이면 자기 캡슐 절반 높이(같은 체격 가정) |
| `Tilt Pitch` / `Tilt Alpha` | Anim Instance가 컴포넌트에서 복사한 적용 각도(도)와 비율(0~1) | 소유자에 Tilt 컴포넌트가 없으면 0 |
| `UKataTiltComponent::BeginTilt` / `SetTiltTarget` / `EndTilt` | 요청 등록(핸들 반환), 목표 각도 갱신, 해제. C++ 전용이며 태스크가 호출한다 | 해제된 핸들은 무시한다 |
| `UKataTargetingComponent::ResolveAimLocation` | 액션 대상을 겨눌 위치와 부위 지점 여부 | 기반은 대상 액터 위치. PC는 그 대상에 락온 중이면 락온 지점. 대상이 없으면 false |
| `Kata.Tilt.Debug` | 1이면 조준 원점(흰색)에서 가정 조준점(회색)과 실제 조준점(노랑, 정면 밖이면 빨강)까지 선을 긋고, 목표 각도(`raw`는 한도 적용 전)와 머리 위의 적용 각도·Alpha를 표시한다 | 0. Shipping 빌드에는 없다 |
| `Kata.Tilt.ForcePitch` | 0이 아니면 모든 Kata Anim Instance에 그 Pitch와 Alpha 1을 넣는다. 노드와 체인만 따로 확인할 때 쓴다 | 0(끔). Anim Blueprint 편집기 프리뷰에서도 동작한다 |

- 요청이 겹치면 가장 최근 요청을 따르고, 그 요청이 해제되면 남은 요청 중 가장 최근 것을 따른다. 콤보 전이로 액션이 취소돼도 기울기가 한 프레임에 풀리지 않는다.
- 몸 정면에서 수평 90도 넘게 벗어난 대상에게는 기울이지 않는다. Pitch는 앞뒤를 구분하지 않아 등 뒤 대상 쪽으로 기울이면 반대로 숙여 보이기 때문이다.
- 컴포넌트는 요청이 있거나 Alpha가 0보다 클 때만 Tick한다. Tick 순서는 Kata 액션 컴포넌트 → Tilt 컴포넌트 → 캐릭터 메시다.
- 노드는 Linked Anim Layer 안에 두어도 메시의 메인 인스턴스에서 체인을 읽는다. 체인은 Anim Instance 초기화 때 읽으므로 실행 중에 바꾼 값은 다음 초기화부터 반영된다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| `showdebug animation`의 Kata Tilt 줄이 `Bones: 0` | 메시 Anim Class가 Template ABP라 체인이 비어 있거나, 체인 본 이름이 스켈레톤에 없다 | 메시 Anim Class를 캐릭터별 자식 ABP로 지정하고 본 이름을 확인한다 |
| 공격 중 머리 위 표시가 `Tilt 0.0` | 평지의 같은 체격 대상, 대상 없음, 대상이 정면 밖 | 정상일 수 있다. 회색과 노란 조준선이 겹치는지, 조준선이 빨간지 본다 |
| 기울지만 각도가 모자람 | Max Pitch Up/Down 한도 | 디버그 표시의 `raw`가 `Target`보다 크면 한도를 늘린다 |
| 작은 몬스터 위로 휘두름 | 락온하지 않으면 대상 캡슐을 기준으로 겨눈다. 캡슐이 실제 몸보다 크면(예: StarvedHound는 사람 크기 캡슐) 몸보다 높은 곳을 겨눈다 | 락온해서 몸통 지점을 겨누거나, 캡슐을 몸 크기에 맞춘다. 락온 없이 Target Point를 겨누는 기능은 없다 |
| 휘두르는 중에 회피해도 따라옴 | Track Until End가 켜져 있다 | AI 공격은 끄고 Track Duration을 공격이 닿기 전으로 잡는다 |
| 루트 모션 몽타주 재생 중 한 프레임 늦게 반영 | 그동안에는 CharacterMovement가 포즈를 갱신한다 | 알려진 제한이다 |
| 이동·대기 중 머리가 대상을 보지 않음 | 상시 LookAt은 이 기능의 범위가 아니다 | [#51](https://github.com/jaykop/Kata/issues/51) |

## 확인 상태와 근거

- [KataTask_TargetTilt.h](../../Plugins/KataFramework/Source/KataFramework/Public/Tasks/KataTask_TargetTilt.h): 태스크 설정과 각도 계산.
- [KataTiltComponent.h](../../Plugins/KataFramework/Source/KataFramework/Public/Animation/KataTiltComponent.h): 요청 병합과 블렌드, Tick 순서.
- [AnimNode_KataTilt.h](../../Plugins/KataFramework/Source/KataFramework/Public/Animation/AnimNode_KataTilt.h), [KataFL_BoneChain.h](../../Plugins/KataFramework/Source/KataFramework/Public/Animation/KataFL_BoneChain.h): 체인 해석과 회전.
- [KataAnimInstance.h](../../Plugins/KataFramework/Source/KataFramework/Public/Animation/KataAnimInstance.h): `Kata|Tilt` 값과 설정, 데이터 검증.
- [KataTargetingComponent.h](../../Plugins/KataTargeting/Source/KataTargeting/Public/Targeting/KataTargetingComponent.h): `ResolveAimLocation`.
- 사용자 확인 범위는 문서 머리의 확인 상태를 따른다. [작업 상태](https://github.com/jaykop/Kata/issues/14).
