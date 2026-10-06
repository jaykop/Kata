# Gameplay Data의 Ability·Effect 부여와 Identity 태그

작성: 2026-10-06  
갱신: 2026-10-06  
유형: 구현 기록, 결정 기록  
대상: KataFramework `UKataGameplayData`, `FKataCharacterRow`, `UKataPreviewSetup_GameplayData`  
기준: #34 작업 트리(미커밋 상태에서 작성)

## 배경과 결론

[#34](https://github.com/jaykop/Kata/issues/34)는 캐릭터가 쓰는 GAS 데이터(Attribute·GA·GE)를 데이터 에셋으로 묶어 캐릭터 행에서 지정하는 작업이다.
에셋 타입과 Attributes 섹션은 [#40](https://github.com/jaykop/Kata/issues/40)에서 먼저 만들었다([기록](2026-10-05-Character-Attributes.md)).
이번에는 같은 에셋에 Granted Abilities·Effects를 추가하고, Identity 태그는 행에 두어 생성 시 함께 적용했다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| `UKataGameplayData` | Attributes 섹션만 있음 | `GrantedAbilities`(`FKataGrantedAbility{AbilityClass, Level}`), `GrantedEffects`(`FKataGrantedEffect{EffectClass, Level}`) 추가. 빈 클래스 항목은 데이터 검증 오류 |
| `ApplyAll` | 세트 추가와 초기값만 | 세트 → 초기값 → Effect → Ability → Identity 태그. `IdentityTags` 매개변수 추가 |
| `FKataGameplayDataHandles` | 추가한 세트만 기록 | Ability·Effect 핸들과 부여한 태그도 기록 |
| `FKataCharacterRow` | — | `IdentityTags`(카테고리 `Gameplay`, 선택기 `Identity` 루트) |
| 프리뷰 셋업 | Gameplay Data만 | Identity Tags 항목 추가 |
| 문서 | Gameplay Data를 `Attributes.md`에서 설명 | [Gameplay Data 사용법](../manual/Gameplay-Data.md)으로 분리 |
| 샘플 | `GE_StaminaRegen` 적용 경로 없음 | BlackKnight·StarvedHound Gameplay Data의 Granted Effects에 할당. 테스트용 `GE_Test_StaminaDrain` 추가 |

## 주요 결정과 이유

사용자 확정(2026-10-06):

- **Identity 태그는 공유 에셋이 아니라 행에 둔다.** Gameplay Data는 여러 캐릭터가 공유하고 조합하는 단위다. 정체성 태그를 공유 에셋에 넣으면 의도하지 않은 캐릭터까지 같은 특성을 갖게 된다.
  Faction도 캐릭터 쪽(타게팅 컴포넌트)이 가진다. 행에서는 `GameplayData`와 같은 `Gameplay` 카테고리에 묶는다.
- **태그 루트는 `Identity`만 허용한다.** ASC에는 `Status`·`Identity` 루트만 넣는 규칙이 있고, 생성 시 고정으로 붙는 태그는 `Identity`의 정의에 해당한다.
- **Ability에 Input ID를 두지 않는다.** 입력 연결은 Kata 입력 계층이 맡는다.
- **회수 함수는 만들지 않는다.** 생성 시 한 번 적용하는 지금 흐름에는 쓰는 곳이 없다. 핸들은 기록해 두고 보스 페이즈 교체처럼 필요해질 때 만든다.
- **하드 참조를 쓴다.** Gameplay Data가 캐릭터 생성 전에 비동기로 로드되므로 참조 클래스도 함께 로드된다. 장비처럼 소프트 참조를 따로 로드할 필요가 없다.

구현 세부:

- Effect를 Ability보다, 초기값을 Effect보다 먼저 적용한다. 회복 GE처럼 Attribute를 참조하는 Effect가 초기값이 들어간 상태에서 시작한다.
- 행의 태그도 `ApplyAll` 매개변수로 받아 적용 순서를 한 함수에 둔다.
- Ability Spec과 Effect Context의 Source Object는 해당 Gameplay Data다. 디버거에서 출처를 구분할 수 있다.

초기화를 GE로 하지 않는 이유도 이 작업 중 확인했다. 엔진의 `InitStats`·`AttributeSetInitter`도 Base를 직접 설정한다. 싱글플레이라 GE 방식의 복제 이점이 없다. 레벨별 커브 테이블은 초기값의 `FScalableFloat`로 연결할 수 있으며, 레벨 출처는 후속 결정이다.

## 근거

- [KataGameplayData.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataGameplayData.cpp): 적용 순서, Effect·Ability 부여, 데이터 검증.
- [KataCharacter.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataCharacter.cpp): 행의 Identity 태그 보관과 적용.
- 엔진 `AbilitySystemCheatManagerExtension.cpp`: `AbilitySystem.Effect.Apply`가 플레이어 폰 ASC에 GE를 적용한다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| Editor 빌드 | 2026-10-06 사용자 빌드 | 통과 | Game 타깃 |
| Granted Effects | 2026-10-06 사용자 PIE. `GE_Test_StaminaDrain`으로 Stamina를 50 낮춤 | `GE_StaminaRegen`으로 Stamina가 다시 올라감 | NPC(StarvedHound) 쪽 |
| 샘플 에셋 설정 | MCP로 설정 후 값 재조회 | 일치, 저장 완료 | — |
| Granted Abilities, Identity Tags, 프리뷰 셋업 | 없음 | — | 전부 미확인 |

## 남은 제한과 후속 작업

- 부여 항목의 회수 함수가 없다.
- 샘플에 Granted Ability와 Identity 태그 예가 없다.
- 회복 정지 태그(Ongoing Tag Requirements)는 아직 정하지 않았다. 스태미나 소모는 [#9](https://github.com/jaykop/Kata/issues/9)에서 다룬다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#34](https://github.com/jaykop/Kata/issues/34) | 결과 댓글·라벨은 사용자 확인 후 게시 |
| [Gameplay Data 사용법](../manual/Gameplay-Data.md) | 신규. 테스트 방법 포함 |
| [Attribute 사용법](../manual/Attributes.md) | Gameplay Data 절을 링크로 대체, 회복 GE 확인 기록 |
| [캐릭터 데이터 테이블 사용법](../manual/Character-Data.md) | Identity Tags 행 설명 |
| [게임플레이 태그 사용법](../manual/Gameplay-Tags.md) | `Identity` 루트 사용처 |
| [캐릭터 정의 계획](../plan/Character-Definition-Plan.md) | 영향 없음. GAS 데이터를 후속으로 둔 당시 결정은 그대로 두고 이 기록에서 결과를 잇는다 |
