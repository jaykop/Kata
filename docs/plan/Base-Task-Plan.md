# 기본 태스크 확장 계획

작성: 2026-09-21
갱신: 2026-09-25

KataAction 요청 메모(2026-09-24 삭제)의 1번 항목("추가로 필요한 기본 태스크가 뭐가 있을까")에 대한 검토 결과다.
현재 구현 상태는 [Implementation-Status.md](../devlog/Implementation-Status.md)를 따르며,
기본 태스크 이슈 [#6 Hit Trace](https://github.com/jaykop/Kata/issues/6)와 [#7 VFX/SFX](https://github.com/jaykop/Kata/issues/7)의 설계 참고 문서다.

이 문서는 검토와 제안이다. 여기 적힌 항목을 전부 구현하라는 지시가 아니며,
착수 범위는 사용자가 항목별로 지정한다.

## 검토 시점의 전제

아래는 최초 검토 당시의 전제다. 현재 제공 태스크는 Play Montage·Transition Window·Send Gameplay Event·
Apply Gameplay Effect·Apply Loose Tag이며 [사용법](../manual/Runtime-Usage.md#기본-태스크)과
[실제 채택한 결정](../devlog/2026-09-25-GAS-and-Tasks.md)을 따른다.
현재 대상 선택은 EKataTaskTargetSource다. 이하 FKataTargetSpec·출력 채널·프리뷰 정책은 확장 제안이며,
기존 기본 태스크가 이를 이미 사용한다는 뜻은 아니다. 진행 상태는 연결 이슈에서 관리한다.

- 구현된 태스크는 `UKataTask_PlayMontage`와 `UKataTask_TransitionWindow` 두 개다.
- `FKataContext`는 Owner·Avatar·Target 각 하나와 `AbilitySystem`, `OwningAbility`를 가진다.
- `EKataTaskPhase`에는 `Movement`와 `Presentation`이 이미 정의되어 있다.
- 프리뷰 뷰포트는 프리뷰 액터와 타깃 액터에 ASC를 준비하고 `InitAbilityActorInfo`까지 호출한다.
  다만 프리뷰 액터 클래스가 AttributeSet을 갖고 있지 않으면 Attribute를 다루는 GE는 프리뷰에서 의미가 없다.

## 1. 태스크를 늘리기 전에 정할 공통 축

후보 태스크 대부분이 공통으로 요구하지만 아직 없는 구조가 둘 있다.
이 둘을 먼저 정하지 않고 태스크만 추가하면 나중에 전부 다시 손봐야 한다.

### 1.1 대상 지정 규약

`FKataContext`의 대상은 `TargetActor` 하나뿐이다. 이 상태로 태스크를 여러 개 추가하면
태스크마다 자기 방식의 대상 필드(`bApplyToSelf`, `AttachSocket`, `TargetOverride` 등)를 따로 만들게 된다.

공용 `FKataTargetSpec`을 정의하고 모든 태스크가 이를 사용하도록 한다. 필요한 선택지는 다음과 같다.

- Self(Owner)
- Avatar
- Context Target
- 부착 소켓 지정
- 선행 태스크의 출력 대상

### 1.2 태스크 출력 채널

히트 판정이 찾아낸 대상을 GE 적용 태스크와 연출 태스크가 받아야 하지만 현재 태스크 간 값 전달 경로가 없다.

`FKataTaskDependency`로 선후 관계는 이미 표현하므로, 의존성을 따라
`TaskId → FGameplayAbilityTargetDataHandle`을 읽는 형태가 자연스러운 확장이다.
저장 위치는 공유 에셋이 아니라 `UKataActionInstance`다.

### 1.3 프리뷰 정책

태스크가 실제 월드에 영향을 주기 시작하면 프리뷰에서의 동작을 태스크 자신이 선언해야 한다.
`UKataTask`에 "프리뷰에서 실행 / 건너뜀 / 시각화만"을 구분하는 가상 함수를 둔다.
태스크를 여러 개 추가한 뒤에 넣으면 그만큼 여러 곳을 고치게 된다.

## 2. Apply Gameplay Effect와 Apply Gameplay Tag는 나눈다

요청 메모의 "위의 2개는 꼭 나누어야 할까?"에 대한 결론은 **나눈다**이다.
"GE로도 태그를 붙일 수 있다"는 사실이 오히려 나누는 근거가 된다.

- 태그 하나를 붙이려고 GE 에셋을 만들게 하면 프로퍼티 하나짜리 에셋이 계속 늘어난다.
  Loose Tag는 인라인 데이터로 끝나는 일이므로, GE를 강제하면 에셋 증가와 불필요한 Effect 파이프라인 통과를 함께 떠안는다.
- 수명 정리 방식이 다르다. Duration GE는 `FActiveGameplayEffectHandle`을 인스턴스가 보관했다가 종료 시 제거해야 하고,
  Loose Tag는 참조 카운트 기준으로 Add와 Remove의 짝을 맞춰야 한다.
  한 태스크로 합치면 인스턴스가 두 종류의 정리 상태를 들고, 설정은 `EditCondition` 양자택일이 된다.
- 의미가 다르다. GE는 Attribute와 상태에 영향을 주는 게임플레이 변경이고,
  Loose Tag는 "이 구간 동안 이 상태다"라는 타임라인 마킹에 가깝다. 후자는 Kata 타임라인의 고유 기능에 속한다.

### GE 태스크의 두 시계 문제

GE 태스크에는 몽타주와 같은 두 시계 문제가 있다. GE 자체의 Duration과 태스크 구간이 어긋날 수 있다.
`EKataMontageEndPolicy`와 같은 결의 정책 프로퍼티가 필요하다.

- GE 자체 Duration을 따른다.
- 태스크 구간이 끝나면 제거한다.

Instant GE는 Duration 0의 순간 태스크로 두면 된다.

## 3. Camera Shake / Spawn VFX / Play Sound는 개별 태스크로 두고 Cue는 선택지로 추가한다

요청 메모의 "Cue로 실행하거나 묶음으로 별도 관리해야 할까?"에 대한 결론이다.

### 셋을 하나로 묶지 않는다

프로퍼티가 거의 겹치지 않는다. VFX는 부착 소켓·오프셋·스케일, 사운드는 볼륨·피치·감쇠·페이드,
셰이크는 진폭·에픽센터·감쇠 반경을 가진다.

정리 방식도 다르다. 나이아가라 컴포넌트는 "남은 파티클을 마저 재생" 옵션이 필요하고,
오디오 컴포넌트는 페이드 아웃, 카메라 셰이크는 블렌드 정지가 필요하다.
하나로 묶으면 서로 무관한 세 섹션과 세 가지 수명을 가진 태스크가 나온다.

### GameplayCue를 기본 경로로 강제하지 않는다

- Cue의 가장 큰 장점인 복제와 멀티캐스트는 싱글플레이 전용 프로젝트에서 가치가 없다.
  남는 것은 "태그 하나로 묶음 재생"이라는 간접 참조뿐이다.
- 그 간접 참조 때문에 타임라인 편집 경험이 나빠진다. Kata 에디터의 강점은 구간을 눈으로 보고 스크럽하는 것인데,
  Cue는 GameplayCueManager를 거쳐 나가므로 타임라인이 그 수명을 알지 못한다.
  직접 스폰 태스크는 컴포넌트 핸들을 보관하므로 구간 종료에 맞춰 정확히 회수된다.
- 프리뷰에서 ASC는 준비되므로 Cue가 원천 차단되지는 않는다.
  다만 경로가 하나 더 끼면 "왜 프리뷰에서 재생되지 않는가"를 확인할 지점이 늘어난다.

### 구성

개별 태스크 세 개를 기본 프리미티브로 두고, `UKataTask_GameplayCue`를 얇게 하나 더 추가한다.
Cue 태스크는 Burst(Execute)와 Looping(Add·Remove, 핸들 보관) 두 모드만 가진다.
GE 쪽에서 이미 사용하는 Cue 라이브러리가 프로젝트에 생기면 재사용 통로가 되고, 없으면 쓰지 않아도 된다.

여러 액션이 같은 연출 묶음을 공유하는 문제의 답은 Cue가 아니라 ParentAction 상속이며,
그것으로 부족하면 공유 태스크 묶음(프리셋)을 나중에 검토한다.

## 4. 태스크 우선순위

| 순위 | 태스크 | 근거 |
| --- | --- | --- |
| 1 | Send Gameplay Event | 구현 비용이 거의 없는데 레버리지가 가장 크다. 프로젝트 고유 동작이 전부 여기로 빠지므로 새 C++ 태스크를 만들 이유가 줄어든다. |
| 2 | Hit Trace(히트 판정) | 없으면 액션 게임이 성립하지 않는다. 설계 결정도 가장 많다. |
| 3 | Apply Gameplay Effect | 2절 참조. |
| 4 | Apply Loose Tag | 2절 참조. |
| 5 | 루트 모션·이동 | 대시와 전진 공격에 필수다. `EKataTaskPhase::Movement`가 이미 정의된 자리다. |
| 6 | Face Target(회전 보정) | 구간 동안 최대 속도를 제한하며 대상 쪽으로 회전한다. 흔하지만 GE로는 대체할 수 없다. |
| 7 | Camera Shake / Spawn VFX / Play Sound / Gameplay Cue | 3절 참조. |
| 8 | Hit Stop / Time Dilation | 타격감에 직결되지만 Kata 시계 자체에 영향을 준다. 시계 정책을 먼저 정해야 안전하다. |

2026-09-24 기준 1·3·4순위(Send Gameplay Event, Apply Gameplay Effect, Apply Loose Tag)는 구현했다.
2순위 Hit Trace는 [#6](https://github.com/jaykop/Kata/issues/6)에서 진행한다. 태스크는 KataFramework에 두고 판정 기준 메시는 캐릭터가 제공한다([결정 기록](../devlog/2026-09-24-Gameplay-Tag-Generation.md#후속-결정-컴포넌트-태그-작업-취소와-distance-조건-단순화)).

### Hit Trace의 설계 요점

- 구간 동안 이전 프레임에서 현재 프레임까지 보간해 스윕한다.
- 대상당 1회 필터를 둔다. 히트 목록은 태스크 시작 시 초기화하며 `UKataTaskInstance`에 보관한다.
- 대상 필터는 기존 `UKataFL_Condition`과 KataConditions를 재사용한다.
- 이 태스크는 데미지를 결정하지 않는다. 찾은 대상을 태스크 출력과 Gameplay Event로 내보내고,
  수치는 GE와 Ability가 가진다. 그래야 Kata 에셋에 데미지 값이 들어가지 않는다.

### 루트 모션·이동의 선행 결정

Motion Warping 플러그인을 도입할지, `ApplyRootMotionConstantForce` 계열 AbilityTask를 감쌀지를 먼저 정한다.

## 5. 현재 미루는 것

- Spawn Actor / Projectile: 당분간 Send Gameplay Event로 충분하다. 필요가 확인되면 클래스와 트랜스폼만 받는 단순한 형태로 추가한다.
- 무기 콜리전 토글, 부착·탈착: 프로젝트 고유 동작이므로 Gameplay Event로 처리한다.
- 디버그 드로우: 범용 디버그 드로우 태스크는 만들지 않는다. 각 기능의 디버그 시각화는 그 기능을 구현한 모듈에 둔다
  (2026-09-25 정책 변경, [AGENTS.md](../../AGENTS.md#모듈-경계). 예: [Hit Trace 계획](Hit-Trace-Plan.md#디버그-시각화)).
- 추가 애니메이션 태스크(레이어드 애디티브, 블렌드 스페이스 등): Play Montage로 충분한지 먼저 확인한다.
- 입력 소비·버퍼: 전이 창과 트리거 이벤트만 있으며 현재 입력 저장·재평가 버퍼는 없다. 후속 범위는 [#8](https://github.com/jaykop/Kata/issues/8)이다.

## 6. 착수 전에 확정할 질문

1. Hit Trace의 대상 필터를 KataConditions 재사용으로 갈지, 태스크 전용 필터를 둘지.
2. 이동 태스크의 기반 — Motion Warping 플러그인 도입 여부.
3. Kata 타임라인이 Time Dilation의 영향을 받는지.

이 질문들은 남은 Hit Trace·이동·시간 정책의 설계에 참고한다. 이미 구현한 태스크를 다시 착수 대상으로 삼지 않는다.
