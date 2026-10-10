# 오토 대시 계획

작성: 2026-10-10  
갱신: 2026-10-10  
연결 이슈: [#37 오토 대시](https://github.com/jaykop/Kata/issues/37)  
현재 상태 근거: [#37 방식 결정 댓글](https://github.com/jaykop/Kata/issues/37), [루트 모션 이동량 커브 결정 기록](../devlog/2026-10-10-Root-Motion-Curve.md), [루트 모션 커브 사용법](../manual/Root-Motion-Curve.md), [타게팅 사용법](../manual/Targeting.md)  
대체 관계: 없음. [액션 게임 기반 시스템 계획](Action-Game-Systems-Plan.md)의 "Motion Warping" 항목은 #37 결정으로 대체됐다

## 목적과 현재 상태

공격을 시작할 때 대상과의 거리에 맞춰 루트 모션 전진 거리를 늘이거나 줄여, 대상 앞에서 멈추게 한다.

- 이미 있는 것
  - `UKataRootMotionCurveComponent`(KataFramework)가 `UCharacterMovementComponent::ProcessRootMotionPreConvertToWorld`를 바인딩한다. `AKataCharacter`의 기본 서브오브젝트다.
  - `KataFL::ExtractMontageRootMotion`이 몽타주 트랙 구간의 루트 모션을 커브 또는 원래 루트 모션으로 만든다.
  - 대상은 Resolve Target Command가 Context Target에 넣는다. PC 락온 지점은 `UKataPlayerTargetingComponent::GetLockPoint()`다.
  - 회전은 Resolve Facing Command(즉시)와 `UKataTask_RotateToFacing`(구간)이 맡는다. 둘 다 KataTargeting이다.
- 없는 것
  - 이동 거리를 바꾸는 태스크와 처리. `EKataTaskPhase::Movement`를 쓰는 이동 태스크가 하나도 없다.
  - 대상 쪽 기준 지점을 돌려주는 타게팅 API. 기반 `UKataTargetingComponent`에는 방향(`ResolveFacingDirection`)만 있다.

### 진단 요점

- 컴포넌트는 `bUseRootMotionCurves`가 꺼져 있으면 처리 전체를 건너뛴다. 이 상태로는 커브를 쓰지 않는 캐릭터에서 거리 보정이 동작하지 않는다.
  바인딩 자체는 설정과 관계없이 `OnRegister`에서 한다.
- 델리게이트 입력은 메시 컴포넌트 공간의 이번 갱신 변화량이다(`USkeletalMeshComponent::ConvertLocalRootMotionToWorld`). 보정은 이 공간의 값을 바꿔 돌려준다.
- `KataFL::ExtractMontageRootMotion`은 시퀀스 커브가 있으면 항상 커브를 쓴다. 커브 대체를 끈 캐릭터의 남은 이동량을 원래 루트 모션으로 구하려면 커브를 쓰지 않는 선택지가 필요하다.
- 태스크 단계 순서는 Input → Movement → Gameplay → Animation이다. 같은 시각에 시작하면 오토 대시 태스크가 Play Montage보다 먼저 시작해 몽타주 인스턴스가 아직 없다.
- `UKataActionComponent`와 CharacterMovement는 둘 다 `TG_PrePhysics`라 같은 프레임의 Kata Tick과 루트 모션 처리 순서가 정해져 있지 않다.
- Kata 시계와 몽타주 시계는 지금은 같은 월드 DeltaTime으로 진행한다. 히트스톱 같은 시계 정책은 아직 없다.

## 범위

- 포함: 오토 대시 태스크, 컴포넌트 처리 단계 분리와 거리 보정 단계, 대상 기준 지점 API, 남은 이동량 계산 공용 함수 확장, 관련 manual·devlog.
- 제외: 전진 제한([#38](https://github.com/jaykop/Kata/issues/38), 같은 처리 체인의 다음 단계로 이 계획에서 자리만 둔다), 회전 보정(기존 회전 태스크가 맡는다), 제자리 애니메이션에 이동을 더하는 기능, 프리뷰의 가상 대상, 히트스톱 시계 정책.

## 처리 구조

컴포넌트의 `ProcessRootMotion`을 단계로 나눈다. 바인딩은 지금처럼 항상 하고, 단계마다 켜는 조건을 따로 둔다.

| 단계 | 켜는 조건 | 하는 일 |
|---|---|---|
| 1. 기준 이동 | `bUseRootMotionCurves`가 켜져 있으면 커브 대체, 아니면 엔진 값 그대로 | 이번 갱신의 이동량을 정한다(현재 구현) |
| 2. 거리 보정(#37) | 오토 대시 요청이 활성 상태일 때 | 남은 구간 이동량과 대상까지의 거리로 배율을 구해 이번 갱신의 수평 이동에 곱한다 |
| 3. 전진 제한(#38) | 전진 제한 요청이 활성 상태일 때 | 이 계획에서는 구현하지 않는다 |

공통 전제(Root Motion From Montages Only, 재생 중인 루트 모션 몽타주, 섹션 경로 복원 성공)를 만족하지 않는 갱신은 모든 단계를 건너뛰고 엔진 값을 쓴다.

### 거리 보정 계산

1. 창 고정: 요청 뒤 블렌드 아웃 중이 아닌 루트 모션 몽타주를 처음 만난 갱신에서 인스턴스 ID를 기록하고,
   남은 창 길이(트랙 시간) = 태스크 Duration × (인스턴스 재생 속도 × 몽타주 Rate Scale)로 정한다. 갱신마다 몽타주가 움직인 길이만큼 줄인다.
   남은 경로는 섹션 링크를 따라 이어 가고(현재 `BuildTrackRanges`와 같은 규칙), 다음 섹션이 없으면 몽타주가 끝나는 곳에서 자른다.
2. 남은 이동량 R: 이번 갱신 시작 위치부터 창 끝까지를 1단계와 같은 원천(커브 또는 원래 루트 모션)으로 추출하고 이동 배율(`GetAnimRootMotionTranslationScale`)을 곱한다. 메시 회전으로 월드 수평 벡터로 바꾼다.
3. 원하는 거리 d: 자신 위치 S에서 대상 기준 지점 P까지의 수평 거리 − 두 캡슐 반지름 − Stop Distance. 0보다 작으면 0이다.
4. 남은 전진량 r = R · (P − S)의 수평 단위 방향. r이 너무 작으면(전진하지 않는 구간) 이번 갱신은 보정하지 않는다.
5. 경로가 대상과 이루는 각 θ(cosθ = r / |R|)로 이동 거리 L = d × cosθ를 정한다. 경로 위에서 대상에 가장 가까워지는 지점까지다. 지금까지 창 안에서 보정해 이동한 수평 거리 T를 누적해 두고, L을 Min Dash Distance − T 이상, Max Dash Distance − T 이하(둘 다 0 이상)로 제한한다. 배율 s = 제한한 L / |R|. 처음 구현은 s = d / r로 정해 경로 전체에 곱했는데, 락온 중 몸이 대상에서 크게 틀어져 있으면 r이 작아 배율이 폭주해 엉뚱한 방향으로 멀리 돌진했다(2026-10-10 사용자 PIE). 그래서 경로 위 최근접점까지로 바꿨다. 이번 갱신 이동 중 창 안의 부분만 수평 성분에 s를 곱한다. 창 경계를 넘는 갱신은 구간을 나눠 창 밖 부분은 그대로 둔다.
6. 대상이 움직이면 다음 갱신에서 d를 다시 구한다. 매 갱신 남은 구간 전체로 배율을 다시 정하므로 창 끝에서 오차가 모이지 않는다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| Motion Warping | 확정 | 쓰지 않는다. 자체 구현([#37 댓글](https://github.com/jaykop/Kata/issues/37)) |
| 델리게이트 소유 | 확정 | `UKataRootMotionCurveComponent`가 단일 바인딩을 소유하고 커브 대체 → 거리 보정 → 전진 제한 순서로 처리 |
| 보정 구간 | 확정 | 몽타주 노티파이가 아니라 Kata 액션 타임라인 태스크의 구간 |
| 대상 | 확정 | KataTargeting의 대상(Context Target)과 락온 지점 |
| 활성화 구조 | 확정 | 위 "처리 구조"처럼 단계별 조건으로 나눈다. `bUseRootMotionCurves`는 1단계만 켜고 끈다 |
| D1 늘이고 줄이는 방식 | 확정 | 남은 이동의 수평 성분 전체에 같은 배율. 방향은 회전 태스크가 맡고 애니메이션 경로 모양이 유지된다. 검토한 대안: 남은 경로를 대상 쪽으로 돌리고 늘이는 워프식(정면이 대상을 향하지 않으면 옆으로 미끄러져 보인다) |
| D2 기준 지점 | 확정 | 락온 지점이 Context Target의 지점이면 그 위치, 아니면 대상 ActorLocation. 대상 위치를 쓰면 두 캡슐 표면 사이 간격, 락온 지점을 쓰면 자신의 캡슐 표면에서 지점까지의 거리로 잰다 |
| D3 기준 지점 API 위치 | 확정 | 기반 `UKataTargetingComponent`에 `ResolveApproachLocation(ActionTarget, OutLocation, bOutIsTargetPoint)`(BlueprintNativeEvent)을 더하고 PC 컴포넌트가 락온 지점으로 재정의한다. AI는 KataAI에서 재정의할 수 있다 |
| D4 전진 거리 한도 | 확정 | 2026-10-10 사용자 결정으로 배율 한도(Min Scale 0, Max Scale 3)를 절대 거리로 바꿨다. 구간 동안 대상 쪽 총 전진 거리를 Min Dash Distance(기본 0cm)~Max Dash Distance(기본 500cm)로 제한한다. 배율 한도는 애니메이션마다 원래 전진 거리가 달라 기획자가 실제 거리를 예측하기 어렵다 |
| D5 대상 추적 | 확정 | 매 갱신 거리를 다시 구한다(Track Target 기본 켬). 끄면 처음 구한 지점으로 고정한다 |
| D6 창 고정 방식 | 확정 | 첫 갱신에서 몽타주 인스턴스에 고정하고, 창을 남은 트랙 길이로 보관해 갱신마다 줄인다. 섹션 반복에서도 같은 위치를 두 번 지나는 문제가 없다. 검토한 대안: 태스크가 매 Tick 남은 Kata 시간을 넘김(Tick 순서에 따라 한 프레임 오차) |
| D7 제자리 애니메이션 | 확정 | 전진 루트 모션이 없는 구간은 보정하지 않는다. 이동을 더하는 기능은 필요하면 후속 이슈 |
| D8 이름 | 확정 | 태스크 `UKataTask_AutoDash`("Kata Task: Auto Dash", Movement 묶음, KataFramework). 컴포넌트 이름은 유지한다(기본 서브오브젝트 이름·BP 직렬화 영향 없음) |
| D9 manual | 확정 | [루트 모션 커브 사용법](../manual/Root-Motion-Curve.md)에 오토 대시 절을 더하고 타게팅 사용법에서 링크한다 |

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| A1 | 높음 | `KataFL::ExtractMontageRootMotion`에 시퀀스 커브 사용 여부 인자 추가, 컴포넌트 안의 섹션 경로 생성을 시작 위치·길이로 일반화 | D6 | 기존 커브 대체 동작이 같고, 커브를 끈 추출이 엔진 추출과 같은 값을 낸다 |
| A2 | 높음 | 컴포넌트 처리 단계 분리, 거리 보정 요청 시작·종료 API(핸들), 2단계 계산 | D1·D4·D5·D6, A1 | 커브 꺼짐에서도 요청이 있으면 보정한다. 요청이 없으면 기존과 같은 결과 |
| A3 | 높음 | 기준 지점 API | D2·D3 | PC는 락온 지점, 기반은 대상 위치를 돌려준다 |
| A4 | 높음 | `UKataTask_AutoDash` 정의·인스턴스 | A2·A3, D8 | 시작에 요청 등록, 완료·취소·중단·소유자 파괴에 해제. Single Frame·Duration 0은 설정 오류 |
| A5 | 보통 | manual·devlog, Action-Game-Systems-Plan의 Motion Warping 문구 정리 | A4, D9 | 문서가 현재 동작과 맞다 |

## 영향과 제한

- 모듈: 태스크와 보정은 KataFramework, 기준 지점 API는 KataTargeting(D3 권장안). KataFramework → KataTargeting 방향이라 의존 규칙에 맞는다. 새 엔진 플러그인 의존은 없다.
- 공개 API: `KataFL::ExtractMontageRootMotion` 인자가 늘어난다(기본값으로 기존 호출 유지). 컴포넌트에 요청 API가 생긴다.
- 실행 상태: 요청(대상·지점 약한 참조, 창, 몽타주 인스턴스 ID)은 컴포넌트가 갖는다. 태스크 정의에는 설정만 둔다. 요청은 한 번에 하나이며 새 요청이 이전 요청을 대체하고, 지난 핸들로 해제하면 무시한다.
- 수명: 태스크는 어떤 사유로 끝나도 해제한다. 컴포넌트 `OnUnregister`는 요청을 비운다. 대상이 파괴되면 보정을 멈추고 엔진 값을 쓴다.
- 에셋: 컴포넌트 이름을 유지하면 직렬화 영향이 없다. 바꾸면 클래스 Redirect와 기본 서브오브젝트 이름을 확인해야 한다(D8).
- 프리뷰: Context에 대상이 없으면 보정하지 않는다. 프리뷰 대상 구성은 [프리뷰 멀티 타겟 배치 보류 결정](../devlog/2026-10-10-Preview-Multi-Target-Waived.md)을 따른다.
- 몽타주 재생 중 다른 몽타주로 바뀌면(인스턴스 ID 불일치) 보정을 멈춘다.

## 사용자 확인 항목

- 구현 완료: A1~A4 코드와 A5 문서.
- 실행 확인(사용자): 커브 켬·끔 캐릭터 각각에서 먼 대상·가까운 대상·대상 없음, 락온 중 부위 지점, 움직이는 대상, 창이 몽타주 섹션 경계를 넘는 경우, 보정 중 액션 취소.
- 미확인으로 남을 범위: 재생 속도 변경, 히트스톱(시계 정책 미정), AI 캐릭터.

## 완료 시 갱신할 문서

- [연결 이슈](https://github.com/jaykop/Kata/issues/37): 구현·확인 상태.
- [루트 모션 커브 사용법](../manual/Root-Motion-Curve.md) 또는 새 manual(D9): 오토 대시 사용법·제한.
- [타게팅 사용법](../manual/Targeting.md): 기준 지점 API와 오토 대시 링크.
- devlog: 결정과 실제 결과. 이 plan은 이슈를 닫을 때 삭제한다.
- [액션 게임 기반 시스템 계획](Action-Game-Systems-Plan.md): Motion Warping 문구를 결정 결과로 바꾼다.
- [문서 목록](../README.md): 이 plan 링크 추가, 종료 시 제거.
