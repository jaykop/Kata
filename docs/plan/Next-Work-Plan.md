# 다음 작업 계획 — Kata 에셋 에디터 이후

갱신: 2026-09-23

이 문서는 기존 Blueprint 클래스 중심 구현 계획을 대체한다.
현재 소스 상태는 [Implementation-Status.md](../devlog/Implementation-Status.md), 조작 방법은
[Editor-Usage.md](../manual/Editor-Usage.md)와 [Runtime-Usage.md](../manual/Runtime-Usage.md)를 따른다.

## 유지할 구조

UKataAction(전용 객체 uasset) → UKataResolvedAction(병합된 설정) → UKataActionInstance(실행 상태).
원본은 ParentAction과 프로퍼티 변경분으로 상속한다.
편집기는 Preview·Timeline·Kata Action Details·Timeline Details를 제공한다.
Blueprint/CDO 기반 UKataDefinition 경로는 제거했다. 이를 다시 도입하지 않는다.

## 우선순위

1. 사용자가 실제 편집 중 발견한 문제를 우선 수정한다.
   에이전트는 명시적인 요청 없이는 테스트·빌드·별도 검사·리뷰를 수행하지 않는다.
   프리뷰 재생·Idle 애니메이션·캐릭터 착지·루트 모션 이동·Select Self/Target 조작은 사용자가 확인했다.
2. 실제 편집 피드백에 따라 타임라인 조작을 확장한다.
   다중 선택, 복사·붙여넣기, 태스크 표시 색·주석, 편집기 전용 트랙 그룹은 구현했다.
   다음 후보는 의존성 대상 선택 UI와 프리뷰 캐릭터 설정 편의성이다.
3. 프로젝트에서 필요한 기본 태스크를 추가한다.
   후보는 Gameplay Effect, 게임플레이 이벤트, 히트 판정, VFX/SFX다. 태스크별 자원 회수와 프리뷰 실행 특성을 함께 설계한다.
4. 콤보 그래프 에셋 KataGraph에 고유 타입을 채운다.
   UKataNode, UKataEntryNode, UKataEdge, UKataTask_TransitionWindow와 UKataGraphInstance/UKataGraphComponent 실행 연결을 구현했다.
   전이는 입력 직접 수신이 아니라 트리거 이벤트 태그로 받는다. 수용 구간은 액션 타임라인의 창 태스크가 연다.
   트리거 버퍼는 1단계에서 끄고(도착 프레임에만 유효) 실제로 눌러본 뒤 켠다.
   SubGraph·Conduit·Alias는 현재 표현력으로 부족한 실제 그래프 사례가 확인될 때 다시 검토한다.

위 후보 전체를 자동으로 구현하라는 지시는 아니다. 다음 요청의 범위에 맞춰 진행한다.
AfterMeshPose, 다중 액션 채널, 네트워크·예측, KataAI는 현재 범위 밖이다.
월드 실행 Subsystem은 인스턴스 순차 진행까지 구현했으며 Gather/Commit 다단계 실행은 후속 필요가 생길 때 확장한다.

## 보류한 항목

착수 순서와 설계는 정했지만 아직 구현하지 않았다. 재개할 때 아래 결정 사항을 그대로 따른다.

### Cost 정책

`UKataAction`에 비용이 없어 쿨다운만 있는 비대칭 상태다. GAS의 비용·Attribute 계산을 중복 구현하지 않는다.

- `FKataCostPolicy { bEnabled, CostEffectClass, EffectLevel, TMap<FGameplayTag, float> CostMagnitudes }`를 추가한다.
  값은 SetByCaller로 주입해 비용 값마다 GE 에셋을 만들지 않는다.
- 순서 계약: 태그 → 조건 → 쿨다운 → 비용 판정 → 시작 → 비용 지불. `EKataStartResult::CostNotMet`을 추가해
  거절 사유를 감추지 않는다.
- 루프 회차마다 비용을 다시 받지 않는다. 쿨다운과 같은 규칙이다.
- `Context.OwningAbility`가 있으면 호출 Ability가 이미 비용을 냈다고 보고 Kata는 내지 않는다.

**`UAbilitySystemComponent::CanApplyAttributeModifiers`를 판정에 쓰지 않는다.**
`GameplayEffect.cpp`의 `FActiveGameplayEffectsContainer::CanApplyAttributeModifiers`는 스펙을 함수 안에서
`(Def, Context, Level)`만으로 만들기 때문에 SetByCaller 값을 넣을 통로가 없다. 미설정 SetByCaller는
`GetSetByCallerMagnitude`의 기본값 0으로 평가되어 비용 판정이 항상 통과한다. 스톡 `UGameplayAbility::CheckCost`도
같은 함수를 쓰므로 같은 한계를 갖는다.

따라서 `KataGas`는 스펙을 한 번 만들어 SetByCaller를 채운 뒤 판정과 지불에 모두 사용한다.
판정은 엔진 함수 대신 같은 규칙(Additive 모디파이어만, `현재값 + 평가된 매그니튜드 < 0`이면 거절)을 직접 수행한다.
판정과 지불이 같은 스펙을 공유하므로 금액이 어긋나지 않는다.

착수 전 확정할 것 두 가지가 남아 있다.

- GE 구성: Attribute마다 GE 하나를 두고 값만 SetByCaller로 넣는 쪽을 권장한다.
  모디파이어의 Attribute는 에셋에 고정되므로 GE 하나로 임의의 Attribute를 다룰 수 없다.
  여러 모디파이어를 가진 공용 GE 하나는 쓰지 않는 모디파이어까지 매번 값을 채워야 하고, 빠뜨리면 에러 로그가 남는다.
- 비용 SetByCaller 태그의 이름 규약. `docs/plan/Kata.md` 4번(기본 태그 구조)과 맞물린다.

### 멀티 타겟 액터 배치

프리뷰에 Target을 하나만 배치할 수 있다.

- `PreviewTargetClass`·`PreviewTargetTransform` 단일 쌍을 `TArray<FKataPreviewActorSpec>`으로 교체한다.
  기존 단일 프로퍼티는 Redirect 또는 `PostLoad` 승격으로 옮긴다.
- `FKataContext::TargetActor`는 단일 필드로 유지한다. 배열의 Primary만 Context에 넣고 나머지는
  히트 판정·거리 조건 검증용으로만 존재하게 한다. 런타임 다중 타겟은 별도 설계이므로 프리뷰 요구로 끌어들이지 않는다.
- 조작은 이미 `EKataPreviewActorSlot`으로 일반화해 두었다. 자리를 `Target` 하나에서 `Target[i]`로 넓히면 되고
  뷰포트 클라이언트의 위젯·입력·커밋 경로는 바뀌지 않는다. `GetActorForSlot`과 `ApplyPreviewActorTransform`의
  분기만 배열 인덱스로 확장한다.

## 공용 작업 규칙

- 모든 작업은 ProjectKata 루트에서 한다.
- AGENTS.md를 따른다. 주석은 한국어, DisplayName·Category·ToolTip·식별자·진단 문자열은 영어다.
- 테스트 및 스트레스 테스트 코드를 추가하지 않는다. 검증은 사람이 담당한다.
- 작업 중인 기존 콘텐츠와 미커밋 변경을 보존한다.
- 변경 후 실제 구현 상태 문서를 갱신하고, 검증하지 않은 내용을 검증 완료로 보고하지 않는다.
