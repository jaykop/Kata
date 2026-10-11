# 대상 방향 Tilt 구현

작성: 2026-10-10  
갱신: 2026-10-11  
유형: 구현 기록·결정 기록  
대상: KataTargeting `ResolveAimLocation`, KataFramework `UKataTiltComponent`·`UKataAnimInstance` Tilt 설정·`FAnimNode_KataTilt`·`UKataTask_TargetTilt`, 새 UncookedOnly 모듈 `KataFrameworkAnimGraph`, 샘플 BlackKnight 설정  
기준: 구현 커밋 1798a64. 이슈 종료 시 계획서의 결정을 이 기록으로 옮겼다(2026-10-11)

## 배경과 결론

[#14](https://github.com/jaykop/Kata/issues/14)에서 공격 중 상체를 대상 쪽으로 기울여, 경사면이나 키 큰 대상의 부위에도 공격 궤적이 닿게 했다.
원래 애니메이션 위에 "애니메이션이 가정한 조준 방향"과 "실제 대상 방향"의 Pitch 차이만 더한다. PC와 AI가 같은 경로를 쓴다.
설계 결정은 #14를 닫으며 계획서에서 이 기록으로 옮겼다. 결정 번호(D1~D18)는 이슈 댓글과 맞추기 위해 유지한다.
이동·대기 중 머리가 대상을 보는 상시 LookAt은 [#51](https://github.com/jaykop/Kata/issues/51)로 나눴다. 현재 사용법은 [대상 방향 Tilt 사용법](../manual/Target-Tilt.md)을 따른다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 조준 위치 | 거리 기준 위치(`ResolveApproachLocation`)만 있음 | 기반 `UKataTargetingComponent::ResolveAimLocation` 추가, PC는 그 대상의 락온 지점을 부위 지점으로 돌려준다 |
| 요청과 블렌드 | 없음 | `UKataTiltComponent`가 요청을 핸들로 받아 최근 요청 우선으로 병합하고, 해제 뒤 마지막 각도를 유지한 채 페이드한다. `AKataCharacter` 기본 서브오브젝트 `KataTilt` |
| Anim Instance | Tilt 값 없음 | `TiltPitch`·`TiltAlpha` 스냅샷, Class Defaults `TiltBoneChain`·`AimOriginHeight`·`ExpectedTargetHeight`와 데이터 검증 |
| 포즈 적용 | 없음 | `FAnimNode_KataTilt`(런타임)와 공용 `KataFL::RotateBoneChain`. 편집기 노드 `UAnimGraphNode_KataTilt`는 새 UncookedOnly 모듈 `KataFrameworkAnimGraph` |
| 타임라인 | 없음 | `Kata Task: Target Tilt`(Animation 단계). 각도 계산, 추적 후 고정, 정면 밖 대상 제외 |
| 디버그 | 없음 | `Kata.Tilt.Debug`(조준선·각도 표시), `Kata.Tilt.ForcePitch`(대상과 무관하게 각도 강제). `ENABLE_DRAW_DEBUG` 조건 |
| 샘플 | Tilt 없음 | `ABP_CharacterBase`에 Kata Tilt 노드(사용자 배치), `ABP_BlackKnight` 체인 `Spine`·`Spine1`(unreal-mcp로 설정), BlackKnight BP 메시 Anim Class를 `ABP_BlackKnight`로 변경(사용자), `KA_BlackKnight_Attack01`에 태스크(사용자) |

## 주요 결정과 이유

- **D1~D3 정의와 범위.** 원래 동작 위에 가정 조준 방향과 실제 대상 방향의 Pitch 차이만 더한다. PC·AI가 같은 경로를 쓰고 PC 락온, 경사면, AI와 PC의 상대 위치를 반영한다. Pitch만 다루며 몸 Yaw는 Resolve Facing·Rotate To Facing이 맡는다(2026-10-10 사용자).
- **D4 조준 위치 API.** 기반 `UKataTargetingComponent`에 `ResolveAimLocation(ActionTarget, OutLocation, bOutIsTargetPoint)`(BlueprintNativeEvent)를 새로 뒀다. 값은 지금 `ResolveApproachLocation`과 같지만 거리 기준과 조준 높이는 의미가 달라 나눴다.
- **D5 상시 LookAt 분리.** 이동·대기 중 머리·목이 대상을 보는 연출은 Yaw·Pitch를 모두 다루는 다른 성격의 기능이라 [#51](https://github.com/jaykop/Kata/issues/51)로 나눴다. Tilt와 조준 위치 API, 체인 회전 함수(`KataFL::RotateBoneChain`), 값 전달 구조를 공유한다.
- **D6·D7 포즈 적용과 체인 위치.** C++ AnimNode 하나를 Template ABP의 Slot 뒤에 둔다. 엔진 `FAnimNode_LookAt`은 본 하나의 축을 목표점에 정렬해 원래 자세를 덮어쓰고 체인에 각도를 나누지 못해 쓰지 않았다. Template ABP에는 스켈레톤이 없고 샘플 스켈레톤마다 본 이름이 달라, 체인은 캐릭터별 자식 ABP Class Defaults에 이름으로 둔다. 대안이던 Control Rig는 스켈레톤마다 리그 에셋이 필요하다.
- **D8 각도 계산.** 원점(자기 발 + 원점 높이), 가정 조준점(대상 수평 위치, 자기 발 + 기준 키), 실제 조준점의 각도 차이를 쓴다. 실제 조준점은 부위 지점이면 그 위치, 아니면 대상 캡슐 세로 범위에서 "대상 발 + 기준 키"에 가장 가까운 높이다. 대상 중심을 겨누는 대안은 거대한 대상 앞에서 크게 위로 기울어 채택하지 않았다.
- **D9 높이 설정 위치.** `AimOriginHeight`·`ExpectedTargetHeight`는 체인과 같은 자식 ABP Class Defaults에 둔다(0이면 자기 캡슐 절반 높이). 애니메이션 세트에 묶인 값이고 캐릭터 행이 이미 Anim Class로 ABP를 고르기 때문이다. 캐릭터 행 데이터 에셋은 대안으로 검토했다. 기준 키는 태스크에서 덮어쓸 수 있다.
- **D10 추적 후 고정.** Track Duration 동안만 목표를 다시 구하고 이후 고정한다. AI가 휘두르는 중에도 따라오면 PC의 회피가 성립하지 않는다.
- **D11 중단과 겹침.** 태스크는 어떤 사유로 끝나도 요청을 해제하고, 컴포넌트가 마지막 각도를 유지한 채 Blend Out Time 동안 풀어 콤보 전이 때 한 프레임에 풀리지 않게 한다. 요청이 겹치면 가장 최근 요청을 따른다.
- **D12·D13 모듈과 이름.** KataFramework에 `AnimGraphRuntime`을 Public 의존으로 더했다. 이름은 `UKataTask_TargetTilt`("Kata Task: Target Tilt", Animation 단계), `UKataTiltComponent`(기본 서브오브젝트 `KataTilt`, 실행 상태만 가짐), `FAnimNode_KataTilt`("Kata Tilt")다.
- **D14·D15 대상 없음과 기본값.** Context Target이 없거나 자신이면 요청하지 않고, 타게팅 컴포넌트가 없으면 대상 액터 위치로 계산한다. 기본값(구간 0.5초, 블렌드 0.1·0.2초, 각도 한도 30°, 각속도 360°/s, 최소 수평 거리 50cm)은 [사용법](../manual/Target-Tilt.md)에 있다.
- **편집기 노드는 UncookedOnly 모듈에 둔다.** Editor 모듈은 쿠킹하지 않은 `-game` 실행에서 로드되지 않아 ABP 그래프의 노드 클래스를 찾지 못할 수 있다. 엔진 Animation Warping 플러그인도 런타임 모듈과 UncookedOnly 모듈로 나눈다.
- **Tick 순서를 컴포넌트가 직접 건다.** Tilt 컴포넌트가 `BeginPlay`에서 Kata 액션 컴포넌트를 자기 선행으로, 자신을 캐릭터 메시의 선행으로 건다. 태스크가 이번 프레임에 넘긴 목표를 같은 프레임의 포즈에 반영하기 위해서다.
  엔진 Tick 관리자는 선행 Tick이 꺼져 있으면 기다리지 않으므로(`TickTaskManager.cpp`의 선행 Tick 처리), 요청이 없어 Tilt 컴포넌트의 Tick을 꺼도 메시 Tick은 막히지 않는다.
- **회전 각도의 부호를 뒤집는다.** `FRotator`의 양의 Pitch(위쪽)는 오른쪽(+Y) 축 기준 음의 쿼터니언 회전이다(`FRotator::Quaternion`). 노드는 위쪽이 양수인 Pitch를 받아 부호를 뒤집어 적용한다. 사용자 PIE에서 아래쪽 대상에게 아래로 기우는 것을 확인했다.
- **디버그 기능을 추가했다.** 사용자가 적용 여부를 구분하기 어렵다고 해 추가했다. ForcePitch는 노드·체인 문제와 각도 계산 문제를 나눠 보기 위한 것이다.
- **조준점 규칙은 그대로 둔다.** 캡슐이 몸보다 큰 작은 몬스터(StarvedHound) 위로 휘두르는 문제를 두고 세 안을 검토했다. 2026-10-10 사용자 결정은 다음과 같다.
  - C(락온하지 않아도 겨누기용 Target Point를 쓴다): 소프트 타겟이 활성화되어 있으면 의미가 없다는 이유로 제외했다.
  - A(캡슐을 몸 크기에 맞춘다), B(작은 대상은 꼭대기 대신 중심을 겨눈다): 채택하지 않았다.
- **샘플 적용 범위.** BlackKnight만 설정한다. SilverKnight는 필요하면 같은 체인을 쓰고, StarvedHound는 Tilt 대상이 아니다(2026-10-10 사용자).

## 시행착오

- **메시 Anim Class가 Template이면 체인이 비어 있다.** 샘플 BlackKnight BP 두 개의 메시가 Template `ABP_CharacterBase`를 직접 써서 `ABP_BlackKnight`에 적은 체인이 쓰이지 않았다. `showdebug animation`의 Kata Tilt 줄에 `Bones: 0`이 나왔다. 사용자가 메시 Anim Class를 `ABP_BlackKnight`로 바꿔 해결했다.
- **`DrawDebugString`의 Duration -1은 만료 없음이다.** 선·구체 그리기의 -1은 한 프레임이지만, HUD 디버그 문자열은 -1을 만료 없음으로 처리해(`HUD.cpp`) 매 Tick 그린 문자열이 쌓였다. 한 번 그리고 지우는 0으로 바꿨다.
- **평지에서는 기울기가 0이다.** 설계대로 같은 체격의 평지 대상에는 기울지 않아 처음에는 적용 여부를 구분하기 어려웠다. 경사면과 디버그 표시로 확인했다.
- **unreal-mcp의 배열 설정.** `set_properties`는 기존 배열 요소 변경과 크기 변경을 한 번에 처리하지 못했다. 같은 크기로 먼저 바꾼 뒤 요소를 추가했다.

## 근거

- [KataTask_TargetTilt.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Tasks/KataTask_TargetTilt.cpp): 각도 계산, 추적, 디버그 조준선.
- [KataTiltComponent.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Animation/KataTiltComponent.cpp): 요청 병합·페이드, Tick 선행 관계, 디버그 콘솔 변수.
- [AnimNode_KataTilt.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Animation/AnimNode_KataTilt.cpp), [KataFL_BoneChain.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Animation/KataFL_BoneChain.cpp): 체인 해석, 누적 강체 변환 회전, 부호.
- [KataFramework.uplugin](../../Plugins/KataFramework/KataFramework.uplugin), [KataFrameworkAnimGraph.Build.cs](../../Plugins/KataFramework/Source/KataFrameworkAnimGraph/KataFrameworkAnimGraph.Build.cs): UncookedOnly 모듈.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 빌드 | 사용자 빌드 | Editor 빌드 통과(구현 후, 디버그 추가 후, 문자열 수정 후). 2026-10-11 Game 타깃 포함 확인 완료 보고 | 없음 |
| PIE 기울기 | 사용자 확인 | 경사면 아래의 대상 쪽으로 상체가 아래로 기운다. 2026-10-11 락온 부위·위쪽 대상·AI 확인 완료 보고 | 없음 |
| 디버그 표시 | 사용자 확인 | 조준선과 각도 표시가 정상이며 문자열이 쌓이지 않는다 | 없음 |
| 수명 | 사용자 확인(2026-10-11) | 콤보 전이·취소 블렌드 아웃, 추적 고정 뒤 회피 확인 완료 보고 | 없음 |
| 프리뷰 | 사용자 확인(2026-10-11) | 액션 편집기 프리뷰와 시간 탐색 확인 완료 보고 | 없음 |
| 샘플 설정 | unreal-mcp 조회, 사용자 실행 | `ABP_BlackKnight` 체인 저장, `ABP_CharacterBase` 노드 연결, BlackKnight BP 메시 Anim Class `ABP_BlackKnight` | 없음 |

## 남은 제한과 후속 작업

- 캡슐이 몸보다 큰 몬스터는 락온하지 않으면 몸보다 높은 곳을 겨눈다. 지금 규칙을 유지한다.
- 루트 모션 몽타주 재생 중에는 CharacterMovement가 포즈를 갱신하므로 Tilt 값이 한 프레임 늦을 수 있다.
- 상시 LookAt은 [#51](https://github.com/jaykop/Kata/issues/51), 발 IK는 별도 설계 세션에서 다룬다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#14](https://github.com/jaykop/Kata/issues/14) | 2026-10-11 사용자 확인 후 종료 |
| [대상 방향 Tilt 사용법](../manual/Target-Tilt.md) | 신규 |
| [타게팅 사용법](../manual/Targeting.md) | `ResolveAimLocation` 행 |
| [애니메이션 레이어 사용법](../manual/Animation-Layers.md) | `Kata|Tilt` 값·설정 행 |
| [런타임 사용법](../manual/Runtime-Usage.md) | `AKataCharacter`의 Tilt 컴포넌트 |
| 대상 방향 Tilt 계획 | 결정을 이 기록으로 옮기고 삭제했다(2026-10-11) |
| [AGENTS.md](../../AGENTS.md) | KataFramework 모듈 목록에 `KataFrameworkAnimGraph` |
