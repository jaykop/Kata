# 전진 제한

작성: 2026-10-10  
갱신: 2026-10-10  
유형: 결정 기록·구현 기록  
대상: KataFramework `UKataRootMotionCurveComponent`(전진 제한 단계)·`KataFL_Approach`·`UKataTask_LimitApproach`·`UKataTask_AutoDash`(거리 기준)  
기준: 커밋 e9b75a1과 이 기록을 담은 커밋. #38 종료 시 삭제한 `docs/plan/Approach-Limit-Plan.md`의 결정을 옮겼다

## 배경과 결론

[#38](https://github.com/jaykop/Kata/issues/38)은 공격 루트 모션 때문에 실행 주체가 대상에게 파고들거나, 몸이 닿아 옆으로 미끄러지는 현상을 막는 기능이다.
루트 모션 속도가 대상 캡슐에 막히면 CharacterMovement가 표면을 따라 미끄러지게 처리하는데, 대상 쪽 성분이 남아 있는 한 계속 밀며 돈다.
[오토 대시](2026-10-10-Auto-Dash.md)의 Stop Distance는 보정 구간 안에서만 지켜지므로 구간 뒤의 전진이나 Min Dash Distance로 강제한 전진은 막지 못한다.

결론은 다음과 같다. #36·#37과 같은 `ProcessRootMotionPreConvertToWorld` 처리에 3단계 전진 제한을 이어 붙이고, 기준은 대상 HurtBox 도형 표면에서 자신에게 가장 가까운 점으로 정했다.
오토 대시도 같은 기준으로 거리를 재게 바꿨다. 사용법은 [루트 모션 커브 사용법](../manual/Root-Motion-Curve.md#전진-제한)을 따른다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 루트 모션 처리 | 1단계 커브 대체, 2단계 거리 보정. 몽타주 전제를 만족하지 못하면 전체를 건너뜀 | 3단계 전진 제한 추가. 몽타주 단계(`ApplyMontageStages`)와 분리해, Root Motion Mode·섹션 경로 복원과 관계없이 적용한다 |
| 전진 제한 요청 | 없음 | `BeginApproachLimit`/`EndApproachLimit`(핸들, 한 번에 하나). 요청은 자신 캡슐 축에서 가장 가까운 기준점을 구하는 함수와 Limit Distance를 담는다 |
| 접근 기준 계산 | 없음 | `KataFL_Approach`: 후보 HurtBox 선정, Sphere·Capsule·Box 표면에서 선분까지의 최근접점, 후보가 없을 때 대상 캡슐 표면 |
| 태스크 | 없음 | `UKataTask_LimitApproach`(Limit Distance 10cm, Soft Lock Hurt Box Preset 선택 사항, Duration 0.5초) |
| 오토 대시 거리 기준 | 락온 지점 또는 대상 위치와 캡슐 반지름 | 대상 HurtBox 표면 최근접점. HurtBox가 없으면 기존 `ResolveApproachLocation` 기준 |

## 주요 결정과 이유

| 결정 | 채택 | 검토한 대안과 제외 이유 |
|---|---|---|
| 처리 위치 | 같은 델리게이트 처리의 3단계 | CharacterMovement 서브클래스의 `ApplyRootMotionToVelocity` 재정의: RootMotionSource까지 잡지만 캐릭터 클래스를 바꿔야 하고, #37에서 정한 단일 처리 체인과 어긋난다 |
| 제한할 이동 | 애니메이션 루트 모션(커브 대체·오토 대시 결과 포함)만 | 입력 이동·Apply Root Motion 계열까지: 위 서브클래스가 필요하다 |
| 기준점(사용자 결정) | 후보 HurtBox마다 도형 표면에서 자신에게 가장 가까운 점, 그중 가장 가까운 점 | 대상 캡슐 표면: 큰 몬스터는 캡슐과 실제 몸 모양이 다르다. HurtBox ComponentLocation: 큰 부위는 중심과 표면 거리가 크다 |
| 후보 선정(사용자 결정) | 락온 중인 PC와 AI는 대상의 HurtBox, 소프트락은 Targeting Preset으로 고른 HurtBox | 항상 대상 하나: 소프트락은 대상이 방향 기준일 뿐이라 앞쪽의 다른 적에게 파고들 수 있다 |
| 가장 가까운 후보 찾기 | 후보 전부의 표면까지 계산한다 | 중심이 가까운 HurtBox를 먼저 고른 뒤 그 표면만 계산: 꼬리·팔·몸통처럼 길거나 큰 도형은 중심이 멀어도 표면이 더 가까울 수 있어 큰 대상일수록 틀린 부위를 고른다. 도형 계산은 가벼워 후보 수십 개도 비용이 작다(사용자 질문에 대한 판단) |
| 거리 측정 | 자신 캡슐 축(선분)과 도형 사이 | 자신 위치(점)에서의 최근접점: 다리·머리처럼 높이가 다른 부위에서 간격이 틀어진다 |
| 자르는 방식 | 기준점 쪽 성분만 남은 간격까지 자르고 옆·수직 이동과 회전은 남긴다. 이미 간격 안이면 밀어내지 않는다 | 수평 이동 전체 정지: 대상 옆으로 도는 공격까지 멈춘다 |
| 설정 위치 | 별도 태스크(공격마다 구간과 간격을 정한다) | 오토 대시 옵션, 캐릭터 기본값 |
| 소프트락 Preset | 태스크 프로퍼티, 선택 사항. 구간 시작에 한 번 실행하고 표면 점은 매 갱신 다시 구한다. 프로젝트 기본값은 두지 않는다(사용자 결정) | PC 타게팅 컴포넌트 설정: KataTargeting이 HurtBox를 몰라 결과 해석은 결국 KataFramework가 한다. 매 갱신 재실행: 비용이 든다 |
| 후보가 없을 때 | Preset 결과 → 대상의 HurtBox → 대상 캡슐 표면 순서. 대상도 없으면 제한하지 않는다 | 후보가 없으면 제한하지 않음: HurtBox를 아직 붙이지 않은 대상에서 아무 효과가 없다 |
| 오토 대시와의 관계(사용자 결정) | 오토 대시도 같은 HurtBox 표면 기준으로 Stop Distance를 잰다. 함께 쓰면 실제 정지 간격은 Stop과 Limit 중 큰 값이다 | 기준을 따로 둠: 같은 50cm라도 대상마다 실제 간격이 달라 둘을 함께 맞추기 어렵다. 오토 대시 구간 동안 Stop Distance를 하한으로도 적용: Min Dash Distance로 "붙어 있어도 내딛기"를 할 수 없게 된다 |

오토 대시는 이번 실행의 대상에게 다가가는 동작이므로 소프트락 Preset 없이 대상의 HurtBox만 쓴다.
그래서 큰 몬스터에 락온했을 때 오토 대시는 락온 부위 대신 가장 가까운 부위 앞에서 멈춘다. 방향은 회전 태스크가 맡으므로 그대로다.

## 근거

- [KataRootMotionCurveComponent.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Animation/KataRootMotionCurveComponent.cpp): `ApplyMontageStages`, `ApplyApproachLimit`.
- [KataFL_Approach.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Movement/KataFL_Approach.cpp): 후보 선정과 도형 최근접점. Box는 선분과 상자 사이를 번갈아 투영해 구한다(볼록 도형끼리라 수렴한다).
- [KataTask_LimitApproach.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Tasks/KataTask_LimitApproach.cpp), [KataTask_AutoDash.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Tasks/KataTask_AutoDash.cpp).

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 전진 제한 기본 동작 | 사용자 빌드·PIE | 대상 앞에서 더 전진하지 않음. 처음 확인에서는 에디터가 새 태스크 클래스 없이 빌드된 상태였고 재빌드 후 동작했다 | — |
| 세부 경우 | — | — | 소프트락 Preset 후보, 오토 대시 병용, 큰 대상 부위, HurtBox 없는 대상, 옆으로 도는 공격, 구간 중 취소, AI 공격(사용자 보고 없음) |

## 남은 제한과 후속 작업

- 입력 이동과 Apply Root Motion 계열 이동은 제한하지 않는다. 필요하면 별도 이슈로 다룬다.
- 이미 겹친 대상에게서 밀어내는 기능은 없다.
- 오토 대시의 이동 거리 누적은 전진 제한으로 자르기 전 값이라, 함께 쓰면 Min Dash Distance가 실제보다 많이 간 것으로 센다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [연결 이슈 #38](https://github.com/jaykop/Kata/issues/38) | 종료 댓글 후 닫기, 본문의 깨진 Hit Trace 계획 링크 교체(게시는 사용자 확인 후) |
| [루트 모션 커브 사용법](../manual/Root-Motion-Curve.md#전진-제한) | 전진 제한 항목, 오토 대시 거리 기준, 확인 상태 |
| [Hit Trace 사용법](../manual/Hit-Trace.md) | HurtBox가 거리 기준으로도 쓰인다는 안내 |
| [오토 대시 결정 기록](2026-10-10-Auto-Dash.md) | 거리 기준 변경을 이 기록으로 연결 |
| 전진 제한 계획 | 이슈 종료로 삭제. 결정 이유는 이 기록으로 옮김 |
