# 피격 반응 샘플과 좌우·이동 잠금·가산 변환 규칙

작성: 2026-10-10  
갱신: 2026-10-10  
유형: 결정 기록  
대상: KataFramework `UKataHitReactionAbility`, 샘플 흑기사 `Light`·`Flinch` 반응, DS3 가산 클립, [피격 반응 사용법](../manual/Hit-Reaction.md)  
기준: `Light`·좌우·이동 잠금은 `347b805`, 가산 변환·`Flinch`·방향 정의는 그 이후 작업 트리. `KataHitReactionAbility.h` 주석과 manual 갱신을 포함한다

## 배경과 결론

[#23](https://github.com/jaykop/Kata/issues/23) 계획의 HR7 중 `Light` 반응 샘플을 만들었다. 흑기사 Greatsword 피격 애니메이션 008000~008003으로 4방향 반응 Kata를 만들었다.
반응 GA 자식 `GA_BlackKnight_HitReaction_Light`를 플레이어와 보스의 Gameplay Data에 부여했다. 사용자 PIE에서 PC와 AI 모두 방향별로 경직했다.

작업 중 두 가지 규칙을 정했다.

- 좌우 피격 방향은 맞은 쪽 기준이다. 판정 코드는 처음부터 맞은 쪽 기준이었지만 주석과 manual이 공격자 시점으로 설명하고 있어 고쳤다.
- 반응 중 이동은 반응 애니메이션의 루트 모션으로 막는다. 이동을 막는 코드 장치는 추가하지 않았다.

이어서 HR6의 가산 변환과 HR7의 `Flinch` 샘플을 진행했다. 변환기가 기준 자세에 합성해 둔 가산 클립은 다시 변환하지 않고 UE 가산 설정만으로 원본 값을 복원했다.
흑기사 Sword 009000~009003으로 가산 몽타주 4개와 `GA_BlackKnight_HitReaction_Flinch`를 만들었고, 사용자 PIE에서 공격·이동을 유지한 채 흔들림만 더해졌다.
방향 enum의 뜻도 "공격이 들어온 쪽"으로 명시했다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| 좌우 설명 | `ResolveHitDirection`의 주석과 manual에 "왼쪽에서 오른쪽으로 베면 왼쪽 피격"이라고 적었다. 마주 선 공격자 시점으로 읽으면 칼은 맞은 쪽의 오른쪽에서 들어오므로 반대 결과가 된다 | 맞은 쪽 기준으로 다시 썼다. 계산은 맞은 쪽의 Right 벡터와 비교하므로 바꾸지 않았다 |
| 샘플 방향 매핑 | 에이전트가 008002를 Left, 008003을 Right로 추정했다. 이 추정도 공격자 시점이었다 | 사용자가 애니메이션을 보고 Left는 008003, Right는 008002로 정했다. Front는 008000, Back은 008001이다 |
| 반응 중 이동 | PC가 경직 중에도 이동 입력대로 움직였다. AI도 경로 이동 속도가 그대로 적용될 수 있다 | 피격 AnimSequence 008000~008003의 Enable Root Motion을 켰다. 사용자 PIE에서 이동이 막혔다 |
| 반응이 없는 피격 | 사용자가 "맞았는데 반응이 없는 경우"를 보고했다 | 로그상 모두 `Flinch` 판정이었다. 샘플에 `Flinch` Ability가 없어 정상 동작이다 |
| 방향의 의미 | 사용자가 `Left`가 왼쪽에서 맞은 것인지 왼쪽으로 휘청이는 것인지 물었다 | 공격이 들어온 쪽이다. `EKataHitDirection` 주석과 manual 방향 규칙 첫 줄에 적었다 |
| 가산 클립 | 변환기는 FBX가 가산 정보를 담지 못해 가산 원본(blendHint 2)을 `delta @ 기준 자세`로 합성한 `PoseBaked` 클립으로 냈다 | 복제본의 Additive Anim Type을 `Local Space`, Base Pose Type을 `Skeleton Reference Pose`로 두었다. 사용자 프리뷰에서 원본 `PoseBaked`와 같았다 |
| 가산 슬롯 | `ABP_CharacterBase`에는 `DefaultSlot` 하나뿐이었다. 스켈레톤 Slot Group은 MCP로 읽거나 쓸 수 없다 | 사용자가 `SK_BlackKnight`에 `AdditiveGroup`/`HitReactionAdditive`를 등록한 뒤, AnimGraph에 슬롯 노드를 `DefaultSlot`과 Output Pose 사이에 넣었다 |
| Flinch 중 공격 모션 이상 | 처음 등록한 Slot Group이 저장되지 않은 채 에디터가 재시작됐다. 그 사이 AnimGraph를 컴파일하면서 `HitReactionAdditive`가 `DefaultGroup`에 자동 등록되어, `Flinch` 몽타주가 공격 몽타주를 끊었다 | 사용자가 슬롯을 `AdditiveGroup`으로 옮기고 스켈레톤을 저장했다. 파일에 그룹이 들어간 것을 확인했고, 이후 공격 모션이 정상이었다 |
| 몇 번 뒤 반응 없음 | `Flinch` 확인 중 4콤보 뒤로 반응이 사라졌다 | 보스 체력이 0이 되어 판정 컴포넌트가 로그 없이 반환했다. 사망 처리가 없어 서 있는 채로 보였다. 정상 동작이다 |
| 몽타주 생성 | MCP로 기존 몽타주를 복제해 애니메이션을 바꾸면 `SequenceLength`가 원본 길이로 남는다 | 엔진은 이 값을 편집 중에 다시 계산하지 않고 로드할 때만 고친다. 몽타주는 에디터의 Create AnimMontage로 만들었다 |

## 주요 결정과 이유

**좌우는 맞은 쪽 기준이다.** 반응 애니메이션은 맞은 캐릭터가 어느 쪽에서 맞았는지를 표현하므로 방향도 맞은 쪽 몸을 기준으로 정의한다.
"왼쪽에서 오른쪽으로 벤다"처럼 주어가 없는 설명은 공격자 시점으로 읽히기 쉽다. 그래서 manual과 주석에 두 시점의 결과를 함께 적었다.

**이동 잠금은 루트 모션으로 한다.** Kata에는 실행 중 이동을 막는 장치가 없다. 공격과 회피가 멈춰 있던 것은 루트 모션 몽타주가 Character Movement의 속도를 덮기 때문이다.
이 규칙은 [입력 사용법](../manual/Input.md)에 이미 있다. 피격 애니메이션은 루트 이동량이 0이지만 루트 모션을 켜면 같은 방식으로 제자리에 선다.
PC 입력과 AI 경로 이동을 함께 막고, 이동 방향 회전도 기본 설정상 멈춘다. `Knock.Back` 후보(008140번대)는 원래 루트 이동이 있어 이후 단계도 같은 규칙을 쓴다.

채택하지 않은 대안은 반응 GA나 Kata 태스크가 이동을 직접 막는 코드 장치다. 루트 모션이 없는 반응에도 쓸 수 있지만 새 기능 설계가 필요하다.
계획의 "제어권 상실 태그"를 무엇이 읽을지도 함께 정해야 한다. 현재 샘플은 에셋 설정만으로 해결되므로 필요해질 때 다시 검토한다.

**가산 클립은 다시 변환하지 않고 UE 설정으로 복원한다.** 변환기의 합성은 `delta @ 기준 자세`이며, delta를 부모 공간에서 앞에 곱한다. DSAnimStudio의 행벡터 `reference * delta`와 같은 규칙이다.
이 기준 자세는 UE 스켈레톤을 만든 HKX 스켈레톤의 기준 자세와 같다. UE의 `Local Space` 가산은 회전 delta를 `대상 × 기준⁻¹`으로 구하고 `delta × 현재 자세`로 더한다.
그래서 `Skeleton Reference Pose`를 기준으로 하면 회전 delta가 그대로 복원되고 적용 규칙도 원본과 같다.
이동 성분은 UE가 차이를 그대로 더하고 원본 행렬 합성은 그 차이를 회전시킨다. 현재 자세의 뼈 로컬 이동이 기준 자세와 다를 때만 차이가 나고, 뼈 길이가 고정이라 작다.
FBX에 가산 정보를 따로 담는 변환기 수정은 필요하지 않았다.

**Slot Group을 먼저 등록한다.** 엔진은 등록되지 않은 슬롯을 `DefaultGroup`으로 다루고, AnimGraph 슬롯 노드는 컴파일 때 없는 슬롯을 `DefaultGroup`에 등록한다.
그러면 가산 몽타주가 공격 몽타주와 같은 그룹이 되어 공격을 멈춘다.

**`Light` 중에는 `Flinch`를 막는다(사용자 결정).** 판정은 Poise를 바로 채우므로 `Light` 직후 타격은 대부분 `Flinch`다. 경직 위에 흔들림이 겹치지 않도록 `Flinch` GA의 Activation Blocked Tags에 `Status.HitReaction.Light`를 넣었다.

## 근거

- [KataHitReactionAbility.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/HitReaction/KataHitReactionAbility.cpp): `ResolveHitDirection`이 들어온 방향을 맞은 쪽 `GetActorForwardVector`·`GetActorRightVector`와 비교한다.
- [입력 사용법](../manual/Input.md): 액션 실행 중에도 이동 입력이 적용되며 루트 모션 몽타주가 재생 중이면 몽타주가 이동을 덮는다.
- UE 5.8 `Skeleton.cpp`: `USkeleton::GetSlotGroupName`은 없는 슬롯을 `DefaultGroup`으로 돌려주고, `RegisterSlotNode`는 없는 슬롯을 `DefaultGroup`에 등록한다.
- 로컬 DS3 변환 도구(저장소 미포함)의 애니메이션 읽기: 가산 원본을 `delta @ rest`로 합성하고, `rest`는 아마추어를 만든 HKX 스켈레톤 기준 자세다.
- UE 5.8 `AnimMontage.cpp`: `UAnimMontage::PostLoad`만 `CalculateSequenceLength`로 길이를 고치며, `PostEditChangeProperty`는 다시 계산하지 않는다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| `Light` 반응 | 2026-10-10 사용자 PIE, 에이전트 로그 분석 | PC·보스 모두 방향별 경직, 공격 중단, AI 복귀, 끝부분 이동 캔슬, 연속 경직 재시작, 상태 태그 부착·제거 | 없음 |
| 반응 중 이동 | 사용자 PIE | 루트 모션을 켠 뒤 이동이 막힘 | AI 경로 이동 중 피격은 별도로 지정해 보지 않았다 |
| 주석 수정 | 소스 대조 | 주석만 바뀌었다 | 빌드는 하지 않았다 |
| 가산 변환 | 사용자 에디터 프리뷰 | 변환 클립을 기준 자세에 더한 모습이 원본 `PoseBaked`와 같음 | Greatsword 자세 위의 어색함은 별도로 보지 않았다 |
| `Flinch` 반응 | 사용자 PIE | 흔들림 재생, 공격·이동 유지, `Light` 중 차단, 사망 후 반응 없음 | 하이퍼아머 공격 샘플이 없다 |

## 남은 제한과 후속 작업

- 하이퍼아머 흔들림은 `PoiseDamageTakenMultiplier`를 낮추는 GE를 거는 샘플 공격이 없어 확인하지 못했다.
- 흑기사는 Greatsword용 가산 클립이 없어 Sword 클립을 쓴다. 은기사 등 다른 캐릭터의 가산 클립은 필요할 때 같은 설정으로 만든다.
- `Flinch` 재생 중 `Light`가 오면 남은 흔들림이 경직 위에 겹친다. 끊어야 하면 `Light` GA가 `Flinch`를 취소하게 한다.
- 계획의 "무적 회피 중 무피해" 확인은 샘플 회피에 `Status.Invincible` 설정이 없어 HR8의 기상 무적과 함께 다룬다.
- 8000번대가 0.67s·1.67s·2.33s 세 묶음이라 계획의 `Medium` 추가 조건에 해당한다. HR8 착수 때 정한다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#23](https://github.com/jaykop/Kata/issues/23) | HR7 `Light` 진행 댓글 게시. HR6·`Flinch` 결과는 게시 전 |
| [피격 반응 사용법](../manual/Hit-Reaction.md) | 좌우 기준과 방향 정의, 루트 모션 이동 잠금, 반응 중 입력, 가산 반응 준비, 문제 해결 행, 샘플 에셋과 확인 상태 |
| [피격 반응 계획](../plan/Hit-Reaction-Plan.md) | 영향 없음. 진행 상태는 이슈에서 관리한다 |
