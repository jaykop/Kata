# 대상 방향 Tilt 구현

작성: 2026-10-10  
갱신: 2026-10-10  
유형: 구현 기록·결정 기록  
대상: KataTargeting `ResolveAimLocation`, KataFramework `UKataTiltComponent`·`UKataAnimInstance` Tilt 설정·`FAnimNode_KataTilt`·`UKataTask_TargetTilt`, 새 UncookedOnly 모듈 `KataFrameworkAnimGraph`, 샘플 BlackKnight 설정  
기준: 커밋 57d3746 위의 미커밋 작업 트리. 이 기록과 함께 커밋한다

## 배경과 결론

[#14](https://github.com/jaykop/Kata/issues/14)에서 공격 중 상체를 대상 쪽으로 기울여, 경사면이나 키 큰 대상의 부위에도 공격 궤적이 닿게 했다.
원래 애니메이션 위에 "애니메이션이 가정한 조준 방향"과 "실제 대상 방향"의 Pitch 차이만 더한다. PC와 AI가 같은 경로를 쓴다.
설계 결정(D1~D15)은 [대상 방향 Tilt 계획](../plan/Target-Tilt-Plan.md)에 있고, 이 기록은 구현 결과와 구현 중에 정한 것, 시행착오를 남긴다.
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

- **설계 결정은 계획을 따른다.** 각도 계산식, 높이 설정 위치, 추적 후 고정, 요청 블렌드, 모듈 구성은 2026-10-10 사용자가 계획의 제안대로 확정했다([계획](../plan/Target-Tilt-Plan.md) D1~D15).
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
| Editor 빌드 | 사용자 빌드(구현 후, 디버그 추가 후, 문자열 수정 후) | 통과 | Game 타깃 빌드 |
| PIE 기울기 | 사용자 확인 | 경사면 아래의 대상 쪽으로 상체가 아래로 기운다 | 락온 부위, 위쪽 대상, AI, 거대 AI의 기준 키 |
| 디버그 표시 | 사용자 확인 | 조준선과 각도 표시가 정상이며 문자열이 쌓이지 않는다 | `Kata.Tilt.ForcePitch` |
| 수명 | 미확인 | — | 콤보 전이·취소 블렌드 아웃, 추적 고정 뒤 회피, 등 뒤 대상 |
| 프리뷰 | 미확인 | — | 액션 편집기 프리뷰와 시간 탐색 |
| 샘플 설정 | unreal-mcp 조회 | `ABP_BlackKnight` 체인 저장, `ABP_CharacterBase` 노드 연결 확인 | 노드 핀의 Property Access가 읽는 변수 이름(MCP로 읽지 못함) |

## 남은 제한과 후속 작업

- 캡슐이 몸보다 큰 몬스터는 락온하지 않으면 몸보다 높은 곳을 겨눈다. 지금 규칙을 유지한다.
- 루트 모션 몽타주 재생 중에는 CharacterMovement가 포즈를 갱신하므로 Tilt 값이 한 프레임 늦을 수 있다.
- 상시 LookAt은 [#51](https://github.com/jaykop/Kata/issues/51), 발 IK는 별도 설계 세션에서 다룬다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#14](https://github.com/jaykop/Kata/issues/14) | 구현 결과 댓글과 `status: needs-verification` 초안. 게시는 사용자 확인 후 |
| [대상 방향 Tilt 사용법](../manual/Target-Tilt.md) | 신규 |
| [타게팅 사용법](../manual/Targeting.md) | `ResolveAimLocation` 행 |
| [애니메이션 레이어 사용법](../manual/Animation-Layers.md) | `Kata|Tilt` 값·설정 행 |
| [런타임 사용법](../manual/Runtime-Usage.md) | `AKataCharacter`의 Tilt 컴포넌트 |
| [대상 방향 Tilt 계획](../plan/Target-Tilt-Plan.md) | 구현 중 결정 D16~D18 추가. 이슈를 닫을 때 결정 이유를 이 기록으로 옮기고 삭제한다 |
| [AGENTS.md](../../AGENTS.md) | KataFramework 모듈 목록에 `KataFrameworkAnimGraph` |
