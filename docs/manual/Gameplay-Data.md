# Gameplay Data 사용법

갱신: 2026-10-06  
대상: KataFramework의 `UKataGameplayData`, 캐릭터 행의 Gameplay Data·Identity Tags, Gameplay Data 프리뷰 셋업  
적용 기준: [#40 캐릭터 스탯 Attribute](https://github.com/jaykop/Kata/issues/40), [#34 캐릭터 GAS 데이터 에셋과 행 슬롯](https://github.com/jaykop/Kata/issues/34)  
확인 상태: Attributes 섹션은 2026-10-05 사용자가 PIE에서 행 적용을 확인했다. 2026-10-06 사용자가 Editor 빌드와 PIE에서 Effects 섹션(GE_StaminaRegen으로 Stamina 회복)을 확인했다. Abilities 섹션과 Identity Tags는 실행 미확인

## 목적과 준비

캐릭터가 쓰는 GAS 데이터(AttributeSet과 초기값, Gameplay Ability, Gameplay Effect)를 Data Asset으로 묶고, 캐릭터 데이터 테이블 행에서 조합해 생성할 때 적용한다.
캐릭터 고유의 특성 태그는 공유 에셋이 아니라 행의 Identity Tags에 둔다.

- 캐릭터는 `AKataCharacter` 계열이어야 한다. 컴포넌트 초기화 직후 ASC의 Actor Info를 초기화하고 바로 적용한다.
- 행이 참조하는 Gameplay Data는 캐릭터를 생성하기 전에 비동기로 로드된다. Gameplay Data가 참조하는 Ability·Effect 클래스도 함께 로드된다.
- 스탯 자체의 정의와 피해 계산은 [Attribute 사용법](Attributes.md)을 따른다.

## 사용 순서

1. 콘텐츠 브라우저에서 Data Asset → `Kata Gameplay Data`를 만든다.
2. 필요한 섹션을 채운다.
   - Attributes: Attribute Sets에 세트를 넣고, Initial Values에 Attribute와 값을 넣는다. 최대값(MaxHealth 등)만 넣으면 현재값은 가득 찬 상태로 시작한다.
   - Abilities: Granted Abilities에 Ability 클래스와 레벨을 넣는다.
   - Effects: Granted Effects에 자기 자신에게 적용할 GE와 레벨을 넣는다. 스태미나 회복처럼 상시 동작하는 Infinite GE가 여기에 들어간다.
3. 캐릭터 데이터 테이블 행의 Gameplay 카테고리에서 Gameplay Data 배열에 에셋을 추가한다.
4. 캐릭터 고유 특성이 있으면 같은 카테고리의 Identity Tags에 `Identity` 루트 태그를 넣는다.
5. 액션 편집기에서 스탯·Ability가 필요한 액션을 미리 보려면 액션의 Preview Setups에 `Gameplay Data`를 추가하고 같은 에셋과 Identity Tags를 지정한다.

여러 에셋을 배열로 조합할 수 있다. 예를 들어 여러 몬스터가 공유하는 Ability 구성과 몬스터별 스탯을 서로 다른 에셋으로 나눈다.

## 주요 설정과 실행 규칙

| 항목 | 의미 | 빈 값·실패 시 동작 |
|---|---|---|
| Attribute Sets | ASC에 추가할 세트 클래스 | ASC에 같은 클래스가 이미 있으면 새로 만들지 않는다 |
| Initial Values | Attribute와 초기 기본값(`FScalableFloat`) | Curve Table을 지정하면 적용 레벨(기본 1)로 평가한다. 세트가 없는 Attribute는 경고 후 건너뛴다 |
| Granted Abilities | Ability 클래스와 레벨. Input ID는 쓰지 않는다 | 클래스가 빈 항목은 건너뛴다. Source Object는 해당 Gameplay Data다 |
| Granted Effects | 자기 ASC에 적용할 GE와 레벨 | 클래스가 빈 항목은 건너뛴다. Instant GE는 적용 즉시 끝나고 핸들이 남지 않는다 |
| Identity Tags (행, 프리뷰 셋업) | 캐릭터가 존재하는 동안 바뀌지 않는 특성 태그. 선택기는 `Identity` 루트만 보여 준다 | ASC에 Loose 태그로 더한다 |

`UKataGameplayData::ApplyAll`은 다음 순서로 적용한다.

1. 모든 에셋의 AttributeSet 추가
2. 초기값 설정. 같은 Attribute가 여러 에셋에 있으면 배열 뒤쪽 값을 쓰고 경고를 남긴다
3. Granted Effects 적용. 초기값을 먼저 넣으므로 회복 GE처럼 Attribute를 참조하는 GE가 올바른 값으로 시작한다
4. Granted Abilities 부여
5. Identity Tags 추가

적용 결과(추가한 세트, Ability·Effect 핸들, 태그)는 `FKataGameplayDataHandles`로 돌려주며 캐릭터가 보관한다. 부여한 항목을 되돌리는 함수는 아직 없다. 캐릭터가 사라지면 ASC와 함께 정리된다.

데이터 검증은 빈 세트·중복 세트·중복 Attribute·클래스가 빈 Ability·Effect 항목을 오류로, 이 에셋에 없는 세트의 Attribute를 경고로 알린다.

### 테스트 방법

PIE 콘솔에서 `showdebug abilitysystem`으로 Attribute를 표시하고, 엔진 GAS 치트 `AbilitySystem.Effect.Apply <GE 이름>`으로 플레이어 폰에 GE를 적용한다.
샘플의 `GE_Test_StaminaDrain`은 Stamina를 50 낮추는 Instant GE다. 이 명령은 메모리에 로드된 GE만 찾으므로, 찾지 못하면 콘텐츠 브라우저에서 GE를 한 번 연다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 초기값이 적용되지 않는다 | 해당 세트가 어느 Gameplay Data에도 없다 | 출력 로그의 `skipped ... has no attribute set` 경고를 확인하고 세트를 추가한다 |
| 프리뷰에서 비용·Attribute 조건이 맞지 않는다 | 프리뷰 액터에는 행이 적용되지 않는다 | Preview Setups에 `Gameplay Data`를 추가한다 |
| 생성 중에 부여한 항목을 회수할 수 없다 | 회수 함수가 없다 | 보스 페이즈 교체처럼 필요해지면 `FKataGameplayDataHandles` 기록으로 회수하는 기능을 추가한다 |
| 초기값 레벨을 바꿀 수 없다 | 캐릭터 생성은 레벨 1로 적용한다. 레벨을 정하는 곳은 아직 없다 | 후속 결정 |

## 확인 상태와 근거

Attributes 섹션의 행 적용은 2026-10-05 사용자 PIE 확인([Attribute 사용법](Attributes.md) 참고). 2026-10-06 사용자가 Editor 빌드 후 PIE에서 BlackKnight의 Stamina를 `AbilitySystem.Effect.Apply GE_Test_StaminaDrain`으로 50 낮췄을 때 Granted Effects의 `GE_StaminaRegen`으로 다시 올라가는 것을 확인했다. Granted Abilities, Identity Tags, 프리뷰 셋업은 실행으로 확인하지 않았다.

- [KataGameplayData.h](../../Plugins/KataFramework/Source/KataFramework/Public/Character/KataGameplayData.h): 섹션 구조체, `FKataGameplayDataHandles`, `ApplyAll`.
- [KataCharacterRow.h](../../Plugins/KataFramework/Source/KataFramework/Public/Character/KataCharacterRow.h): Gameplay Data, Identity Tags.
- [KataPreviewSetup_GameplayData.h](../../Plugins/KataFramework/Source/KataFramework/Public/Character/KataPreviewSetup_GameplayData.h): 프리뷰 적용.
- [작업 상태](https://github.com/jaykop/Kata/issues/34).
