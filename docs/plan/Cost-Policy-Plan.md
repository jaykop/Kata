# 액션 비용 정책 계획

작성: 2026-09-24  
갱신: 2026-09-24  
연결 이슈: [#9 액션 비용(Cost) 정책](https://github.com/jaykop/Kata/issues/9)  
현재 상태 근거: [현재 구현 상태](../devlog/Implementation-Status.md)  
대체 관계: 삭제한 Next-Work-Plan의 "보류한 항목 — Cost 정책" 절을 옮겼다.

## 목적과 현재 상태

`UKataAction`에 비용이 없어 쿨다운만 있는 비대칭 상태다. 착수 순서와 설계는 정했지만 아직 구현하지 않았다.
GAS의 비용·Attribute 계산을 중복 구현하지 않는다.

## 범위

- 포함: `UKataAction`의 비용 판정과 지불, 거절 사유 반환.
- 제외: 호출 Ability가 있는 경우의 비용 처리. 이때는 Ability가 이미 비용을 냈다고 본다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| 비용 설정 구조 | 확정 | `FKataCostPolicy { bEnabled, CostEffectClass, EffectLevel, TMap<FGameplayTag, float> CostMagnitudes }`. 값은 SetByCaller로 주입해 비용 값마다 GE 에셋을 만들지 않는다. |
| 판정 순서 | 확정 | 태그 → 조건 → 쿨다운 → 비용 판정 → 시작 → 비용 지불. `EKataStartResult::CostNotMet`을 추가해 거절 사유를 감추지 않는다. |
| 루프 회차 | 확정 | 루프 회차마다 비용을 다시 받지 않는다. 쿨다운과 같은 규칙이다. |
| 호출 Ability | 확정 | `Context.OwningAbility`가 있으면 Kata는 비용을 내지 않는다. |
| 판정 방식 | 확정 | `CanApplyAttributeModifiers`를 쓰지 않고 같은 스펙으로 판정과 지불을 수행한다. 아래 "엔진 제약" 참고. |
| GE 구성 | 결정 필요 | Attribute마다 GE 하나를 두고 값만 SetByCaller로 넣는 쪽을 권장한다. 아래 참고. |
| 비용 SetByCaller 태그 이름 | 결정 필요 | 태그 체계는 [게임플레이 태그 계획](Gameplay-Tag-Plan.md)으로 확정했다. 이름은 착수 시 정한다. |

### 엔진 제약: CanApplyAttributeModifiers를 쓰지 않는 이유

`GameplayEffect.cpp`의 `FActiveGameplayEffectsContainer::CanApplyAttributeModifiers`는 스펙을 함수 안에서
`(Def, Context, Level)`만으로 만들기 때문에 SetByCaller 값을 넣을 통로가 없다. 미설정 SetByCaller는
`GetSetByCallerMagnitude`의 기본값 0으로 평가되어 비용 판정이 항상 통과한다. 스톡 `UGameplayAbility::CheckCost`도
같은 함수를 쓰므로 같은 한계를 갖는다.

따라서 `KataGas`는 스펙을 한 번 만들어 SetByCaller를 채운 뒤 판정과 지불에 모두 사용한다.
판정은 엔진 함수 대신 같은 규칙(Additive 모디파이어만, `현재값 + 평가된 매그니튜드 < 0`이면 거절)을 직접 수행한다.
판정과 지불이 같은 스펙을 공유하므로 금액이 어긋나지 않는다.

### GE 구성 권장안

모디파이어의 Attribute는 에셋에 고정되므로 GE 하나로 임의의 Attribute를 다룰 수 없다.
여러 모디파이어를 가진 공용 GE 하나는 쓰지 않는 모디파이어까지 매번 값을 채워야 하고, 빠뜨리면 에러 로그가 남는다.
그래서 Attribute마다 GE 하나를 두는 쪽을 권장한다.

## 작업 순서와 완료 조건

| ID | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|
| C1 | GE 구성과 태그 이름 결정 | 없음 | 두 결정이 이 문서와 이슈에 기록됨 |
| C2 | `FKataCostPolicy`와 판정·지불 구현 | C1 | 비용 부족 시 `CostNotMet`으로 거절하고, 시작 후 한 번만 지불함 |

## 영향과 제한

- `UKataAction`에 새 직렬화 필드가 생긴다. 기본값은 비활성이라 기존 에셋의 동작은 바뀌지 않는다.
- 프리뷰 액터에 AttributeSet이 없으면 비용 판정은 프리뷰에서 의미가 없다.

## 완료 시 갱신할 문서

- [현재 구현 상태](../devlog/Implementation-Status.md): 비용 정책과 판정 순서.
- [런타임 사용법](../manual/Runtime-Usage.md): 비용 설정과 거절 사유.
- 결정 이유는 devlog로 옮기고 이 문서를 삭제한다.
