# 다음 작업 계획 — Kata 에셋 에디터 이후

갱신: 2026-09-24

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
   Gameplay Event·Gameplay Effect·Loose Tag 태스크는 구현했다. 다음 대상은 히트 판정(Hit Trace)이다.
   그 선행 작업으로 [게임플레이 태그 체계와 컴포넌트 태그 계획](Gameplay-Tag-Plan.md)의 태그 생성(T01·T02)을 완료했다.
   컴포넌트 태그 작업(T03~)은 플러그인·모듈 정리 이후로 보류했다.
4. 콤보 그래프 에셋 KataGraph에 고유 타입을 채운다.
   UKataNode, UKataEntryNode, UKataEdge, UKataTask_TransitionWindow와 UKataGraphInstance/UKataGraphComponent 실행 연결을 구현했다.
   전이는 입력 직접 수신이 아니라 트리거 이벤트 태그로 받는다. 수용 구간은 액션 타임라인의 창 태스크가 연다.
   트리거 버퍼는 1단계에서 끄고(도착 프레임에만 유효) 실제로 눌러본 뒤 켠다.
   SubGraph·Conduit·Alias는 현재 표현력으로 부족한 실제 그래프 사례가 확인될 때 다시 검토한다.

위 후보 전체를 자동으로 구현하라는 지시는 아니다. 다음 요청의 범위에 맞춰 진행한다.
AfterMeshPose, 다중 액션 채널, 네트워크·예측, KataAI는 현재 범위 밖이다.
월드 실행 Subsystem은 인스턴스 순차 진행까지 구현했으며 Gather/Commit 다단계 실행은 후속 필요가 생길 때 확장한다.

## Play Montage 포즈 탐색 수정 — 확인 완료

2026-09-24 사용자 요청으로 현재 미커밋 코드를 포함해 UE 5.8 몽타주 에디터와 소스를 비교했다.
현재 월드 재실행·Next Section 순회·활성 몽타주 보정 방식은 임의 시각의 포즈 탐색 요구와 다르다.
[진단 기록](../devlog/2026-09-24-Montage-Scrub-Diagnosis.md)과
[수정 계획](Montage-Scrub-Plan.md)에 따라 UAnimPreviewInstance 직접 위치 평가를 구현했다.
태스크 구간·겹침·일반 재생 복귀 정책과 실제 수정 범위는
[구현 기록](../devlog/2026-09-24-Montage-Scrub-Implementation.md)을 따른다.
빌드·UI 확인은 미실시다.
사용자의 루트 모션 이동 누락·일시 정지 뒤 재시작 보고에 따라, 엔진 루트 모션 추출을 이용한 메시 이동과
Current Time의 동일 값 확정 방어를 추가했다.
이후 탐색은 [실행 시뮬레이션 방식](../devlog/2026-09-24-Preview-Scrub-Simulation.md)으로 바뀌었고,
2026-09-24 사용자가 실행 확인을 마치고 푸시했다. 위 "빌드·UI 확인은 미실시"는 당시 기록이다.

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
- 비용 SetByCaller 태그의 이름 규약. 태그 체계는 [게임플레이 태그 계획](Gameplay-Tag-Plan.md)으로 확정했고, 이름은 Cost 재개 시 정한다.

### 멀티 타겟 액터 배치

프리뷰에 Target을 하나만 배치할 수 있다.

- `PreviewTargetClass`·`PreviewTargetTransform` 단일 쌍을 `TArray<FKataPreviewActorSpec>`으로 교체한다.
  기존 단일 프로퍼티는 Redirect 또는 `PostLoad` 승격으로 옮긴다.
- `FKataContext::TargetActor`는 단일 필드로 유지한다. 배열의 Primary만 Context에 넣고 나머지는
  히트 판정·거리 조건 검증용으로만 존재하게 한다. 런타임 다중 타겟은 별도 설계이므로 프리뷰 요구로 끌어들이지 않는다.
- 조작은 이미 `EKataPreviewActorSlot`으로 일반화해 두었다. 자리를 `Target` 하나에서 `Target[i]`로 넓히면 되고
  뷰포트 클라이언트의 위젯·입력·커밋 경로는 바뀌지 않는다. `GetActorForSlot`과 `ApplyPreviewActorTransform`의
  분기만 배열 인덱스로 확장한다.

## 문서 정비 후보

문서 템플릿과 작성·동기화 지침은 추가했다. 기존 기능 문서의 정비는 아직 수행하지 않았다.
근거와 완료 조건은 [문서 부채와 분류 진단](../devlog/2026-09-24-Documentation-Diagnosis.md)을 따른다.
이번 갱신은 문서 정비 후보를 연결한 것이며, 위 기능 계획의 모든 후보가 아직 미구현이라는 뜻은 아니다.

| 순서 | 진단 항목 | 정비 범위 | 상태 |
|---|---|---|---|
| 1 | D01~D03 | 루트 소개·모듈 구조, 런타임 예제·기본 태스크, 현재 에디터 조작 안내 | 미착수 |
| 2 | D04~D06 | 이 문서와 세부 계획의 완료·보류 상태, 요청 메모의 결과 연결 | 미착수 |
| 3 | D07~D10 | 조건 공용 함수 사용법, 에셋 이전 안내, 결정·확장 계약, 확인 이력 | 미착수 |
| 완료 | D11 | 세 템플릿·AGENTS 작성 규칙·문서 목록 | 문서 반영 완료 |

문서 정비는 새로운 기능 구현이나 실행 검증을 자동으로 포함하지 않는다. 후속 요청 범위에 맞춰 진행한다.
설명서와 devlog의 구체적인 작성 대상·기존 문서 보강 범위는
[진단의 작성 대상 선별](../devlog/2026-09-24-Documentation-Diagnosis.md#작성-대상-선별)을 따른다.

## 공용 작업 규칙

- 모든 작업은 ProjectKata 루트에서 한다.
- AGENTS.md를 따른다. 주석은 한국어, DisplayName·Category·ToolTip·식별자·진단 문자열은 영어다.
- 테스트 및 스트레스 테스트 코드를 추가하지 않는다. 검증은 사람이 담당한다.
- 작업 중인 기존 콘텐츠와 미커밋 변경을 보존한다.
- 변경 후 실제 구현 상태 문서를 갱신하고, 검증하지 않은 내용을 검증 완료로 보고하지 않는다.
