# 애니메이션 레이어 사용법

갱신: 2026-10-04  
대상: KataFramework의 `UKataAnimInstance`·`UKataAnimLayerInstance`·`UKataAnimLayerSetup`으로 캐릭터 Anim Blueprint를 구성하는 사용자  
적용 기준: [애니메이션 레이어 구조 결정](../devlog/2026-10-04-Anim-Layer-Structure.md), [장비·무기 계획](../plan/Equipment-Plan.md) EQ-3  
확인 상태: 2026-10-04 사용자가 BlackKnight로 Anim Blueprint 편집기 프리뷰와 액션 편집기 프리뷰의 Idle 재생을 확인했다. PIE의 이동 전환, 무기 장착·해제에 따른 레이어 교체, Game 타깃 빌드는 미확인

## 목적과 준비

메인 Anim Blueprint 하나가 상태 머신과 전이를 맡고, 무기마다 다른 애니메이션은 Linked Anim Layer로 바꿔 끼운다.
무기를 바꾸면 상태 전환 규칙은 그대로이고 포즈만 바뀐다. 몸 구조(2족·4족·뱀·비행)나 역할(PC·몬스터)로 C++ Anim Instance를 나누지 않는다.

| 구성 | 종류 | 역할 |
|---|---|---|
| `UKataAnimInstance` | C++, 메인 ABP의 부모 | 이동 값 계산, GAS 태그→변수 매핑 |
| `UKataAnimLayerInstance` | C++, 레이어 ABP의 부모 | 메인 인스턴스를 돌려주는 `Get Main Anim Instance` |
| Anim Layer Interface | 에셋 | 레이어 함수 목록. 무기 레이어용과 Body 레이어용을 따로 둔다 |
| `UKataAnimLayerSetup` | 데이터 에셋, 스켈레톤마다 하나 | Body 레이어, 기본 무기 레이어, `Equipment.Type` 태그별 무기 레이어 |
| `UKataEquipmentComponent` | 컴포넌트 | 레이어 설정에 따라 캐릭터 Mesh에 레이어를 링크 |

캐릭터는 `AKataCharacter` 계열이어야 한다. 레이어 링크는 캐릭터의 `Kata Equipment` 컴포넌트가 한다.

## 사용 순서

공용 에셋은 한 번만 만든다.

1. Anim Layer Interface(예: `ALI_KataWeapon`)를 만들고 레이어 함수(`FullBody_Idle`, `FullBody_Walk`, `FullBody_Run` 등)를 입력 없이 추가한다.
2. 메인 Template ABP(예: `ABP_CharacterBase`)를 만든다. Parent Class는 `Kata Anim Instance`, Interfaces에 1의 ALI를 추가한다.
   상태 머신과 전이 조건은 부모의 변수(`Ground Speed` 등)로 만들고, 각 상태 안에는 같은 이름의 레이어 노드 하나만 둔다. 애니메이션 에셋은 넣지 않는다.
3. 레이어 Template ABP(예: `ABP_WeaponLayersBase`)를 만든다. Parent Class는 `Kata Anim Layer Instance`, Interfaces에 같은 ALI를 추가한다.
   애니메이션·Blend Space 변수를 기본값 없이 선언하고 레이어 함수를 구현한다. Blend Space의 X·Y는 Property Access로 `Get Main Anim Instance` → `Local Right Speed`·`Local Forward Speed`에 바인딩한다.

캐릭터(스켈레톤)마다 반복한다.

4. 2의 자식 ABP(예: `ABP_BlackKnight`)를 만들고 스켈레톤을 지정한다.
5. 무기 종류마다 3의 자식 ABP(예: `ABP_BlackKnight_Layers_Greatsword`)를 만들고, 스켈레톤을 지정한 뒤 Class Defaults에서 애니메이션 변수만 채운다.
6. `Kata Anim Layer Setup` 데이터 에셋을 만들어 `Skeleton`, `Default Weapon Layer`, `Weapon Layers`(태그 → 5의 레이어)를 지정한다.
7. 6을 세 곳에 지정한다.
   - 캐릭터 테이블 행의 `Anim Layer Setup`: 게임에서 쓴다.
   - 캐릭터 Blueprint의 `Kata Equipment` 컴포넌트 `Anim Layer Setup`: 행이 없을 때와 BP 뷰포트·액션 편집기 프리뷰에서 쓴다.
   - 4의 Class Defaults `Preview Anim Layer Setup`: Anim Blueprint 편집기 프리뷰에서만 쓴다.
8. 장비 행의 `Equipment Type`에 무기 종류 태그를 지정한다. 오른손(Equipment Setup의 Default Slot)에 그 장비를 장착하면 해당 무기 레이어로 바뀐다.

## 주요 설정과 실행 계약

| UI 항목 또는 API | 의미·입력 | 기본값·빈 값·실패 시 동작 |
|---|---|---|
| `Kata Anim Instance`의 `Kata\|Locomotion` 변수 | `Velocity`, `Acceleration`, `Ground Speed`, `Local Forward Speed`, `Local Right Speed`, `Velocity Direction Angle`(-180~180도), `Has Acceleration`, `Is Falling`, `Is On Ground`, `Movement Mode` | 소유자가 캐릭터가 아니면(ABP 프리뷰 등) 기본값. Blueprint에서는 읽기 전용 |
| `Gameplay Tag Property Map` | ASC 태그가 붙고 떨어질 때 ABP의 bool·int·float 변수를 갱신 | ASC가 없으면 갱신하지 않는다. 변수 이름·타입이 틀리면 데이터 검증 오류 |
| `Get Main Anim Instance` | 레이어가 링크된 메시의 메인 인스턴스 | Thread Safe. 메인이 `Kata Anim Instance`가 아니거나 없으면 None이며 Property Access는 0을 넣는다 |
| `Skeleton` | 레이어 설정의 대상 스켈레톤 | 메시의 스켈레톤과 다르면 링크하지 않고 경고. 비우면 검사하지 않는다 |
| `Body Layer` | 무기와 무관한 레이어(발 IK 등) | 비워도 된다. 무기 레이어를 바꿔도 유지된다 |
| `Default Weapon Layer` | 무기가 없을 때의 무기 레이어 | 비어 있고 무기도 없으면 메인 ABP의 기본 구현(비어 있으면 기준 포즈)이 쓰인다 |
| `Weapon Layers` | `Equipment.Type` 태그별 무기 레이어 | 태그가 없으면 경고 후 기본 무기 레이어 |
| `Set Anim Layer Setup` | 컴포넌트의 레이어 설정 교체 | 즉시 다시 링크한다 |

- 링크 시점: 메시의 Anim Instance가 초기화될 때, 장착·해제할 때, 레이어 설정을 바꿀 때. 캐릭터 행이 Anim Class를 바꿔도 다시 링크된다.
- 레이어 클래스는 하드 참조라 레이어 설정을 로드하면 그 스켈레톤의 모든 무기 레이어와 애니메이션이 함께 로드된다.
- 데이터 검증은 레이어 ABP의 Target Skeleton이 `Skeleton`과 다르면 오류를 낸다. 스켈레톤이 없는 Template 레이어는 검사하지 않는다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| T 포즈가 나온다 | 링크된 레이어가 없어 메인 ABP의 빈 기본 구현이 쓰인다 | 7의 세 곳과 `Default Weapon Layer`를 확인한다. `LogKataFramework`에 `Layers are not linked`가 있으면 스켈레톤이 다르다 |
| 무기를 바꿔도 포즈가 같다 | 장비 행의 Equipment Type이 비었거나 Weapon Layers에 없음, 오른손이 아닌 슬롯에 장착 | 경고 로그 `has no weapon layer for`와 장비 행을 확인한다 |
| BP 뷰포트에 무기가 없다 | 시작 장비는 BeginPlay에서 장착한다 | 정상이다. 액션 편집기 프리뷰는 Preview Setups로 장착한다 |
| 레이어 ABP에서 `Get Main Anim Instance`가 안 보인다 | Parent Class가 `Kata Anim Layer Instance`가 아니거나 빌드 전 | 부모를 확인하고 빌드한다 |
| 레이어에서 `bGuard` 같은 BP 변수를 읽을 수 없다 | 레이어는 메인 인스턴스를 C++ 타입으로만 안다 | 지원하지 않는다. 레이어용 태그 매핑은 아직 없다 |

왼손 무기의 레이어, 손별로 다른 Anim Layer Interface, Body 레이어 예제는 아직 없다.

## 확인 상태와 근거

- [KataAnimInstance.h](../../Plugins/KataFramework/Source/KataFramework/Public/Animation/KataAnimInstance.h): 이동 값, 태그 매핑, 프리뷰 레이어.
- [KataAnimLayerInstance.h](../../Plugins/KataFramework/Source/KataFramework/Public/Animation/KataAnimLayerInstance.h): `GetMainAnimInstance`.
- [KataAnimLayerSetup.h](../../Plugins/KataFramework/Source/KataFramework/Public/Animation/KataAnimLayerSetup.h): 레이어 선택, 스켈레톤 검사, 데이터 검증.
- [KataEquipmentComponent.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Equipment/KataEquipmentComponent.cpp): `RefreshAnimLayers`, `OnAnimInitialized` 바인딩.
- 사용자 확인은 문서 머리의 확인 상태를 따른다. [작업 상태](https://github.com/jaykop/Kata/issues/32).
